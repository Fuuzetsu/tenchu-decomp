#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * CreateHumanoid(short type, unsigned long *mad);
 *     HUMAN.C:36, 31 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * `ConflictObject[conflict_id].size.components.y = half`:
 *   A (address): sll/sra sign-extend conflict_id, *120
 *                (sll4/subu/sll3), addu base
 *   B (value)  : lhu height, sll/sra resign, srl/addu/sra signed /2
 * The target completes A first, then loads height into the load-delay slot.
 * Three edits are each INDIVIDUALLY INERT (or worse) and only work TOGETHER:
 *   1. FOLD the divide into the store: `... .size.components.y = half = (s16)hh2 / 2;`
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
 *  - `if (mad == 0 || Humans >= MAX_HUMANS) { SystemOut(...); } <unconditional
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
 *  - `nhalf = -half;` then `.offset.components.y = nhalf - model->rotate.pad;` — negating
 *    the just-stored value (not a fresh negate of the reloaded pad) is what
 *    produces the asm's `negu`. `half` and `nhalf` coalesce onto one hard
 *    register (gcc-2.8.1 has no coalescing pass; non-conflicting allocnos simply
 *    land together), so the split costs nothing.
 *  - `ConflictObject[conflict_id].common.human = human;` sits BETWEEN reading
 *    `width` and computing its half — matches the store scheduled between the
 *    `lhu` and the resign/divide chain.
 *  - `oldHumans = Humans; Humans = Humans + 1; HumanGroup[oldHumans] = human;` —
 *    Humans captured into a named temp BEFORE the increment; `HumanGroup[Humans++]`
 *    instead computes the array address before the increment, one instruction off.
 *  - `s16 conflict_id` is correct: the target's sign-extend belongs to the
 *    SUBSCRIPT USE, not the assignment (`s32 conflict_id` hoists it to
 *    0x80027a08 and scores 53). The repeated ARRAY_REF is also correct — a
 *    `ConflictObjectType *co` local sinks the `lui/addiu` and scores 52.
 */
extern void *vcalloc(u32 size, u8 c);
extern ModelArchiveType *LoadModelArchive(u_long *adr, ModelType *prnt);
extern void SetupThinkFunction(Humanoid *human, TThinkType type);

extern char msg_human_overflow[]; /* HUMAN OVERFLOW */

Humanoid *CreateHumanoid(character_kind type, unsigned long *mad)
{
    Humanoid *human;
    s16 conflict_id;
    u16 hh;
    u16 hh2;
    u16 ww;
    s32 half;
    s32 nhalf;
    s16 oldHumans;

    if (mad == 0 || Humans >= MAX_HUMANS)
    {
        SystemOut(msg_human_overflow);
    }
    human = (Humanoid *)vcalloc(sizeof(Humanoid), 0);
    human->type = type;
    human->status = STAT_NORMAL;
    human->attribute = 0;
    human->model = LoadModelArchive(mad, &World);
    human->locate = (VECTOR *)human->model->locate.coord.t;
    human->rotate = &human->model->rotate;
    human->model->attribute = MODEL_ATTR_CULL_BEHIND | MODEL_ATTR_CULL_SCREEN |
                              MODEL_ATTR_CULL_FAR;
    SetupThinkFunction(human, THINK_MIX_NONE);
    SetupCharacterParameter(type, human);
    hh = human->height;
    human->model->clip.vy = -((s16)hh / 2);
    UpdateMotion(human->motion, 0);
    GetAreaMapVector(GlobalAreaMap, &human->map, human->locate, human->width,
                     AREA_LEVEL_STEP_DOWN);
    SetupWeapon(human);
    conflict_id = InsertConflict(human->model->object[MODEL_PART_WAIST]);
    hh2 = human->height;
    ConflictObject[conflict_id].size.components.y = half = (s16)hh2 / 2;
    nhalf = -half;
    ConflictObject[conflict_id].offset.components.y =
        nhalf - human->model->rotate.pad;
    ww = human->width;
    ConflictObject[conflict_id].common.human = human;
    ConflictObject[conflict_id].size.components.x =
        ConflictObject[conflict_id].size.components.z =
        (s16)ww / 2;
    if (type == KUMA_0 || type == KUMA_1)
    {
        ConflictObject[conflict_id].offset.components.y = -0x1C5;
        ConflictObject[conflict_id].offset.components.z = 0xC0;
    }
    oldHumans = Humans;
    Humans++;
    HumanGroup[oldHumans] = human;
    return human;
}
