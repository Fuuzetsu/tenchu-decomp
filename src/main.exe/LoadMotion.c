#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct MotionPackType * LoadMotion(unsigned long *data);
 *     ACTION.C:77, 19 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long * data
 *     reg   $s0       struct MotionPackType * mpd
 *     reg   $a1       struct MotionDataType * mmp
 *     reg   $a3       short i
 *     reg   $a2       short j
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionPackType *MotionPack;
 * END PSX.SYM */

extern char msg_no_motion_data[]; /* NO MOTION DATA */

MotionPackType *LoadMotion(unsigned long *data)
{
    MotionPackType *mpd;
    MotionDataType *mmp;
    short i;
    short j;

    mpd = (MotionPackType *)data;
    if (mpd == 0)
    {
        SystemOut(msg_no_motion_data);
    }
    for (i = 0; i < mpd->n; i++)
    {
        mpd->motion[i] =
            (MotionDataType *)((s32)mpd->motion[i] + (s32)mpd);
        mmp = mpd->motion[i];
        if (mmp->n != 0)
        {
            mmp->locate =
                (MotionElementType *)((s32)mmp->locate + (s32)mmp);
            for (j = 0; j < mmp->n; j++)
            {
                mmp->rotate[j] =
                    (MotionElementType *)((s32)mmp->rotate[j] +
                                          (s32)mmp);
            }
        }
    }
    MotionPack = mpd;
    return mpd;
}
