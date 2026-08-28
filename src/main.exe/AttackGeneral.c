#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "item.h"
#include "padcmd.h"
#include "humanoid.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short AttackGeneral(void);
 *     THINK_3.C:368, 78 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern struct BattleType BattleDB[78];
 *     extern long Distance;
 *     extern short Degree;
 *     extern short EngageLevel;
 *     extern short Attrib;
 *     extern long GameClock;
 *     extern long AttackActionCount;
 * END PSX.SYM */

/*
 * AttackGeneral (0x8002e39c, 0x5a4 bytes) -- general-purpose humanoid
 * attack chooser.  The status-7 continuation gate shares AttackIndirect's
 * one-shot loop fence; the ordinary path chooses chase, turn, item, and
 * SetCommand actions from distance, facing, and EngageLevel rolls.
 *
 * Matching notes:
 *  - GameClock must use its original scalar declaration.  The equivalent
 *    unknown-size-array declaration lets delay-slot filling hoist its `lui`
 *    across the modulo guard and makes the function one instruction short.
 *  - Spell the time guard `GameClock > AttackActionCount`: comparison
 *    operand evaluation order puts the absolute GameClock load before the
 *    gp-relative action-count load, as in the target.
 *  - The cold close-range `% 4` switch needs an explicit default return.
 *    Otherwise its fallthrough becomes another predecessor of the normal
 *    range label, so CSE cannot carry the pre-switch Distance value and
 *    reloads it.  Explicit close/normal/near labels preserve the target's
 *    cold-block placement and shared SetCommand tail without any assembly.
 */

extern Humanoid *Me_THINK_C;
extern s32 AttackActionCount;

extern s16 ChasetoTarget(s32 distance);
extern s16 ItemUse(void);

short AttackGeneral(void)
{
    s16 pad;
    s16 status7_result;

    pad = 0;
    if (Me_THINK_C->status == STAT_ATTACK)
    {
        s32 status_degree;

        do
        {
            if (Me_THINK_C->motion->count !=
                BattleDB[Me_THINK_C->warid].contfrm)
            {
                status7_result = 0;
                goto status7_return;
            }
            if (Distance < 2000)
            {
                status_degree = Degree;
                if (status_degree < 0)
                {
                    status_degree = -status_degree;
                }
                if (status_degree < 500)
                {
                    goto choose_status7;
                }
            }
            if (rand() % (EngageLevel + 1) != 0)
            {
                status7_result = pad;
                goto status7_return;
            }
        } while (0);

    choose_status7:
        if (Degree >= 301)
        {
            pad = PADLright;
        }
        else
        {
            pad |= PADRleft;
            if (Degree < -300)
            {
                pad = PADLleft;
            }
            else
            {
                goto status7_value;
            }
        }
        pad |= PADRleft;

    status7_value:
        status7_result = pad;
    status7_return:
        return status7_result;
    }

    if (Me_THINK_C->status == STAT_JUMP)
    {
        return 0;
    }

    if (Me_THINK_C->motion->mid == MOT_ENGAGE)
    {
        return (u16)(rand() % (EngageLevel + 1) != 0) << 14;
    }

    if (Me_THINK_C->actmode == 0)
    {
        s32 chase_degree;

        pad = ChasetoTarget(3000);
        if (pad == 0 || (ATTRIB_BITS & ATTR_HIT) != 0)
        {
            Me_THINK_C->actmode = 1;
        }
        if (Distance < 5001)
        {
            goto return_pad;
        }
        chase_degree = Degree;
        if (chase_degree < 0)
        {
            chase_degree = -chase_degree;
        }
        if (chase_degree >= 100)
        {
            goto return_pad;
        }
        if (rand() % 5 != 0)
        {
            goto return_pad;
        }
        pad = PADLup | PADRdown;
        goto return_pad;
    }

    if ((Me_THINK_C->motion->count & 0xf) != 0)
    {
        s32 raw_degree;
        s32 motion_degree;

        pad = Me_THINK_C->pad.data;
        if (Distance >= 2000)
        {
            goto return_pad;
        }
        raw_degree = Degree;
        motion_degree = (raw_degree >= 0) ? raw_degree : -raw_degree;
        if (motion_degree < 1000)
        {
            pad = PADLdown;
            goto return_pad;
        }
        if (motion_degree < 1501)
        {
            goto return_pad;
        }
        pad = PADLup;
        goto return_pad;
    }

    if (Distance >= 5001)
    {
        Humanoid *me;

        Me_THINK_C->actmode = 0;
        me = Me_THINK_C;
        Me_THINK_C->chase[1] = 0;
        me->chase[0] = 0;
        ItemUse();
        return 0;
    }

    if ((ATTRIB_BITS & ATTR_WALL) != 0)
    {
        Me_THINK_C->actmode = 0;
    }

    if (Degree >= 501)
    {
        pad = PADLright;
    }
    else if (Degree < -500)
    {
        pad = PADLleft;
    }

    if ((u32)(Distance - 1001) < 1999)
    {
        s32 attack_degree;

        do
        {
            attack_degree = Degree;
            if (attack_degree < 0)
            {
                attack_degree = -attack_degree;
            }
            if (attack_degree >= 1200)
            {
                break;
            }
            if (rand() % (EngageLevel + 1) != 0)
            {
                break;
            }
            if (GameClock > AttackActionCount)
            {
                AttackActionCount = GameClock + EngageLevel * 10;
                if (rand() % 3 == 0)
                {
                    pad = PADLdown;
                }
                return pad | PADRleft;
            }
        } while (0);
    }

    {
        s32 degree;

        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }

        if (degree >= 1001 || Distance < 2000)
        {
            if (Distance < 1000)
            {
                switch (rand() % 4)
                {
                case 0:
                    pad = PADLdown | PADRdown;
                    goto return_pad;
                case 1:
                    pad = PADRleft | PADRright;
                    goto return_pad;
                case 2:
                    pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_BACKWARD);
                    goto return_pad;
                case 3:
                    pad |= PADRleft;
                    goto return_pad;
                default:
                    goto return_pad;
                }
            }
            pad |= PADLdown;
            goto return_pad;
        }

        if (Distance >= 3001)
        {
            pad |= PADLup;
            if (Distance < 4001)
            {
                goto return_pad;
            }
            if ((rand() & 1) != 0)
            {
                pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_FORWARD);
                goto return_pad;
            }

            degree = Degree;
            if (degree < 0)
            {
                degree = -degree;
            }
            if (degree < 500)
            {
                pad = SetCommand(&Me_THINK_C->pad, CMD_LUNGE);
                goto return_pad;
            }
            ItemUse();
            goto return_pad;
        }

        if ((rand() & 1) == 0)
        {
            goto return_pad;
        }
        if (Degree >= 101)
        {
            pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_RIGHT);
            goto return_pad;
        }
        if (Degree < -100)
        {
            pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_LEFT);
            goto return_pad;
        }
        goto return_pad;
    }

return_pad:
    return pad;
}
