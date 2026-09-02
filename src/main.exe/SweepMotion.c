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
            (mot->locate->x - object->locate.coord.t[0]) / count;
        object->locate.coord.t[2] +=
            (mot->locate->z - object->locate.coord.t[2]) / count;
        object->locate.coord.t[1] +=
            (((s32)mmp->model->rotate.pad * mot->locate->y >>
               FIXED_SHIFT) -
             object->locate.coord.t[1]) /
            count;
        object->rotate.vx +=
            (mot->rotate[MODEL_PART_WAIST]->x - object->rotate.vx) /
            count;
        object->rotate.vy +=
            (mot->rotate[MODEL_PART_WAIST]->y - object->rotate.vy) /
            count;
        object->rotate.vz +=
            (mot->rotate[MODEL_PART_WAIST]->z - object->rotate.vz) /
            count;
        UpdateCoordinate(object);
    }

    for (i = MODEL_PART_TORSO; i < mmp->n; i++)
    {
        if (MOTION_PART_ENABLED(mmp->mask, i))
        {
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
