#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void MoveHumanoid(struct Humanoid *human, short ordr, short side);
 *     HUMAN.C:349, 17 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short ordr
 *     param $a2       short side
 * END PSX.SYM */

void MoveHumanoid(Humanoid *human, short ordr, short side)
{
    int sine, cosine;
    int order_value;
    short order_speed, side_speed;

    order_value = ordr;
    order_speed = ordr;
    side_speed = side;
    if (order_value != 0 || side != 0)
    {
        sine = -rsin(human->rotate->vy);
        cosine = -rcos(human->rotate->vy);
        /* Sign-extend only when the upper bits are clear; callers may pass wider values. */
        if ((order_value & MOTION_BYTE_UPPER_MASK) == MOTION_BYTE_SIGN_BIT)
        {
            order_speed = ordr - MOTION_BYTE_RANGE;
        }
        if ((side & MOTION_BYTE_UPPER_MASK) == MOTION_BYTE_SIGN_BIT)
        {
            side_speed = side - MOTION_BYTE_RANGE;
        }
        human->vector.vx = (short)(((short)sine * order_speed -
                                    (short)cosine * side_speed) >> FIXED_SHIFT);
        human->vector.vz = (short)(((short)cosine * order_speed +
                                    (short)sine * side_speed) >> FIXED_SHIFT);
    }
    else
    {
        human->vector.vz = 0;
        human->vector.vx = 0;
    }
}
