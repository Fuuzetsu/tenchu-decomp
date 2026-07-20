#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1random(void);
 *     THINK_1.C:74, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *     reg   $a1       long xx
 *     reg   $v1       long zz
 *     reg   $s1       short pad
 *     reg   $a1       long vx
 *     reg   $v1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Attrib;
 * END PSX.SYM */

/*
 * Advance the random-walk counter. A new cycle picks a chase point near the
 * spawn point; later ticks either reset after reaching it or steer toward it
 * unless Attrib blocks the action.
 *
 * The direct vx/vz difference expressions are intentional. Splitting each
 * chase coordinate, locate coordinate, and result into extra scratch roles
 * changes global allocation and delay-slot scheduling. Keeping vx/vz as the
 * actual chase-minus-locate values yields the target load order, preserves
 * them in $a0/$a1 for turn_towards_player_, and leaves each absolute value
 * in a separate $v0 temporary.
 */
extern s16 Attrib;

s16 Think1random(void)
{
    u8 actcnt;
    s32 result;

    actcnt = Me_THINK_C->actcnt + 1;
    Me_THINK_C->actcnt = actcnt;
    result = 0;
    if (actcnt == 1)
    {
        Me_THINK_C->chase[0] = Me_THINK_C->point[0] + rand() % 10000 - 5000;
        Me_THINK_C->chase[1] = Me_THINK_C->point[1] + rand() % 10000 - 5000;
    }
    else
    {
        s32 vx, vz;
        s32 adx, adz;
        VECTOR *locate;

        locate = Me_THINK_C->locate;
        vx = Me_THINK_C->chase[0] - locate->vx;
        vz = Me_THINK_C->chase[1] - locate->vz;
        adx = (vx >= 0) ? vx : -vx;
        if (adx < 1000)
        {
            adz = (vz >= 0) ? vz : -vz;
            if (adz < 1000)
            {
                goto reset;
            }
        }
        if (Attrib & 0x400)
        {
        reset:
            Me_THINK_C->actcnt = 0;
        }
        else
        {
            result = turn_towards_player_(vx, vz);
        }
    }
    return result;
}
