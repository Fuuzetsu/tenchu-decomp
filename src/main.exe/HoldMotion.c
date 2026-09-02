#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short HoldMotion(struct MotionManager *mmp);
 *     ACTION.C:242, 26 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct MotionManager * mmp
 *     reg   $s2       struct MotionDataType * mot
 *     reg   $a1       struct ModelType * object
 *     reg   $s0       short i
 * END PSX.SYM */

short HoldMotion(MotionManager *mmp)
{
    MotionDataType *mot;
    ModelType *object;
    short i;

    mot = mmp->motion;
    if (mmp->mask & MOTION_MASK_ROOT)
    {
        object = *mmp->model->object;
        object->locate.coord.t[0] = (s32)mot->locate->x;
        object->locate.coord.t[2] = (s32)mot->locate->z;
        object->locate.coord.t[1] =
            ((s32)mmp->model->rotate.pad *
             (s32)mot->locate->y) >>
            12;
    }
    for (i = 0; i < mmp->n; i++)
    {
        if (MOTION_PART_ENABLED(mmp->mask, i))
        {
            object = mmp->model->object[i];
            object->rotate.vx = mot->rotate[i]->x;
            object->rotate.vy = mot->rotate[i]->y;
            object->rotate.vz = mot->rotate[i]->z;
            UpdateCoordinate(object);
        }
    }
    mmp->loop = MOTION_LOOP_FROZEN;
    mmp->count = 0;
    return 0;
}
