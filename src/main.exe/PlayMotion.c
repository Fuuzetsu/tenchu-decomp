#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short PlayMotion(struct MotionManager *mmp, short mode);
 *     ACTION.C:220, 17 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct MotionManager * mmp
 *     param $a1       short mode
 * END PSX.SYM */

extern short SweepMotion(MotionManager *mmp);

short PlayMotion(MotionManager *mmp, short mode)
{
    short result;

    if (mmp->loop < 0)
    {
        return 0;
    }
    if (mode != 0)
    {
        if (mmp->count < 0)
        {
            SweepMotion(mmp);
            goto done;
        }
        result = ActiveMotion(mmp);
        if (result != 0)
        {
            goto done;
        }
        result = mmp->loop;
    }
    else
    {
        result = mmp->count + 1;
        mmp->count = result;
        if (result <= mmp->motion->time)
        {
            goto done;
        }
        result = mmp->loop;
        mmp->count = 0;
    }
    mmp->loop = result + 1;
done:
    return mmp->count;
}
