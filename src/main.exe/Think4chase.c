#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think4chase(void);
 *     THINK_4.C:75, 189 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 * During the first 0x1e ticks, override the default 0x1000 command with a
 * directional 0x3000 or -0x7000 command. With a chase point, steer toward
 * it and clear it after arriving or when actscnt wraps.
 *
 * As in Think4contact, the default result must be assigned before the two
 * comparisons and the nonzero outcomes expressed only as overrides. This
 * preserves the target's fallthrough bodies, explicit jumps, and inline
 * return-conversion delay slot.
 *
 * The first comparison is intentionally written `Degree > rotation_speed`.
 * Its mathematical inverse spelling emits the same slt but evaluates the
 * field load first; this spelling preserves the target's Degree-first load
 * order.
 */
extern s16 SR;
extern s16 Attrib;
extern s16 Degree;
extern s16 Think4abandon(void);
extern s32 SquareRoot0(s32 x);

s16 Think4chase(void)
{
    s32 result;

    if (SR == 1)
    {
        Attrib = (Attrib & 0xFFFC) | 2;
        return 0;
    }

    if (Me_THINK_C->some_other_x_position == 0 && Me_THINK_C->some_other_z_position == 0)
    {
        if (0x5B <= Me_THINK_C->actcnt)
        {
            return Think4abandon();
        }
        else
        {
            Me_THINK_C->actcnt = Me_THINK_C->actcnt + 1;
            result = 0x1000;
            if (Me_THINK_C->actcnt < 0x1E)
            {
                if (Degree > Me_THINK_C->character_rotation_speed)
                {
                    result = 0x3000;
                }
                else if (Degree < -Me_THINK_C->character_rotation_speed)
                {
                    result = -0x7000;
                }
            }
        }
    }
    else
    {
        s32 dx, dz;

        Me_THINK_C->actscnt = Me_THINK_C->actscnt + 1;
        dx = Me_THINK_C->some_other_x_position - Me_THINK_C->some_kind_of_current_position->vx;
        dz = Me_THINK_C->some_other_z_position - Me_THINK_C->some_kind_of_current_position->vz;
        result = turn_towards_player_(dx, dz);
        if (SquareRoot0(dx * dx + dz * dz) < 1000 || Me_THINK_C->actscnt == 0)
        {
            Me_THINK_C->some_other_z_position = 0;
            Me_THINK_C->some_other_x_position = 0;
            Me_THINK_C->actcnt = 0;
        }
    }
    return result;
}
