#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SweepMotion(struct MotionManager *mmp);
 *     ACTION.C:272, 31 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $s3       struct MotionManager * mmp
 *     reg   $s2       struct MotionDataType * mot
 *     reg   $a2       struct ModelType * object
 *     reg   $s1       short i
 * END PSX.SYM */

/*
 * SweepMotion (0x8001ba24, 0x3d8 bytes) — advance the motion count while
 * interpolating each enabled model object toward its current keyframe. Bone
 * zero also sweeps the root translation; the remaining enabled bones update
 * rotation only.
 *
 * Matching notes: the narrow postincrement expression is load-bearing:
 * `count = -mmp->count++` preserves the old unsigned halfword for the
 * increment/store, then negates that old value into the shared signed-short
 * divisor. The direct compound assignments deliberately re-read each narrow
 * rotation field after division, matching cc1's natural HImode update shape.
 * Variable division requires maspsx `--expand-div` for all nine PsyQ guard
 * sequences.
 */

short SweepMotion(MotionManager *mmp)
{
    MotionDataType *mot;
    ModelType *object;
    short i;
    short count;

    count = -mmp->count++;
    mot = mmp->motion;

    if (mmp->mask & 1) {
        object = *mmp->model->object;
        object->locate.coord.t[0] +=
            (mot->locate->x - object->locate.coord.t[0]) / count;
        object->locate.coord.t[2] +=
            (mot->locate->z - object->locate.coord.t[2]) / count;
        object->locate.coord.t[1] +=
            (((s32)mmp->model->rotate.pad * mot->locate->y >> 12) -
             object->locate.coord.t[1]) /
            count;
        object->rotate.vx +=
            (mot->rotate[0]->x - object->rotate.vx) / count;
        object->rotate.vy +=
            (mot->rotate[0]->y - object->rotate.vy) / count;
        object->rotate.vz +=
            (mot->rotate[0]->z - object->rotate.vz) / count;
        UpdateCoordinate(object);
    }

    for (i = 1; i < mmp->n; i++) {
        if ((mmp->mask >> i) & 1) {
            object = mmp->model->object[i];
            object->rotate.vx +=
                (mot->rotate[i]->x - object->rotate.vx) / count;
            object->rotate.vy +=
                (mot->rotate[i]->y - object->rotate.vy) / count;
            object->rotate.vz +=
                (mot->rotate[i]->z - object->rotate.vz) / count;
            UpdateCoordinate(object);
        }
    }

    return mmp->count;
}
