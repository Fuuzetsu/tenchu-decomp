#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetTargetDistance(struct Humanoid *human, short *deg);
 *     HUMAN.C:394, 10 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short * deg
 * END PSX.SYM */

long GetTargetDistance(Humanoid *human, short *deg)
{
    s32 dx, dz;
    s32 angle;
    s32 vy;
    s32 diff;
    s16 deg2;

    dx = human->target->coord.t[0] - human->locate->vx;
    dz = human->target->coord.t[2] - human->locate->vz;
    vy = (u16)human->rotate->vy;
    angle = ratan2(-dx, -dz);
    diff = angle - vy;
    deg2 = (s16)diff;
    if (deg2 > ANGLE_HALF)
    {
        deg2 = ANGLE_FULL - deg2;
    }
    else if (deg2 <= -ANGLE_HALF)
    {
        deg2 += ANGLE_FULL;
    }
    *deg = deg2;
    return SquareRoot0(dx * dx + dz * dz);
}
