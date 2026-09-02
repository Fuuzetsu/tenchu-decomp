#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ActiveMotion(struct MotionManager *mmp);
 *     ACTION.C:307, 31 src lines, frame 48 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct MotionManager * mmp
 *     reg   $s0       short i
 *     reg   $s3       short count
 *     reg   $s1       struct ModelType * object
 *     stack sp+16     struct SVECTOR vect
 * END PSX.SYM */

extern short HoldMotion(MotionManager *mmp);
extern void GetSpline(SVECTOR *vect, SplineControlType *spc, short cnt);

short ActiveMotion(MotionManager *mmp)
{
    short i;
    short count;
    short frame;
    ModelType *object;
    SVECTOR vect;

    if (mmp->motion->time == 0)
    {
        count = HoldMotion(mmp);
        return count;
    }
    frame = mmp->count;
    mmp->count = frame + 1;
    count = frame;
    if (mmp->mask & MOTION_MASK_ROOT)
    {
        object = *mmp->model->object;
        i = frame;
        GetSpline(&vect, mmp->control, i);
        object->locate.coord.t[0] = (s32)vect.vx;
        object->locate.coord.t[2] = (s32)vect.vz;
        object->locate.coord.t[1] =
            (s32)mmp->model->rotate.pad * (s32)vect.vy >> FIXED_SHIFT;
        GetSpline(&object->rotate, mmp->control + 1, i);
        UpdateCoordinate(object);
    }
    for (i = 1; i < mmp->n; i++)
    {
        if (MOTION_PART_ENABLED(mmp->mask, i))
        {
            object = mmp->model->object[i];
            GetSpline(&object->rotate, mmp->control + (i + 1), count);
            UpdateCoordinate(object);
        }
    }
    count = mmp->count;
    if (mmp->motion->time < count)
    {
        mmp->count = 0;
        return 0;
    }
    return count;
}
