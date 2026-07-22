#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetConflictResult(struct ModelType *model, short index);
 *     CONFLICT.C:382, 25 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
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
 *     param $a0       struct ModelType * model
 *     param $a2       short index
 *     reg   $t0       short i
 *     reg   $t1       short idx
 *
 * Globals it touches, as the original declared them:
 *     extern short ConflictObjects;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct SVECTOR ConflictDistance;
 *     extern struct ModelType *ConflictModel;
 * END PSX.SYM */

/*
 * GetConflictResult (0x8001a9b8) — query the ConflictObject conflict pool for a
 * live collision against `model`. `model->id` is its own slot in the pool; the
 * slot's `result[]` array flags which other slots it currently overlaps. When
 * `index < 0` the function SCANS result[0..ConflictObjects) for the next flagged
 * (nonzero) entry that has NOT already been consumed (bit 0x40); `offset.pad`
 * caps how many flagged entries may be examined. When `index >= 0` that specific
 * slot is used directly. On a hit the slot is marked consumed (|= 0x40), the
 * inter-model delta is published to ConflictDistance and the other model to
 * ConflictModel, and the slot index is returned; every failure path returns -1.
 *
 * Matching notes (docs/matching-cookbook.md; siblings DeleteConflict.c /
 * InsertConflict.c define the conflict-TU conventions). Three former
 * "below-the-C-level" residuals all turned out to be source structure,
 * found by RTL-dump analysis (cc1 -dj/-dc/-dS/-dg/-dl/-dJ/-dR/-dd):
 *  - id-guard is NESTED (`if (id != -1) { ... } return -1;`, DeleteConflict's
 *    own style), NOT an early `return -1;` guard: the final `return -1` is the
 *    id-guard's else path, cse elides its `li v0,-1` along the beq's taken edge
 *    (v0 still holds the compare's -1; single-predecessor label), leaving a
 *    bare label between the success `return index` and the function end. That
 *    label blocks jump.c's jump-to-next deletion, so the success return stays
 *    a real jump that reorg's make_return_insns converts to its OWN `jr` with
 *    `addu v0,a3` in the delay slot, followed by the bare `jr; nop` island the
 *    id==-1 beq targets. (An early-return spelling instead merges the success
 *    return into the island: shared jr, move outside the slot — 8B off.)
 *  - `ConflictModel = ...` is the FIRST store of the publish group (before
 *    .vx/.vy/.vz): sched1 then drops its lw into the vx pair's load-delay slot
 *    and its sw into the vy pair's, and the vz temps allocate to v1/a0 reusing
 *    the dying k-base (the `do{}while(0)` barrier hack this file used to carry
 *    is not needed). Ordering the model store between vx and vy instead leaves
 *    a nop in the vx slot and shifts sw below the vy subu (~26B).
 *  - The scan-loop guard: `index = 0; if (index >= ConflictObjects) goto
 *    ret_m1;` then `for (; index < ConflictObjects; index++)`. The guard's
 *    test folds (index==0 by cse) to `blez N -> ret_m1`, and cse1's
 *    record_jump_equiv on its fallthrough records LT(idx0, N) on the
 *    zero-valued quantity, which lets fold_rtx delete the for's
 *    duplicate_loop_exit_test entry copy (comparison_dominates_p(LT,LT) —
 *    the recorded comparison must sit on the copy's FIRST slt operand, which
 *    is why the guard must compare INDEX against N, not `ConflictObjects < 1`:
 *    that records on N's quantity and never folds — both blez's survive).
 *    The loop must stay a real `for`: duplicate_loop_exit_test's
 *    NOTE_INSN_LOOP_VTOP makes reorg's mostly_true_jump predict the
 *    result==0 skip-branch taken, filling both skip delay slots from the
 *    continue-point (`addiu v0,a2,1` twice); a do-while has no VTOP, the EQ
 *    heuristic predicts not-taken, and the fills come from the fallthrough.
 *  - The pad cap is `i > ConflictObject[iVar3].offset.pad` (`i` FIRST):
 *    expand evaluates op0 first, putting the short `i` sll before the
 *    lh of offset.pad (spelling it `pad < i` loads first — not a sched
 *    tie).
 *  - `model->id` is loaded TWICE, un-CSE'd (the DeleteConflict lhu-vs-lh
 *    split): `int iVar3 = model->id;` (lh — the `== -1` guard and the scan
 *    base id*0x78) and `short idx = model->id;` (lhu, narrowing — the
 *    post-loop result/position base). Different machine modes don't CSE.
 *  - `if (index < 0)` is `sll a1,16; bgez` (short sign test); `i = 0;`
 *    sits before it so reorg fills the bgez delay slot, and `index = 0`
 *    cse-copies the zero (`move a2,a3`).
 *  - All mid-function failure paths `goto ret_m1;`, a label INSIDE the
 *    attribute guard (the inverted `(attr & 0x4000) == 0` places its
 *    `jr; li v0,-1` island first, at 0x8001A9E0).
 *  - ConflictDistance is one SVECTOR whose three fields splat auto-named as
 *    SEPARATE, ADDRESS-DRIFTED D_80097EC8/ECC + D_80097ED0 glabels (8 bytes low
 *    in the .map — see the fresh correct-address binds in config/symbols).
 *    `(short)position.vx` reads the low half `lhu` (narrowing into the s16
 *    delta).
 */

short GetConflictResult(ModelType *model, short index)
{
    short idx;
    int iVar3;
    short i;
    int k;

    iVar3 = model->id;
    idx = model->id;
    if (iVar3 != -1)
    {
        if ((model->attribute & 0x4000) == 0)
        {
        ret_m1:
            return -1;
        }
        i = 0;
        if (index < 0)
        {
            index = 0;
            if (index >= ConflictObjects)
            {
                goto ret_m1;
            }
            for (; index < ConflictObjects; index++)
            {
                if (ConflictObject[iVar3].result[index] != 0)
                {
                    i++;
                    if (i > ConflictObject[iVar3].offset.pad)
                    {
                        goto ret_m1;
                    }
                    if ((ConflictObject[iVar3].result[index] & 0x40) == 0)
                    {
                        break;
                    }
                }
            }
        }
        k = index;
        if (k < ConflictObjects)
        {
            if (ConflictObject[idx].result[k] != 0)
            {
                ConflictObject[idx].result[k] |= 0x40;
                ConflictModel = ConflictObject[k].model;
                ConflictDistance.vx = (short)ConflictObject[k].position.vx - (short)ConflictObject[idx].position.vx;
                ConflictDistance.vy = (short)ConflictObject[k].position.vy - (short)ConflictObject[idx].position.vy;
                ConflictDistance.vz = (short)ConflictObject[k].position.vz - (short)ConflictObject[idx].position.vz;
                return index;
            }
        }
        goto ret_m1;
    }
    return -1;
}
