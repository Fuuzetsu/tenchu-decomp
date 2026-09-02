#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupSpline(struct MotionManager *mmp);
 *     ACTION.C:344, 17 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct MotionManager * mmp
 * END PSX.SYM */

extern void UpdateSplineControl(SplineControlType *spc);

void SetupSpline(MotionManager *mmp)
{
    short time;
    short i;
    SplineControlType *spc;

    time = mmp->motion->time;
    spc = mmp->control;
    spc->key0 = mmp->motion->locate;
    spc->dd0.pad = time;
    if (time != 0)
    {
        spc->key1 = spc->key0 + 1;
        UpdateSplineControl(spc);
    }
    for (i = 0; i < mmp->n; i++)
    {
        spc = &mmp->control[i + 1];
        spc->key0 = mmp->motion->rotate[i];
        spc->dd0.pad = time;
        if (time != 0)
        {
            spc->key1 = spc->key0 + 1;
            UpdateSplineControl(spc);
        }
    }
}
