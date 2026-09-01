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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * (nonzero) entry that has NOT already been consumed (bit 0x40); `result_count`
 * caps how many flagged entries may be examined. When `index >= 0` that specific
 * slot is used directly. On a hit the slot is marked consumed (|= 0x40), the
 * inter-model delta is published to ConflictDistance and the other model to
 * ConflictModel, and the slot index is returned; every failure path returns
 * CONFLICT_NONE.
 *
 * Matching notes (docs/matching-cookbook.md; siblings DeleteConflict.c /
 * InsertConflict.c define the conflict-TU conventions). Three former
 * "below-the-C-level" residuals all turned out to be source structure,
 * found by RTL-dump analysis (cc1 -dj/-dc/-dS/-dg/-dl/-dJ/-dR/-dd):
 *  - id-guard is NESTED (`if (id != CONFLICT_NONE) { ... } return
 *    CONFLICT_NONE;`, DeleteConflict's own style), NOT an early return guard:
 *    the final CONFLICT_NONE return is the id-guard's else path; cse elides
 *    its `li v0,-1` along the beq's taken edge
 *    (v0 still holds the compare's -1; single-predecessor label), leaving a
 *    bare label between the success `return index` and the function end. That
 *    label blocks jump.c's jump-to-next deletion, so the success return stays
 *    a real jump that reorg's make_return_insns converts to its OWN `jr` with
 *    `addu v0,a3` in the delay slot, followed by the bare `jr; nop` island the
 *    id==CONFLICT_NONE beq targets. (An early-return spelling instead merges
 *    the success return into the island: shared jr, move outside the slot —
 *    8B off.)
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
 *  - The result-count cap is
 *    `i > ConflictObject[id].offset.components.result_count` (`i` FIRST):
 *    expand evaluates op0 first, putting the short `i` sll before the
 *    lh of result_count (spelling it `result_count < i` loads first — not a sched
 *    tie).
 *  - `model->id` is loaded TWICE, un-CSE'd (the DeleteConflict lhu-vs-lh
 *    split): `int id = model->id;` (lh — the CONFLICT_NONE guard and the
 *    scan base id*0x78) and `short idx = model->id;` (lhu, narrowing —
 *    the post-loop result/position base). Different machine modes don't CSE.
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

/* index CONFLICT_NONE walks the slot's result[] for the next unconsumed overlap,
 * marking it CONFLICT_CONSUMED and returning the partner's slot id —
 * giving up after result_count hits. A non-negative index reads that specific
 * partner entry directly (DamageControl). CONFLICT_NONE means no slot,
 * an inactive conflict, or nothing left. */
conflict_id GetConflictResult(ModelType *model, conflict_id index)
{
    conflict_id idx;
    int id;
    short i;
    int k;

    id = model->id;
    idx = model->id;
    if (id != CONFLICT_NONE)
    {
        if ((model->attribute & MODEL_ATTR_COLLIDE) == 0)
        {
        ret_m1:
            return CONFLICT_NONE;
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
                if (ConflictObject[id].result[index] != 0)
                {
                    i++;
                    if (i > ConflictObject[id].offset.components.result_count)
                    {
                        goto ret_m1;
                    }
                    if ((ConflictObject[id].result[index] & CONFLICT_CONSUMED) == 0)
                    {
                        break;
                    }
                }
            }
        }
        /* k re-registers index as an int for this arm: byte-required
         * (indexing with the s16 param recolors the subu; measured). */
        k = index;
        if (k < ConflictObjects)
        {
            if (ConflictObject[idx].result[k] != 0)
            {
                ConflictObject[idx].result[k] |= CONFLICT_CONSUMED;
                ConflictModel = ConflictObject[k].model;
                ConflictDistance.vx = (short)ConflictObject[k].position.vx - (short)ConflictObject[idx].position.vx;
                ConflictDistance.vy = (short)ConflictObject[k].position.vy - (short)ConflictObject[idx].position.vy;
                ConflictDistance.vz = (short)ConflictObject[k].position.vz - (short)ConflictObject[idx].position.vz;
                return index;
            }
        }
        goto ret_m1;
    }
    return CONFLICT_NONE;
}
