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
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 * Clear the interrupted state, turn briefly while no chase point exists,
 * then abandon after 0x5b ticks. With a chase point, steer toward it and
 * clear it after arriving or when actscnt wraps.
 *
 * The default result assignment belongs before the two turn comparisons.
 * Besides expressing the three-way choice directly, it gives cc1 the
 * target's zero-valued delay-slot move and keeps the two nonzero outcomes
 * as fallthrough bodies with explicit jumps to the shared return conversion.
 */
extern s16 Think4abandon(void);

s16 Think4contact(void)
{
    s32 result;

    if (SR == 1)
    {
        Attrib = (Attrib & 0xFFFC) | 2;
        return 0;
    }

    if (Me_THINK_C->chase[0] == 0 && Me_THINK_C->chase[1] == 0)
    {
        if (0x5B <= Me_THINK_C->actcnt)
        {
            return Think4abandon();
        }
        else
        {
            Me_THINK_C->actcnt = Me_THINK_C->actcnt + 1;
            result = 0;
            if (Me_THINK_C->turn < Degree)
            {
                result = 0x2000;
            }
            else if (Degree < -Me_THINK_C->turn)
            {
                result = -0x8000;
            }
        }
    }
    else
    {
        s32 dx, dz;

        Me_THINK_C->actscnt = Me_THINK_C->actscnt + 1;
        dx = Me_THINK_C->chase[0] - Me_THINK_C->locate->vx;
        dz = Me_THINK_C->chase[1] - Me_THINK_C->locate->vz;
        result = turn_towards_player_(dx, dz);
        if (SquareRoot0(dx * dx + dz * dz) < 1000 || Me_THINK_C->actscnt == 0)
        {
            Me_THINK_C->chase[1] = 0;
            Me_THINK_C->chase[0] = 0;
            Me_THINK_C->actcnt = 0;
        }
    }
    return result;
}
