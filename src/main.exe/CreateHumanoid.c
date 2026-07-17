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
 * chain (A) and the signed half-height chain (B) are interleaved in the
 * opposite order and consequently receive the mirror register roles.
 *   target:  A1..A4, lhu, A5, B1..B5, sh     (address chain first)
 *   ours:    lhu, A1, B1..B5, A2..A5, sh     (half chain first)
 * The instruction MULTISET is identical (127/127 insns, same ops, same
 * operands) — this is a pure reorder, not a missing/surplus instruction.
 *
 * NOT a division-idiom bug. The target HAS the signed divide-by-2 trio
 * (`srl a0,a0,0x1f; addu v0,v0,a0; sra v0,v0,0x1` at 0x80027a34..3c) and the
 * `sll 16/sra 16` sign-extension (0x80027a2c/30) — they sit 4 instructions
 * lower than ours, so a diff read column-wise makes them look absent. Do not
 * "fix" this by trying `>>1` or an unsigned dividend: both chains already
 * match the target instruction-for-instruction. tools/access.py proves
 * height@0xE is `lhu` on BOTH reads and width@0xC is `lh`+`lhu`, and the
 * target's lhu+sll/sra (rather than a bare `lh`) proves the value passes
 * through a u16 REGISTER before sign-extension — so the `hh`/`ww` u16 locals
 * are required, not incidental.
 *
 * MEASURED DEAD ENDS (do not re-derive):
 *  - RTL STATEMENT ORDER IS INERT HERE. `expand_assignment` really is
 *    LHS-first for a COMPONENT_REF/ARRAY_REF destination (get_inner_reference
 *    + offset expand before store_field expands the RHS), so
 *    `ConflictObject[idx].size.vy = half = (s16)(hh = human->height) / 2;`
 *    provably moves the load from RTL line 15 to line 66, putting chain A
 *    ahead of it in the .rtl dump — and still yields a BYTE-IDENTICAL object
 *    (tools/nullcheck.py: same hash). sched2 re-derives its schedule from the
 *    DAG; LUID is only its last-resort tiebreak and never reaches it here.
 *  - `ConflictObjectType *co = &ConflictObject[idx];` -> 52. It collapses the
 *    address into one pseudo and SINKS the `lui/addiu` from 0x80027a08 to
 *    0x80027a34. The repeated ARRAY_REF is correct: `lui %hi` has no operands,
 *    so sched2 hoists it into the post-call slots, which is what the target does.
 *  - `s32 idx` -> 53 (also 53 combined with the RHS fold): it moves the
 *    sign-extend to the assignment (line 153), emitting `sll,sra,lui` where the
 *    target has `lui,addiu,sll,sra`. `s16 idx` is correct — the target's
 *    sign-extend belongs to the SUBSCRIPT USE, not the assignment.
 *  - tools/autorules.py: 27 candidates, no improvement (`hh: u16->s16` and
 *    `half: s32->s16` are both inert at 39). Permuter: 420 s, -j4, nothing.
 *
 * WHERE IT ACTUALLY DIVERGES (for whoever picks this up): sched2 owns it. In
 * the .sched2 dump our order is 182,183,184,170,185,... — after scheduling 184
 * (`sll v0,v0,16`) the ready list holds 185 (`sra v0,v0,16`, dependent) and 170
 * (the lhu, independent); ours takes 170, the target takes 185. Since the DAG
 * and the RTL order both reproduce ours, the target's DAG must differ upstream
 * in a way not yet identified — the lhu's chain must outrank chain A for us and
 * not for it.
 *
 * This is NOT "just a scheduler tie": tools/siblingdiff.py --demo shows the
 * DEMO build (0x800247fc, ConflictObject stride 0x68 -> a 6-insn *104 chain
 * instead of retail's 4-insn *120) ALSO completes the whole address chain
 * before `lhu v0,14(s0)`. Two builds with different strides and register roles
 * agree on address-first, so the order is a source-structure property that is
 * still reachable from C. Keep hunting the source difference; do not disturb
 * the now-exact model block.
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
