#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * BreedLife(short type, long x, long y, long z, long r);
 *     APPEAR.C:202, 50 src lines, frame 160 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s4       short type
 *     param $s5       long x
 *     param $s7       long y
 *     param $s6       long z
 *     param stack+16  long r
 *     reg   $v0       long r
 *     reg   $s0       struct Humanoid * human
 *     reg   $s2       unsigned long * model
 *     reg   $s0       unsigned long idx
 *     stack sp+16     unsigned char [100] name
 *
 * Globals it touches, as the original declared them:
 *     extern struct HumanDataType HumanData[63];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 451 of 628 bytes differ, but the LENGTH matches
 * exactly (157 of 157 instructions) and every block/branch/call/offset is
 * confirmed correct against the raw `.s` (asmdiff --structural shows zero
 * insert/delete blocks, only register-choice replacements). The residual is
 * that this draft's cc1 invocation needs ONE MORE simultaneously-live
 * callee-saved register than the target: target manages the whole function
 * in $s0-$s7 (8 registers: x,y,z,type + idx/human + model + a "found entry"
 * pointer + the HumanData[] array-base/walking-pointer, the last two
 * literally SWAPPING roles via `move $s1,$s0`/`move $s0,$s3` right before
 * the rename loop), while every spelling tried here needs a 9th ($s8, i.e.
 * the frame pointer register — only available because -fomit-frame-pointer
 * frees it for general use). Tried and rejected: computing the "found
 * entry" pointer (`pp`) before vs. after the SystemOut guard (both compile,
 * neither drops below 1-too-many registers; "after" is the closer of the
 * two and is what's kept); reusing `human`'s own register for the pp role
 * via a cast (worse, 118 vs 92 differing lines); `pp = HumanData + idx`
 * vs `&HumanData[idx]` (byte-identical, as expected). A bounded
 * `tools/permute.py -j4 --stop-on-zero` run (~400s, 4 workers) plateaued at
 * score ~4090, nowhere near 0 — confirms this is a structural
 * register-pressure tie, not a permuter-crackable statement-order/coloring
 * tie. `tools/regalloc.py` shows the target's specific two-way role-swap
 * (`$s0`/`$s1`/`$s3` all changing roles at the rename-loop boundary) is
 * simply not reachable from a source-level respelling tried so far.
 *
 * BreedLife (0x8002a018, 0x278 bytes) — spawns a new Humanoid of `type` at
 * ground position (x,z) with yaw `r`: resolves `type` to its HumanData[]
 * row (linear search, sentinel .type==-1), loads the row's .MAD model file
 * on first use (and patches every OTHER HumanData row sharing that same
 * filename to point at the freshly-loaded model too — a cache shared by
 * name, not by index), then CreateHumanoid's the instance and positions it
 * (point[]/model->locate.coord.t[] snap Y to GetAreaMapLevel's terrain
 * height), before applying two type-range special cases (ninja-dog leash
 * flag; auto-equip a weapon for player-ish/low types; a "guard" attribute
 * bit for a high type range).
 *
 * The demo's PSX.SYM register roles for the 4 register params ($s4=type,
 * $s5=x, $s7=y, $s6=z) do NOT match retail's actual assignment (verified
 * against the raw `.s`: prologue copies a1->s7(x), a2->s6(y), a3->s4(z),
 * a0->s5(type)) — just the parameter ORDER/TYPES carry over, not which
 * register each lands in; the C source doesn't need to name registers.
 *
 * splat/reverse.py's carve splits this one function into TWO INCLUDE_ASM
 * pieces (`BreedLife` + `create_character__override__prt_...`) — that
 * second symbol is a Ghidra call-site prototype-override marker sitting
 * inside this function's own byte range, NOT a second function (see
 * CVAsetup.c's `reload_animations__override__` sibling case). One `BreedLife`
 * definition below produces both pieces.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - Ghidra renders the not-found path as a hard stop ("WARNING: Subroutine
 *    does not return" after SystemOut), but the raw `.s` proves SystemOut is
 *    NOT noreturn here: execution falls straight through afterward into the
 *    SAME code the success path reaches (recomputing `&HumanData[idx]` and
 *    continuing to index it, model and all, even though idx's row is the
 *    sentinel). This is the cookbook's "guard clause with no second return
 *    is a plain if" family taken to its limit — there is no early return at
 *    all here, just a diagnostic call with no effect on control flow. Two
 *    INDEPENDENT tests reach the one `SystemOut` call site: the initial
 *    `HumanData[0].type == -1` (empty table) skips the search loop
 *    entirely via a direct branch, and the search loop's own post-loop
 *    `HumanData[idx].type == -1` (not found) falls into the same call —
 *    spelled as two `if (cond) goto callit;` tests sharing one label
 *    (the two-independent-goto-into-one-label family), not a nested
 *    if/return.
 *  - The search loop is a genuine walking-pointer `do { ... } while`, not an
 *    index-based search (unlike SetupCharacterParameter's sibling loop in
 *    the same TU): the raw `.s` increments a real HumanDataType* by 0x18
 *    every iteration (Ghidra's own `pHVar5 = pHVar5 + 1;` is literal, not
 *    an SSA artifact here). The `idx` counter only increments on iterations
 *    that DON'T break, so it deliberately diverges from the pointer's own
 *    position — the post-loop code recomputes `&HumanData[idx]` from `idx`
 *    via the `*24` strength reduction (`idx*3<<3`), it does NOT reuse the
 *    walking pointer's final value (confirmed: no register carries the
 *    pointer out of the loop).
 *  - `human->model->locate.coord.t[N]`/`.rotate.vy` are each written via a
 *    FRESH re-dereference of `human->model` (5 separate `lw`s at the same
 *    +0x58 offset, one per access, including across the intervening
 *    `GetAreaMapLevel`/`UpdateCoordinate` calls) — Ghidra's own `pMVar4 =
 *    human->model;` cache is an SSA artifact for exactly ONE of the five
 *    accesses and reproduces nothing extra if written literally each time
 *    instead (CVArun/vrealloc's "target reloads a fresh field dereference"
 *    family).
 *  - `GlobalAreaMap` is read into a temp BEFORE the point[]/coord.t[0]
 *    stores that don't need it, matching Ghidra's own `puVar7 =
 *    GlobalAreaMap;` position — read early, consumed only much later at the
 *    `GetAreaMapLevel` call.
 *  - `GetAreaMapLevel`'s real prototype (DebugMenuItemSet.c) takes 5 args
 *    (map,x,y,z,mode); Ghidra's rendering here drops the trailing `z,1`
 *    (the m2c/Ghidra call-arg-undercount family) — the raw `.s` sets up
 *    a3=z and a stack mode=1 that are never otherwise touched, so the real
 *    call is `GetAreaMapLevel(map, x, y, z, 1)`.
 *  - The two type-range special cases are a plain `if (type<0x8b) {...}
 *    else if (type<0xa8 && 0xa5<type) {...}` — the raw `.s` tests `type<0x8b`
 *    FIRST and branches to the nested block on true, matching this polarity
 *    directly (no De Morgan inversion needed here, unlike several other
 *    guard-clause functions in this TU family).
 */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/BreedLife", BreedLife);
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/BreedLife", create_character__override__prt_8002a100_aee7b64a);
#else /* NON_MATCHING */

typedef struct
{
    s16 type;   /* 0x0 */
    s16 wepid;  /* 0x2 */
    s16 turn;   /* 0x4 */
    s16 life;   /* 0x6 */
    s16 width;  /* 0x8 */
    s16 height; /* 0xA */
    MotionRegistType *mtbl; /* 0xC */
    u8 *name;   /* 0x10 */
    u32 *model; /* 0x14 */
} HumanDataType; /* 0x18 */

extern HumanDataType HumanData[63];
extern u32 *GlobalAreaMap;

extern Humanoid *CreateHumanoid(s16 type, u32 *mad);
extern long GetAreaMapLevel(void *map, long x, long y, long z, long e);
extern void UpdateCoordinate(ModelType *dim);
extern void EquipWeapon(Humanoid *human, s16 mode);
extern short SetNowMotion(Humanoid *human, s16 mid, s16 move);
extern void SystemOut(char *msg);
extern int sprintf(char *buf, char *fmt, ...);
extern u32 *FileRead(char *path);
extern int strcmp(char *a, char *b);

extern char D_800117C8[]; /* "ILLIGAL CHARACTER TYPE" */
extern char D_800117E0[]; /* "%s%s.MAD" */
extern char D_80011734[]; /* "K:\\WORK\\CDIMAGE\\HUMAN\\" */

Humanoid *BreedLife(s16 type, s32 x, s32 y, s32 z, s32 r)
{
    s16 idx;
    s32 sVar1;
    HumanDataType *pHVar5;
    HumanDataType *pp;
    u32 *model;
    Humanoid *human;
    u8 name[100];

    idx = 0;
    if (HumanData[0].type != -1)
    {
        pHVar5 = HumanData;
        sVar1 = HumanData[0].type;
        do
        {
            pHVar5 = pHVar5 + 1;
            if (sVar1 == type)
                break;
            sVar1 = pHVar5->type;
            idx = idx + 1;
        } while (sVar1 != -1);
    }
    if (HumanData[idx].type == -1)
    {
        SystemOut(D_800117C8);
    }
    pp = &HumanData[idx];

    model = pp->model;
    if (model == 0)
    {
        sprintf((char *)name, D_800117E0, D_80011734, pp->name);
        model = FileRead((char *)name);
        pp->model = model;
        pHVar5 = HumanData;
        sVar1 = HumanData[0].type;
        while (sVar1 != -1)
        {
            if (strcmp((char *)pp->name, (char *)pHVar5->name) == 0)
            {
                pHVar5->model = model;
            }
            pHVar5 = pHVar5 + 1;
            sVar1 = pHVar5->type;
        }
    }

    human = CreateHumanoid(type, model);
    model = GlobalAreaMap;
    human->point[0] = x;
    human->model->locate.coord.t[0] = x;
    human->model->locate.coord.t[1] = GetAreaMapLevel(model, x, y, z, 1);
    human->point[1] = z;
    human->model->locate.coord.t[2] = z;
    human->model->rotate.vy = r;
    UpdateCoordinate((ModelType *)human->model);

    if (type == 0x87)
    {
        human->item[3] = 1;
    }
    if (type < 0x8B)
    {
        if ((0x80 < type) || (type < 2 && -1 < type))
        {
            human->attribute = human->attribute | 2;
            EquipWeapon(human, 1);
            SetNowMotion(human, 0x501, 1);
        }
    }
    else if (type < 0xA8 && 0xA5 < type)
    {
        human->attribute = human->attribute | 0x20;
    }
    return human;
}
#endif /* NON_MATCHING */
