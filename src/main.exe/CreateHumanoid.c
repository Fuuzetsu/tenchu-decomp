#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * CreateHumanoid(short type, unsigned long *mad);
 *     HUMAN.C:36, 31 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s1       short type
 *     param $s0       unsigned long * mad
 *     reg   $s0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct ModelType World;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — exact 508-byte length, 39 differing bytes, fuzzy
 * 90.55. The old missing-instruction root is fixed: assigning the loader's
 * result directly to `human->model`, then spelling `locate` before `rotate`,
 * gives the target's complete post-call sequence, including
 * `move v1,v0; addiu v1,v1,80`. The guard, allocation, model/think setup,
 * character setup, area-map/weapon calls, special 0x86/0x89 writes, and
 * Humans/HumanGroup append are now byte-for-byte exact.
 *
 * The only residual is 0x80027a14..0x80027a58: the collision index/address
 * and signed half-height chains are scheduled in the opposite order and
 * consequently receive the mirror register roles. The operations and
 * stores are equivalent and the instruction count is exact. Named
 * ConflictObject pointers and manually split index offsets make this block
 * worse; a bounded 100-candidate guided/aggressive autorules search from
 * this 39-byte state found no improvement. Treat this as a local scheduler/
 * allocator problem, not a reason to disturb the now-exact model block.
 *
 * CreateHumanoid (0x800278e4) — allocate and register a new Humanoid: zero
 * it, load its model archive, wire up locate/rotate into the archive's own
 * matrix, run the think/character-parameter setup, derive a half-height
 * clip offset, register it in the collision (Conflict) pool with half-width
 * boxes (special-cased offset for types 0x86/0x89), then append it to
 * HumanGroup. Aborts via SystemOut (noreturn) if `mad` is null or the
 * HumanGroup pool is full.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `if (mad == 0 || Humans >= 0x28) { SystemOut(...); } <unconditional
 *    body...>` — NOT nested inside the success guard. SystemOut never
 *    returns, and the target's actual block ORDER places the short
 *    SystemOut call INLINE right after the guard (reached by the `||`
 *    short-circuit's first-operand branch) with the long success body
 *    placed AFTER it, fallen into unconditionally — the polarity a plain
 *    `if (cond) { fail; } body...;` produces via De Morgan on Ghidra's
 *    `mad != 0 && Humans < 0x28`, not the nested "guard clause" shape.
 *  - `height`/`width` (item.h, s16) are each read TWICE for different
 *    purposes: once as a plain pass-through (GetAreaMapVector's `wide` arg,
 *    human->width — natural `lh`, no cast) and once copied into a same-width
 *    `u16` LOCAL for the half-size computation (forces `lhu`, matching the
 *    HUMAN.C-sibling "global copied into a same-width local" idiom), then
 *    explicitly `(s16)`-cast back for the signed divide-by-2 (the
 *    sll/sra + srl-bias/sra idiom cc1 emits for a provably-possibly-negative
 *    dividend — proves the cast, not a field-type change).
 *  - `ConflictObject[idx].size.vy = half;` then `.offset.vy = -half -
 *    model->rotate.pad;` — writing `-half` (not `-model->rotate.pad`) first
 *    negates the ALREADY-LIVE `half` register in place (matches asm's
 *    `negu` on the just-stored value, not a fresh negate of the reloaded
 *    pad).
 *  - `ConflictObject[idx].common = human;` sits BETWEEN reading `width` and
 *    computing its half — matches the asm's store scheduled between the
 *    `lhu` and the resign/divide chain.
 *  - `oldHumans = Humans; Humans = Humans + 1; HumanGroup[oldHumans] =
 *    human;` — Humans captured into a named temp BEFORE the increment
 *    (matches Ghidra's `uVar2 = Humans; Humans = Humans + 1;` capture-and-
 *    increment shape exactly): `HumanGroup[Humans++] = human;` instead
 *    computes the array address before the increment, one instruction off.
 */
extern Humanoid *vcalloc(u32 size, u8 c);
extern ModelArchiveType *LoadModelArchive(u32 *adr, ModelType *prnt);
extern void SetupThinkFunction(Humanoid *human, short type);
extern void SetupCharacterParameter(short type, Humanoid *human);
extern void SetupWeapon(Humanoid *human);
extern s16 InsertConflict(ModelType *m);
extern s32 GetAreaMapVector(void *area, void *mvp, void *pos, s32 wide, s32 mode);
extern void SystemOut(char *msg);

extern s16 Humans;
extern ModelType World;
extern void *GlobalAreaMap;
extern char D_80011658[]; /* "HUMAN OVERFLOW" */

/* Conflict slot (Ghidra: ConflictObjectType, 0x78 bytes; see DeleteConflict.c). */
typedef struct
{
    ModelType *model;            /* 0x00 */
    VECTOR position;             /* 0x04 */
    SVECTOR offset;              /* 0x14 */
    SVECTOR size;                /* 0x1C */
    void *common;                /* 0x24 */
    u8 result[64];                /* 0x28 */
    u8 pad[0x10];                 /* 0x68 */
} ConflictObjectType;             /* 0x78 */

extern ConflictObjectType ConflictObject[];
extern Humanoid *HumanGroup[];

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/CreateHumanoid", CreateHumanoid);
#else
Humanoid *CreateHumanoid(short type, u32 *mad)
{
    Humanoid *human;
    s16 idx;
    u16 hh;
    u16 ww;
    s32 half;
    s32 half2;
    s16 oldHumans;

    if (mad == 0 || Humans >= 0x28)
    {
        SystemOut(D_80011658);
    }
    human = vcalloc(0xd0, 0);
    human->type = type;
    human->status = 0;
    human->attribute = 0;
    human->model = LoadModelArchive(mad, &World);
    human->locate = (VECTOR *)human->model->locate.coord.t;
    human->rotate = &human->model->rotate;
    human->model->attribute = 0x1c;
    SetupThinkFunction(human, 0);
    SetupCharacterParameter(type, human);
    hh = human->height;
    human->model->clip.vy = -((s16)hh / 2);
    UpdateMotion(human->motion, 0);
    GetAreaMapVector(GlobalAreaMap, &human->map, human->locate, human->width, 1);
    SetupWeapon(human);
    idx = InsertConflict(human->model->object[0]);
    hh = human->height;
    half = (s16)hh / 2;
    ConflictObject[idx].size.vy = half;
    half = -half;
    ConflictObject[idx].offset.vy = half - human->model->rotate.pad;
    ww = human->width;
    ConflictObject[idx].common = human;
    half2 = (s16)ww / 2;
    ConflictObject[idx].size.vz = half2;
    ConflictObject[idx].size.vx = half2;
    if (type == 0x86 || type == 0x89)
    {
        ConflictObject[idx].offset.vy = -0x1C5;
        ConflictObject[idx].offset.vz = 0xC0;
    }
    oldHumans = Humans;
    Humans = Humans + 1;
    HumanGroup[oldHumans] = human;
    return human;
}
#endif /* NON_MATCHING */
