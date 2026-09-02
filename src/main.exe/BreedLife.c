#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * BreedLife(short type, long x, long y, long z, long r);
 *     APPEAR.C:202, 50 src lines, frame 160 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * MATCHED — byte-exact at the full 632-byte / 158-instruction extent,
 * fence-free and volatile-free.
 *
 * CARVE FIX (kept for the record): the Ghidra functions.tsv size for
 * BreedLife (628) was UNDER-SIZED by one word — the same class as LoadCard /
 * decode_tmd_fast_ / AdtVsprintf (cookbook: "UNDER-SIZED functions.tsv entry").
 * The word at 0x8002a28c is BreedLife's own return-delay-slot `addiu
 * sp,sp,168`, which splat had parked as a data blob between BreedLife and
 * GetWeaponData; deleting that `{ start:0x19A8C, type:data, ... }` line in
 * config/splat.main.exe.yaml exposed the true 632-byte carve.
 *
 * BreedLife (0x8002a018, 0x278 bytes) — spawns a new Humanoid of `type` at
 * ground position (x,z) with yaw `r`: resolves `type` to its HumanData[]
 * row (linear search, sentinel .type==CHARACTER_KIND_END), loads the row's
 * .MAD model file
 * on first use (and patches every OTHER HumanData row sharing that same
 * filename to point at the freshly-loaded model too — a cache shared by
 * name, not by index), then CreateHumanoid's the instance and positions it
 * (point[]/model->locate.coord.t[] snap Y to GetAreaMapLevel's terrain
 * height), before applying the type special cases (a starter kusuri for
 * NINJA_0; auto-equip a weapon for player-ish/low types; a float attribute
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
 *    `HumanData[0].type == CHARACTER_KIND_END` (empty table) skips the
 *    search loop entirely via a direct branch, and the search loop's own
 *    post-loop `base[idx].type != CHARACTER_KIND_END` (found) branches
 *    around the same call — spelled as two goto tests sharing one label (the
 *    two-independent-goto-into-one-label family), not a nested if/return.
 *  - The first search is SetupCharacterParameter's index-based while idiom
 *    (same TU), NOT a source-level walking pointer: plain re-indexed reads
 *    in the while condition and the break test. Everything the asm shows is
 *    compiler machinery: loop.c strength-reduces the two reads into one
 *    walking-pointer giv ($v1, `addiu +0x18`), hoists the in-loop (s16)type
 *    widen and the cont-test's -1 into the preheader (the -1 cse2-folds
 *    onto the empty-check's still-tracked `li v1,-1`, giving `move a2,v1`),
 *    and reorg retargets the backedge past the redundant loop-top reload
 *    (0x8002a090 -> 0x8002a07c, skipping the 0x8002a078 lh that only the
 *    entry path runs). The explicit empty-table check is load-bearing: it
 *    makes jump.c's machine copy of the exit test fold away (cse's
 *    record_jump_equiv knows the value != -1 on the fall-through). Without
 *    it, the machine dup survives and derails the giv reduction (s8 frame,
 *    in-loop idx*24 recompute).
 *  - `base = HumanData` is deliberately the loop's FIRST BODY STATEMENT:
 *    as a loop.c movable it is hoisted into the post-check preheader slot
 *    (0x8002a070 `addiu a1,a1,%lo` in-place on the shared %hi), which is a
 *    position no pre-loop source statement can reach (a pre-loop assignment
 *    lands before the check's folded lh and re-homes every preheader
 *    register). It stays live to the post-loop found-check `base[idx]`
 *    (0x8002a0a4 `addu v0,v0,a1`); the giv init cse2-folds onto it
 *    (`move v1,a1`). Reachability is sound because every found-check path
 *    runs >= 1 loop body iteration (the empty path branches straight to
 *    SystemOut).
 *  - The join recomputes the row from a named table base with the ordinary
 *    `pp = &tbl[idx]` array expression.
 *    The separate tbl statement puts %hi/lo_sum BEFORE the idx*24 chain, so
 *    %hi is born while idx is still live: %hi -> $s1 (not idx's freed $s0),
 *    pp -> $s0, tbl -> $s3, and dbr copies the join's lui into the
 *    found-check's delay slot (0x8002a0b4) with the label split at
 *    0x8002a0c4/0x8002a0c8. A direct `&HumanData[idx]` emits the mult chain
 *    first and rotates all three homes.
 *  - The filename scan is a GOTO-loop (label + `if (...) goto`), not a
 *    do/while: with no LOOP notes, loop.c cannot hoist the scan's -1, which
 *    keeps retail's in-loop `li v0,-1` (0x8002a150). `q = pp` before the
 *    loop is the real copy retail has at 0x8002a124 (`move s1,s0`): pp dies
 *    at the copy so pp shares idx's $s0, and q takes the dead %hi's $s1.
 *    The copy survives because q's uses sit behind the loop label
 *    (optimize_reg_copy_1 cannot cross it).
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
 *  - `GetAreaMapLevel`'s real prototype (AddItem2.c) takes 5 args
 *    (area,x,y,z,mode); Ghidra's rendering here drops the trailing `z,1`
 *    (the m2c/Ghidra call-arg-undercount family) — the raw `.s` sets up
 *    a3=z and a stack mode=1 that are never otherwise touched, so the real
 *    call is `GetAreaMapLevel(area, x, y, z, 1)`.
 *  - The type-range dispatch keeps its low block in place with an inverse
 *    `type >= ANI` guard. The low block similarly guards the two rejection
 *    tests with `type < HANBE`; direct one-line returns then cross-jump to
 *    the shared epilogue. The remaining `done`/`high_type` edges preserve
 *    the required low-before-high physical block order.
 */

extern int sprintf(char *buf, char *fmt, ...);
extern int strcmp(char *a, char *b);

extern char msg_illigal_character_type[]; /* ILLIGAL CHARACTER TYPE */
extern char fmt_mad[];                    /* %s%s.MAD */
extern char path_human[];                 /* K:\\WORK\\CDIMAGE\\HUMAN\\ */

Humanoid *BreedLife(character_kind type, long x, long y, long z, long r)
{
    /* PSX.SYM and the retail multiply both show a full-width counter. */
    u32 idx;
    HumanDataType *row;
    HumanDataType *base;
    u_long *model;
    HumanDataType *pp;
    HumanDataType *tbl;
    HumanDataType *q;
    Humanoid *human;
    u8 name[100];

    idx = 0;
    if (HumanData[0].type == CHARACTER_KIND_END)
        goto illegal_type;
    /* First-search idiom as in SetupCharacterParameter (same TU): plain
     * re-indexed reads in the while condition and the break test; loop.c
     * strength-reduces them into one walking-pointer giv and hoists the
     * widen/-1, and reorg retargets the backedge past the redundant
     * loop-top reload. The explicit empty-table check makes jump.c's
     * machine copy of the exit test fold away (cse knows the value != -1
     * on the fall-through), and `base` — assigned as the loop's first
     * statement — is a loop.c movable, hoisted after that check, keeping
     * the table base alive for the post-loop found-check. */
    while (HumanData[idx].type != CHARACTER_KIND_END)
    {
        base = HumanData;
        if (base[idx].type == type)
            break;
        idx++;
    }
    if (base[idx].type != CHARACTER_KIND_END)
        goto type_found;
illegal_type:
    SystemOut(msg_illigal_character_type);
type_found:
    tbl = HumanData;
    pp = &tbl[idx];
    model = pp->model;
    if (model == 0)
    {
        sprintf((char *)name, fmt_mad, path_human, pp->name);
        model = FileRead(name);
        pp->model = model;
        if (HumanData[0].type != CHARACTER_KIND_END)
        {
            q = pp;
            row = HumanData;
        scan_next:
            if (strcmp((char *)q->name, (char *)row->name) == 0)
            {
                row->model = model;
            }
            row++;
            if (row->type != CHARACTER_KIND_END)
                goto scan_next;
        }
    }

    human = CreateHumanoid(type, model);
    human->point[HUMANOID_HOME_X] = x;
    human->model->locate.coord.t[0] = x;
    human->model->locate.coord.t[1] = GetAreaMapLevel(
        GlobalAreaMap, x, y, z, AREA_LEVEL_STEP_DOWN);
    human->point[HUMANOID_HOME_Z] = z;
    human->model->locate.coord.t[2] = z;
    human->model->rotate.vy = r;
    UpdateCoordinate((ModelType *)human->model);

    if (type == NINJA_0)
    {
        human->item[ITEM_KUSURI] = 1;
    }
    if (type >= ANI)
    {
        if (type >= ARROW)
            return human;
        if (type < S1)
            goto done;
        goto high_type;
    }

    if (type < HANBE)
    {
        if (type >= RIKIMARU_1)
            return human;
        if (type < 0)
            return human;
    }
    human->attribute = human->attribute | PHASE_ALERT;
    EquipWeapon(human, WEAPON_DRAWN);
    SetNowMotion(human, MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
    return human;
high_type:
    human->attribute = human->attribute | ATTR_FLOAT;
done:
    return human;
}
