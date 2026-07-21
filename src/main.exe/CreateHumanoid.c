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
 * CreateHumanoid (0x800278e4) — allocate and register a new Humanoid: zero it,
 * load its model archive, wire up locate/rotate into the archive's own matrix,
 * run the think/character-parameter setup, derive a half-height clip offset,
 * register it in the collision (Conflict) pool with half-width boxes
 * (special-cased offset for types 0x86/0x89), then append it to HumanGroup.
 * Aborts via SystemOut (noreturn) if `mad` is null or the HumanGroup pool is
 * full.
 *
 * THE THREE-FACTOR SCHEDULING FIX (this cost four rounds — read before editing):
 * After InsertConflict the block holds two independent chains that both feed
 * `ConflictObject[idx].size.vy = half`:
 *   A (address): sll/sra sign-extend idx, *120 (sll4/subu/sll3), addu base
 *   B (value)  : lhu height, sll/sra resign, srl/addu/sra signed /2
 * The target completes A first, then loads height into the load-delay slot.
 * Three edits are each INDIVIDUALLY INERT (or worse) and only work TOGETHER:
 *   1. FOLD the divide into the store: `... .size.vy = half = (s16)hh2 / 2;`
 *      expand_assignment is LHS-first, so chain A is generated FIRST and gets
 *      the LOWER LUID. Alone: a nullcheck NO-OP (sched1 reorders it back).
 *   2. SPLIT `half` (`half` / `nhalf`) so each is SET ONCE. sched1's
 *      adjust_priority only bumps a BIRTHING insn (REG_N_SETS == 1) to
 *      LAUNCH_PRIORITY 0x7f000001. With `half = -half` reusing the variable,
 *      chain B's last insn stays at priority 9 and LOSES every ready-list
 *      contest to chain A's bumped insns. Alone: still 39.
 *   3. SPLIT `hh` per read site (`hh` / `hh2`). Only once 1+2 land does this
 *      matter — it gives the two height reads DIFFERENT registers (v1 for the
 *      clip read, a0 for the conflict read), which the target requires. Alone
 *      (at 39): a nullcheck NO-OP.
 * 1+2 = 20 bytes; 1+2+3 = MATCH. Because each factor scores "no change" on its
 * own, three earlier rounds recorded them individually as dead ends. If you
 * change any one of them, expect to lose all three.
 *
 * Verify with sched1, not sched2: sched2 faithfully reproduces the order it is
 * handed (`.greg` == its input == the emitted order). The decision is sched1's,
 * and `;; ready list at T-20: 180 (9) 193 (7f000001)` in the .sched dump is the
 * whole story. NOTE the trap: the `;; insn[N]: priority` TABLE is not what the
 * scheduler used — it prints 172 as priority 9 while the ready list shows
 * 172 (7f000001). Read the ready lists.
 *
 * Other matching notes (docs/matching-cookbook.md):
 *  - `if (mad == 0 || Humans >= 0x28) { SystemOut(...); } <unconditional
 *    body...>` — NOT nested inside the success guard. SystemOut never returns,
 *    and the target's block ORDER places the short SystemOut call INLINE right
 *    after the guard (reached by the `||` short-circuit's first-operand branch)
 *    with the long success body placed AFTER it, fallen into unconditionally.
 *  - `height`/`width` (item.h, s16) are each read TWICE for different purposes:
 *    once as a plain pass-through (GetAreaMapVector's `wide` arg, human->width —
 *    natural `lh`, no cast) and once copied into a same-width `u16` LOCAL for
 *    the half-size computation (forces `lhu`), then explicitly `(s16)`-cast back
 *    for the signed divide-by-2 (the sll/sra + srl-bias/sra idiom cc1 emits for
 *    a provably-possibly-negative dividend — proves the cast, not a field-type
 *    change).
 *  - `nhalf = -half;` then `.offset.vy = nhalf - model->rotate.pad;` — negating
 *    the just-stored value (not a fresh negate of the reloaded pad) is what
 *    produces the asm's `negu`. `half` and `nhalf` coalesce onto one hard
 *    register (gcc-2.8.1 has no coalescing pass; non-conflicting allocnos simply
 *    land together), so the split costs nothing.
 *  - `ConflictObject[idx].common = human;` sits BETWEEN reading `width` and
 *    computing its half — matches the store scheduled between the `lhu` and the
 *    resign/divide chain.
 *  - `oldHumans = Humans; Humans = Humans + 1; HumanGroup[oldHumans] = human;` —
 *    Humans captured into a named temp BEFORE the increment; `HumanGroup[Humans++]`
 *    instead computes the array address before the increment, one instruction off.
 *  - `s16 idx` is correct: the target's sign-extend belongs to the SUBSCRIPT USE,
 *    not the assignment (`s32 idx` hoists it to 0x80027a08 and scores 53). The
 *    repeated ARRAY_REF is also correct — a `ConflictObjectType *co` local sinks
 *    the `lui/addiu` and scores 52.
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

extern ConflictObjectType ConflictObject[];
extern Humanoid *HumanGroup[];

Humanoid *CreateHumanoid(short type, u32 *mad)
{
    Humanoid *human;
    s16 idx;
    u16 hh;
    u16 hh2;
    u16 ww;
    s32 half;
    s32 nhalf;
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
    hh2 = human->height;
    ConflictObject[idx].size.vy = half = (s16)hh2 / 2;
    nhalf = -half;
    ConflictObject[idx].offset.vy = nhalf - human->model->rotate.pad;
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
