#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short UpdateMotion(struct MotionManager *mmp, short mid);
 *     ACTION.C:182, 34 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct MotionManager * mmp
 *     param $t0       short mid
 *     reg   $a0       struct MotionRegistType * mrp
 *     reg   $a2       short i
 *     reg   $a1       short j
 *     reg   $a3       short * xyz
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionRegistType MOTcommon[41];
 * END PSX.SYM */

extern void SetupSpline(MotionManager *mmp);

s16 UpdateMotion(MotionManager *mmp, motion_id mid)
{
    MotionRegistType *mrp;
    MotionDataType *md;
    s16 i;
    s16 j;
    s16 *xyz;
    s32 t;
    s16 sweep;

    if (mid == mmp->mid)
        return -1;

    mrp = mmp->motreg;
    i = 0;
    while (mrp[i].mid != mid)
    {
        if (mrp[i].mid == MOTION_ID_NONE)
            break;
        i++;
    }
    if (mrp[i].motion == 0)
    {
        mrp = MOTcommon;
        i = 0;
        while (mrp[i].mid != mid)
        {
            if (mrp[i].mid == MOTION_ID_NONE)
                break;
            i++;
        }
        if (mrp[i].motion == 0)
            return 0;
    }

    md = mrp[i].motion;
    mmp->mid = mid;
    mmp->motion = md;
    sweep = md->sweep;
    mmp->count = sweep;
    if (sweep & MOTION_BYTE_SIGN_BIT)
        mmp->count = sweep - MOTION_BYTE_RANGE;
    mmp->loop = 0;
    i = (mmp->motion->n < mmp->model->n) ? mmp->motion->n : mmp->model->n;
    mmp->n = i;
    SetupSpline(mmp);

    for (i = 0; i < mmp->model->n; i++)
    {
        xyz = &mmp->model->object[i]->rotate.vx;
        for (j = 0; j < 3; j++)
        {
            t = xyz[j];
            if (((t < 0) ? -t : t) > ANGLE_HALF)
                xyz[j] = (xyz[j] < 0) ? (t += ANGLE_FULL) : (t -= ANGLE_FULL);
            xyz[j] = xyz[j] % ANGLE_FULL;
        }
    }
    return 1;
}
