#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think4chase(void);
 *     THINK_4.C:75, 189 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a0       short pad
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
 * Turn briefly while no chase point exists, then abandon after 0x5b ticks.
 * During the first 0x1e ticks, override the default forward command with a
 * forward-right or forward-left command. With a chase point, steer toward
 * it and clear it after arriving or when actscnt wraps.
 *
 * As in Think4contact, the default pad value must be assigned before the two
 * comparisons and the nonzero outcomes expressed only as overrides. This
 * preserves the target's fallthrough bodies, explicit jumps, and inline
 * return-conversion delay slot.
 *
 * The first comparison is intentionally written `Degree > rotation_speed`.
 * Its mathematical inverse spelling emits the same slt but evaluates the
 * field load first; this spelling preserves the target's Degree-first load
 * order.
 */
extern s16 Think4abandon(void);

s16 Think4chase(void)
{
    s32 pad;

    if (SR == SR_SEEN)
    {
        Attrib = (Attrib & (u16)~ATTR_PHASE) | PHASE_ALERT;
        return 0;
    }

    if (Me_THINK_C->chase[HUMANOID_CHASE_X] == 0 && Me_THINK_C->chase[HUMANOID_CHASE_Z] == 0)
    {
        if (Me_THINK_C->actcnt >= 0x5B)
        {
            return Think4abandon();
        }
        else
        {
            Me_THINK_C->actcnt++;
            pad = PADLup;
            if (Me_THINK_C->actcnt < 30)
            {
                if (Degree > Me_THINK_C->turn)
                {
                    pad = PADLup | PADLright;
                }
                else if (Degree < -Me_THINK_C->turn)
                {
                    /* The signed view keeps this combination in addiu's
                     * immediate range. */
                    pad = (s16)(PADLleft | PADLup);
                }
            }
        }
    }
    else
    {
        s32 dx, dz;

        Me_THINK_C->actscnt++;
        dx = Me_THINK_C->chase[HUMANOID_CHASE_X] - Me_THINK_C->locate->vx;
        dz = Me_THINK_C->chase[HUMANOID_CHASE_Z] - Me_THINK_C->locate->vz;
        pad = turn_towards_player_(dx, dz);
        if (SquareRoot0(dx * dx + dz * dz) < 1000 || Me_THINK_C->actscnt == 0)
        {
            Me_THINK_C->chase[HUMANOID_CHASE_Z] = 0;
            Me_THINK_C->chase[HUMANOID_CHASE_X] = 0;
            Me_THINK_C->actcnt = 0;
        }
    }
    return pad;
}
