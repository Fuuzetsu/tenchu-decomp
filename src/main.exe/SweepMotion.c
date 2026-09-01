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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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

    if (mmp->mask & MOTION_MASK_ROOT)
    {
        object = *mmp->model->object;
        object->locate.coord.t[0] +=
            (mot->locate.keyframes->x - object->locate.coord.t[0]) / count;
        object->locate.coord.t[2] +=
            (mot->locate.keyframes->z - object->locate.coord.t[2]) / count;
        object->locate.coord.t[1] +=
            (((s32)mmp->model->rotate.pad * mot->locate.keyframes->y >> 12) -
             object->locate.coord.t[1]) /
            count;
        object->rotate.vx +=
            (mot->rotate[0].keyframes->x - object->rotate.vx) / count;
        object->rotate.vy +=
            (mot->rotate[0].keyframes->y - object->rotate.vy) / count;
        object->rotate.vz +=
            (mot->rotate[0].keyframes->z - object->rotate.vz) / count;
        UpdateCoordinate(object);
    }

    for (i = 1; i < mmp->n; i++)
    {
        if (MOTION_PART_ENABLED(mmp->mask, i))
        {
            object = mmp->model->object[i];
            object->rotate.vx +=
                (mot->rotate[i].keyframes->x - object->rotate.vx) / count;
            object->rotate.vy +=
                (mot->rotate[i].keyframes->y - object->rotate.vy) / count;
            object->rotate.vz +=
                (mot->rotate[i].keyframes->z - object->rotate.vz) / count;
            UpdateCoordinate(object);
        }
    }

    return mmp->count;
}
