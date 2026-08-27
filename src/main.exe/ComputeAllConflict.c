#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ComputeAllConflict(void);
 *     CONFLICT.C:329, 50 src lines, frame 72 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     reg   $s1       struct ConflictObjectType * confop
 *     reg   $s0       struct ModelType * model
 *     stack sp+16     struct MATRIX mat
 *     reg   $s2       short i
 *     reg   $t0       short j
 *
 * Globals it touches, as the original declared them:
 *     extern short ConflictObjects;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct ModelType World;
 * END PSX.SYM */

/* The conflict pool + live count (Ghidra: ConflictObject / ConflictObjects). */


extern void *memset(void *s, int c, u32 n);

/*
 * ComputeAllConflict (0x8001a688) — same TU as InsertConflict.c/DeleteConflict.c/
 * GetConflictResult.c: runs the conflict pool's per-frame update, once per model
 * flagged "active" (attribute bit 0x4000). Pass 1 refreshes each active slot's
 * world-space `position` from its model (either directly, when the model's
 * coordinate hierarchy root IS World, or via GsGetLw/GsSetLsMatrix/RotTrans
 * otherwise) and clears its `result[]` row and `offset.pad` hit counter. Pass 2
 * is the O(n^2) AABB overlap test (y, then z, then x — that axis order matches
 * the target) between every distinct pair of active slots; a hit stamps both
 * slots' `result[]` (the OTHER slot's `size.pad` byte, tagged with 0x80),
 * flags both models' attribute bit 0x8000, and bumps both `offset.pad` counters.
 *
 * Matching notes:
 *  - `confop` and `model` (real PSX.SYM locals) belong to pass 1. Pass 2
 *    deliberately keeps `ConflictObject[i]` as direct indexed expressions;
 *    cc1 then builds the outer pointer in `$a3`, preserves both its byte
 *    offset and `i` for the result writes, and copies the pointer to `$a2`
 *    before the inner loop. Reusing the pass-1 locals here incorrectly kept
 *    them in `$s1`/`$s0` and removed that target pointer copy.
 *  - `other` (`&ConflictObject[j]`) is a block-local pointer not recorded in
 *    the demo local list. It gives the target's `$a1` pointer reuse across
 *    the inner loop's position, size, model, and counter accesses.
 *  - Each axis is one direct `__builtin_abs(other position - outer position)`
 *    expression. The opaque `abssi2` RTL lets cc1 schedule both independent
 *    size loads around the subtraction before expanding the target's
 *    `bgez; nop; negu`, eliminating the two load-delay stalls produced by a
 *    separate source-level sign fix. The commutative sum is written outer
 *    size first so the two `lh` destinations also match.
 *  - The two `result[]` writes retain their direct `ConflictObject[i/j]`
 *    indexing. The target recomputes `base + i*0x78 + j` and the symmetric
 *    address instead of shortening either store through the cached pointers.
 *  - `model->locate.super == &World.locate`: locate (GsCOORDINATE2) is
 *    ModelType's own first field, so `&model->locate` is model itself (see
 *    GetAbsolutePosition.c's identical `GsGetLw(&model->locate, &m)` idiom).
 *  - Both loops are natural `for (i = 0; i < ConflictObjects; i++)` — cc1's
 *    duplicate_loop_exit_test alone produces the guarded do-while target shape
 *    (DeleteConflict.c/GetConflictResult.c precedent), no explicit redundant
 *    guard needed here.
 *
 * STATUS: MATCHED — exact 816 bytes / 204 instructions.
 */
void ComputeAllConflict(void)
{
    short i;
    short j;
    ModelType *model;
    ConflictObjectType *confop;
    MATRIX mat;
    int d;

    for (i = 0; i < ConflictObjects; i++)
    {
        confop = &ConflictObject[i];
        model = confop->model;
        if (model->attribute & 0x4000)
        {
            memset(confop->result, 0, 0x50);
            confop->offset.pad = 0;
            model->attribute = model->attribute & 0x7fff;
            if (model->locate.super == &World.locate)
            {
                confop->position.vx = model->locate.coord.t[0] + confop->offset.vx;
                confop->position.vy = model->locate.coord.t[1] + confop->offset.vy;
                confop->position.vz = model->locate.coord.t[2] + confop->offset.vz;
            }
            else
            {
                GsGetLw(&model->locate, &mat);
                GsSetLsMatrix(&mat);
                RotTrans(&confop->offset, &confop->position, (long *)0);
            }
        }
    }

    for (i = 0; i < ConflictObjects; i++)
    {
        if (ConflictObject[i].model->attribute & 0x4000)
        {
            for (j = i + 1; j < ConflictObjects; j++)
            {
                ConflictObjectType *other = &ConflictObject[j];

                if (other->model->attribute & 0x4000)
                {
                    d = __builtin_abs(other->position.vy - ConflictObject[i].position.vy);
                    if (d <= ConflictObject[i].size.vy + other->size.vy)
                    {
                        d = __builtin_abs(other->position.vz - ConflictObject[i].position.vz);
                        if (d <= ConflictObject[i].size.vz + other->size.vz)
                        {
                            d = __builtin_abs(other->position.vx - ConflictObject[i].position.vx);
                            if (d <= ConflictObject[i].size.vx + other->size.vx)
                            {
                                ConflictObject[i].result[j] = other->size.pad | 0x80;
                                ConflictObject[j].result[i] = ConflictObject[i].size.pad | 0x80;
                                ConflictObject[i].model->attribute =
                                    ConflictObject[i].model->attribute | 0x8000;
                                other->model->attribute = other->model->attribute | 0x8000;
                                ConflictObject[i].offset.pad = ConflictObject[i].offset.pad + 1;
                                other->offset.pad = other->offset.pad + 1;
                            }
                        }
                    }
                }
            }
        }
    }
}
