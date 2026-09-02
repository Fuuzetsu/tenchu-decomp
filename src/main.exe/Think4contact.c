#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think4contact(void);
 *     THINK_4.C:36, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a1       short pad
 *     reg   $s1       long xx
 *     reg   $s2       long zz
 *     reg   $s1       long vx
 *     reg   $s2       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short SR;
 *     extern short Attrib;
 *     extern short Degree;
 * END PSX.SYM */

/*
 * On clear sight escalate to PHASE_ALERT, turn briefly while no chase point
 * exists, then abandon when the investigation wait expires. With a chase
 * point, steer toward it and clear it after arriving or when actscnt wraps.
 *
 * The default pad assignment belongs before the two turn comparisons.
 * Besides expressing the three-way choice directly, it gives cc1 the
 * target's zero-valued delay-slot move and keeps the two nonzero outcomes
 * as fallthrough bodies with explicit jumps to the shared return conversion.
 */
extern s16 Think4abandon(void);

s16 Think4contact(void)
{
    s32 pad;

    if (SR == SR_SEEN)
    {
        Attrib = (Attrib & (u16)~ATTR_PHASE) | PHASE_ALERT;
        return 0;
    }

    if (Me_THINK_C->chase.point[HUMANOID_CHASE_X] == 0 && Me_THINK_C->chase.point[HUMANOID_CHASE_Z] == 0)
    {
        if (Me_THINK_C->actcnt >= THINK4_ABANDON_TICKS)
        {
            return Think4abandon();
        }
        else
        {
            Me_THINK_C->actcnt++;
            pad = 0;
            if (Me_THINK_C->turn < Degree)
            {
                pad = PADLright;
            }
            else if (Degree < -Me_THINK_C->turn)
            {
                pad = -PADLleft;
            }
        }
    }
    else
    {
        s32 dx, dz;

        Me_THINK_C->actscnt++;
        dx = Me_THINK_C->chase.point[HUMANOID_CHASE_X] - Me_THINK_C->locate->vx;
        dz = Me_THINK_C->chase.point[HUMANOID_CHASE_Z] - Me_THINK_C->locate->vz;
        pad = GotoPosition(dx, dz);
        if (SquareRoot0(dx * dx + dz * dz) < THINK4_ARRIVAL_DISTANCE ||
            Me_THINK_C->actscnt == 0)
        {
            Me_THINK_C->chase.point[HUMANOID_CHASE_Z] = 0;
            Me_THINK_C->chase.point[HUMANOID_CHASE_X] = 0;
            Me_THINK_C->actcnt = 0;
        }
    }
    return pad;
}
