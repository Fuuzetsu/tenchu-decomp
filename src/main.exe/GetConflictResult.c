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
 * STATUS: NON_MATCHING — 38 of 368 bytes differ (arithmetically correct, right
 * length). Three independent below-the-C-level residuals remain (a decomp-permuter
 * run improves its own metric but does not beat this in raw bytes):
 *   1. Guard target (~4B): the search loop is a `for` (needed so reorg duplicates
 *      the `index+1` increment into the two skip-branch delay slots — the
 *      cookbook "loop-internal skip branch ... needs a real for" rule). cc1's
 *      duplicate_loop_exit_test then aims the top guard at the loop EXIT
 *      (post-loop, .L…aa7c); the target aims it at the shared `return -1`
 *      (.L…a9e0). Recovering that needs an explicit `if (ConflictObjects < 1)`
 *      guard whose redundant for-copy cc1 elides — but cc1 reloads the gp
 *      `ConflictObjects` (no calls, yet it won't CSE across the loop), so both
 *      guards survive; forcing one value through a local `n` reallocates the
 *      whole loop (found→t0) and regresses hard (>200B). Structural, not source.
 *   2. offset.pad / found-extension order (~8B): in the loop `offset.pad < found`,
 *      the target schedules the `(short)found` sll before the `lh` of offset.pad;
 *      cc1 emits the load first. Pure sched1 tie (the store/load-vs-alu ready
 *      tiebreak), permuter-immune.
 *   3. model lw schedule (~26B): the target hoists `pool[k].model`'s `lw` into
 *      the vx load-delay (implying a source temp `m = pool[k].model; … =m;`); cc1
 *      here fills that slot with a nop and sinks the lw into the vy region. Adding
 *      the temp DOES hoist the lw, but flips the pool[k] base register v1→v0 and
 *      drops the return move out of the jr delay slot (length 91) — a net +7B.
 *      The two are mutually exclusive from C.
 *
 * Matching notes (docs/matching-cookbook.md; siblings DeleteConflict.c /
 * InsertConflict.c / ProcItemMakibishi.c define the conflict-TU conventions):
 *  - `model->id` is loaded TWICE, un-CSE'd (the DeleteConflict lhu-vs-lh split):
 *    `int iVar3 = model->id;` (lh — the `== -1` guard and the loop's scan base
 *    id*0x78) and `short sVar1 = model->id;` (lhu, narrowing — the post-loop
 *    result/position base). Different machine modes don't CSE. iVar3's scan base
 *    dies once its id*0x78 invariant is hoisted, so it lands in caller-saved $v1
 *    and the post-loop index gets its own `int k`.
 *  - `if (index < 0)` is `sll a1,16; bgez` (short sign test); `found = 0;` sits
 *    before it so reorg fills the bgez delay slot, and the for-init `index = 0`
 *    cse-copies the zero (`move a2,a3`).
 *  - The `return -1`s: the `id == -1` case is a distinct `return -1` that reuses
 *    the compare's `v0 = -1` (a bare `jr;nop` at the tail); every other failure
 *    path shares one early `ret_m1:` block (label INSIDE the attribute guard, so
 *    the inverted `(attr & 0x4000) == 0` places it first) reached by `goto`.
 *  - ConflictDistance is one SVECTOR whose three fields splat auto-named as
 *    SEPARATE, ADDRESS-DRIFTED D_80097EC8/ECC + D_80097ED0 glabels (8 bytes low
 *    in the .map — see the fresh correct-address binds in config/symbols).
 *    `(short)position.vx` reads the low half `lhu` (narrowing into the s16 delta).
 *  - `do { vz…; } while (0);` is a scheduler-barrier lever (permuter-found): it
 *    keeps the model `sw`/return `move` out of the vz region so the return move
 *    lands in the jr delay slot and the function is the right length (92 insns);
 *    without it the tail is one insn short.
 */

/* Conflict slot (Ghidra: ConflictObjectType, 0x78 bytes; see DeleteConflict.c). */
typedef struct
{
    ModelType *model;            /* 0x00 */
    VECTOR position;             /* 0x04 */
    SVECTOR offset;              /* 0x14 */
    SVECTOR size;                /* 0x1C */
    void *common;                /* 0x24 */
    u8 result[64];               /* 0x28 */
    u8 pad[0x10];                /* 0x68 */
} ConflictObjectType;            /* 0x78 */

/* The conflict pool + live count (Ghidra: ConflictObject / ConflictObjects). */
extern ConflictObjectType ConflictObject[];
extern s16 ConflictObjects;

/* Output globals (Ghidra names, bound to correct addresses in config/symbols
 * because the splat-auto-named D_80097EC8/ECC/ED0 glabels are drifted 8B low). */
extern SVECTOR ConflictDistance; /* 0x80097EC8 (vx/vy/vz) */
extern ModelType *ConflictModel; /* 0x80097ED0 */

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/GetConflictResult", GetConflictResult);
#else
short GetConflictResult(ModelType *model, short index)
{
    short sVar1;
    int iVar3;
    short found;
    int k;

    iVar3 = model->id;
    sVar1 = model->id;
    if (iVar3 == -1)
    {
        return -1;
    }
    if ((model->attribute & 0x4000) == 0)
    {
    ret_m1:
        return -1;
    }
    found = 0;
    if (index < 0)
    {
        for (index = 0; index < ConflictObjects; index++)
        {
            if (ConflictObject[iVar3].result[index] != 0)
            {
                found++;
                if (ConflictObject[iVar3].offset.pad < found)
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
        if (ConflictObject[sVar1].result[k] != 0)
        {
            ConflictObject[sVar1].result[k] |= 0x40;
            ConflictDistance.vx = (short)ConflictObject[k].position.vx - (short)ConflictObject[sVar1].position.vx;
            ConflictModel = ConflictObject[k].model;
            ConflictDistance.vy = (short)ConflictObject[k].position.vy - (short)ConflictObject[sVar1].position.vy;
            do
            {
                ConflictDistance.vz = (short)ConflictObject[k].position.vz - (short)ConflictObject[sVar1].position.vz;
            } while (0);
            return index;
        }
    }
    goto ret_m1;
}
#endif
