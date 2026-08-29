#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "humanoid.h"
#include "game_globals.h"
#include "item.h"
#include "padcmd.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3area(void);
 *     THINK_3.C:128, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     reg   $s0       short pad
 *     reg   $s2       long xx
 *     reg   $s1       long zz
 *     reg   $s3       long dist
 *     reg   $s2       long vx
 *     reg   $s1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 * END PSX.SYM */

/*
 * Think3area chooses controls while an enemy approaches its assigned area
 * point.  Weapon class 3 delegates to Think3attack; other classes either run
 * the active AttackFunc callback or steer toward point[0]/point[1].
 *
 * Matching notes:
 *  - The already-acting callback arm is written before the longer steering
 *    arm.  This matches retail's physical fallthrough layout, the same shape
 *    used by Think3hitaway.
 *  - This THINK_3.C caller sees turn_towards_player_ returning s16.  Declaring
 *    it s32 adds a deferred result copy and makes the function one instruction
 *    long; the original-width prototype keeps the return in $v0 until reorg
 *    moves it into `pad` in the following Attrib branch's delay slot.
 *  - `__builtin_abs(Degree)` gives the target's two-pseudo abssi2 expansion:
 *    the raw signed Degree remains in $v0 while the absolute result occupies
 *    $v1.  Mutating one `degree` local in place leaves a seven-byte residual.
 */

extern Humanoid *Me_THINK_C;

extern s16 SuccessionAttack(s32 dist, s16 deg);
extern s16 Think3attack(void);
extern s16 turn_towards_player_(s32 x_diff, s32 z_diff);

s16 Think3area(void)
{
    s16 pad;
    s32 xx;
    s32 zz;
    s32 dist;

    pad = 0;
    if (Me_THINK_C->status == STAT_ATTACK)
    {
        return SuccessionAttack(4000, 500);
    }

    if (Distance < SR_CLEAR_RANGE && SR != SR_GONE)
    {
        SR = SR_NONE;
    }

    if (WPATK_CLASS(Me_THINK_C->wpatk) == WPATK_CLASS_RANGED)
    {
        return Think3attack();
    }

    xx = Me_THINK_C->point[0] - Me_THINK_C->locate->vx;
    zz = Me_THINK_C->point[1] - Me_THINK_C->locate->vz;
    dist = SquareRoot0(xx * xx + zz * zz);

    if (Me_THINK_C->actflg != 0)
    {
        pad = AttackFunc[WPATK_CLASS(Me_THINK_C->wpatk)]();
        if (Distance < 4000)
        {
            Me_THINK_C->actcnt++;
            if (Me_THINK_C->actcnt == 0 && dist > 4000)
            {
                Me_THINK_C->actflg = 0;
            }
        }
        goto return_pad;
    }

    if ((ATTRIB_BITS & ATTR_HIT) != 0)
    {
        Me_THINK_C->actflg = 1;
    }

    if (dist < 2000)
    {
        s32 degree;

        if ((Me_THINK_C->motion->count & 7) != 0)
        {
            pad = Me_THINK_C->pad.data;
        }
        else if (Degree > 500)
        {
            pad = PADLright;
        }
        else if (Degree < -500)
        {
            pad = PADLleft;
        }
        else if (rand() % 10 == 0)
        {
            SetNowMotion(Me_THINK_C, 0x713, 1); /* taunt */
            Me_THINK_C->actflg = 1;
        }

        if (Distance >= 4000)
        {
            goto return_pad;
        }

        degree = __builtin_abs(Degree);

        if (degree < 100)
        {
            pad = SetCommand(&Me_THINK_C->pad, CMD_LUNGE);
        }
        else if (degree < 1000)
        {
            pad |= PADRleft;
        }
        Me_THINK_C->actflg = 1;
        goto return_pad;
    }

    pad = turn_towards_player_(xx, zz);
    if ((ATTRIB_BITS & ATTR_WALL) != 0)
    {
        Me_THINK_C->actflg = 1;
    }
    if (Me_THINK_C->motion->count != 0)
    {
        goto return_pad;
    }
    if (Distance >= 3000)
    {
        goto return_pad;
    }
    {
        s32 degree;

        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }
        if (degree < 1000)
        {
            pad |= PADRleft;
        }
    }

return_pad:
    return pad;
}
