#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetMotionID(struct MotionManager *mmp, short mid);
 *     ACTION.C:169, 9 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct MotionManager * mmp
 *     param $a1       short mid
 * END PSX.SYM */

short GetMotionID(MotionManager *mmp, motion_id mid)
{
    MotionRegistType *registrations;
    s16 i;

    registrations = mmp->motreg;
    i = 0;
    while (registrations[i].mid != MOTION_ID_NONE)
    {
        if (registrations[i].mid == mid)
        {
            break;
        }
        i++;
    }
    return registrations[i].id;
}
