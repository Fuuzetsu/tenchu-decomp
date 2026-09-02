#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct MotionDataType * SearchMotion(short id);
 *     ACTION.C:100, 22 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a1       short id
 *     reg   $a3       struct MotionPackType * mpd
 *     reg   $a2       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionPackType *CommonMotion;
 *     extern struct MotionPackType *PlayerMotion;
 *     extern struct MotionPackType *StageMotion;
 * END PSX.SYM */

MotionDataType *SearchMotion(short id)
{
    MotionPackType *mpd;
    short i;

    mpd = CommonMotion;
    if (mpd != 0)
    {
        for (i = 0; i < mpd->n; i++)
        {
            if (mpd->motion[i]->id == id)
            {
                return mpd->motion[i];
            }
        }
    }
    mpd = PlayerMotion;
    if (mpd != 0)
    {
        for (i = 0; i < mpd->n; i++)
        {
            if (mpd->motion[i]->id == id)
            {
                return mpd->motion[i];
            }
        }
    }
    mpd = StageMotion;
    if (mpd != 0)
    {
        for (i = 0; i < mpd->n; i++)
        {
            if (mpd->motion[i]->id == id)
            {
                return mpd->motion[i];
            }
        }
    }
    return 0;
}
