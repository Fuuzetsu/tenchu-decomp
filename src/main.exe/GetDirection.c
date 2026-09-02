#include "common.h"
#include "main.exe.h"
#include "humanoid.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetDirection(long dx, long dz, short roty);
 *     HUMAN.C:381, 9 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long dx
 *     param $a1       long dz
 *     param $a2       short roty
 * END PSX.SYM */

s16 GetDirection(s32 dx, s32 dz, s32 roty)
{
    s32 diff;
    s16 sdiff;
    s16 result;

    diff = ratan2(-dx, -dz) - roty;
    sdiff = diff;
    result = diff;
    if (sdiff > ANGLE_HALF)
    {
        result = ANGLE_FULL - diff;
    }
    else if (sdiff <= -ANGLE_HALF)
    {
        result = diff + ANGLE_FULL;
    }
    return result;
}
