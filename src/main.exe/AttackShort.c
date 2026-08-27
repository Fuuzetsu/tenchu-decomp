#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "item.h"
#include "padcmd.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short AttackShort(void);
 *     THINK_3.C:266, 98 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 * Chooses a short-range humanoid attack.  Status 7 waits for the current
 * BattleDB continuation frame; the ordinary path handles chase, turn, item,
 * and SetCommand choices from distance, facing, and EngageLevel rolls.
 *
 * Matching notes:
 *  - The full-width `status_raw` producer is narrowed through `status_pad`,
 *    followed by an empty one-shot loop and identical full-width assignments.
 *    This zero-code boundary preserves the explicit zero-return island while
 *    making both result copies plain `move v0,s0` instructions.
 *  - Capturing `status_human` before the boundary keeps the existing humanoid
 *    pointer live across its loop notes.  Reading Me_THINK_C again afterwards
 *    introduces one extra load.
 *  - The separate SImode result carrier keeps the three status-7 edges joined
 *    at one shared sign-extension tail without narrowing either copy.
 */

extern Humanoid *Me_THINK_C;
extern s32 AttackActionCount;

extern s16 AttackAnimal(void);
extern s16 ChasetoTarget(s32 distance);
extern s16 ItemUse(void);

short AttackShort(void)
{
    MotionManager *motion;
    s16 pad;
    s32 status7_result;

    pad = 0;
    if ((Me_THINK_C->type & 0xf0) == PAGE_BEAST)
    {
        return AttackAnimal();
    }

    if (Me_THINK_C->status == STAT_ATTACK)
    {
        Humanoid *status_human;
        s16 status_pad;
        s32 status_raw;
        s32 status_degree;

        status_human = Me_THINK_C;
        status_raw = 0;
        status_pad = status_raw;
        do
        {
        } while (0);
        if (Degree != 0)
        {
            status_raw = (s32)status_pad;
        }
        else
        {
            status_raw = (s32)status_pad;
        }
        if (status_human->motion->count ==
            BattleDB[status_human->warid].contfrm)
        {
            goto status7_continue;
        }
        status7_result = 0;
        goto status7_return;

    status7_continue:
        if (Distance < 2000)
        {
            status_degree = Degree;
            if (status_degree < 0)
            {
                status_degree = -status_degree;
            }
            if (status_degree < 1000)
            {
                goto choose_status7;
            }
        }
        if (rand() % (EngageLevel + 1) != 0)
        {
            status7_result = status_raw;
            goto status7_return;
        }

    choose_status7:
        if (Degree >= 301)
        {
            status_raw = 0x2000;
        }
        else
        {
            status_raw |= 0x80;
            if (Degree < -300)
            {
                status_raw = -0x8000;
            }
            else
            {
                goto status7_value;
            }
        }
        status_raw |= 0x80;

    status7_value:
        status7_result = status_raw;
    status7_return:
        return (s16)status7_result;
    }

    if (Me_THINK_C->status == STAT_JUMP)
    {
        return 0;
    }

    motion = Me_THINK_C->motion;
    if (motion->mid == MOT_ENGAGE)
    {
        return (u16)(rand() % (EngageLevel + 1) != 0) << 14;
    }

    if (Me_THINK_C->actmode == 0)
    {
        s32 raw_degree;
        s32 degree;

        if (Distance < 2500)
        {
            raw_degree = Degree;
            degree = (raw_degree >= 0) ? raw_degree : -raw_degree;
            if (degree < 1500)
            {
                if ((motion->count & 0xf) != 0)
                {
                    return 0;
                }
                if (raw_degree >= 501)
                {
                    pad = 0x2000;
                }
                else if (raw_degree < -500)
                {
                    pad = -0x8000;
                }
                pad |= 0x80;
                goto activate_and_return;
            }
        }

        pad = ChasetoTarget(2000);
        if (pad == 0)
        {
            Me_THINK_C->actmode = 1;
        }
        if (Distance >= 4001)
        {
            degree = Degree;
            if (degree < 0)
            {
                degree = -degree;
            }
            if (degree < 100 && rand() % 5 == 0)
            {
                pad = 0x1040;
            }
        }
        if ((ATTRIB_BITS & 0x4000) == 0)
        {
            goto return_pad;
        }

    activate_and_return:
        Me_THINK_C->actmode = 1;
        goto return_pad;
    }

    if ((motion->count & 0xf) != 0)
    {
        s32 raw_degree;
        s32 degree;

        pad = Me_THINK_C->pad.data;
        if (Distance >= 1500)
        {
            goto return_pad;
        }
        raw_degree = Degree;
        degree = (raw_degree >= 0) ? raw_degree : -raw_degree;
        if (degree < 1000)
        {
            pad = 0x4000;
            goto return_pad;
        }
        if (degree >= 1501)
        {
            if (Distance < 1000)
            {
                pad = SetCommand(&Me_THINK_C->pad, 1);
                goto return_pad;
            }
            pad = 0x1000;
            goto return_pad;
        }
        if (rand() % 30 != 0)
        {
            goto return_pad;
        }
        pad = SetCommand(&Me_THINK_C->pad, 0x21);
        goto return_pad;
    }

    if (Distance >= 4001)
    {
        Humanoid *me;

        Me_THINK_C->actmode = 0;
        me = Me_THINK_C;
        Me_THINK_C->chase[1] = 0;
        me->chase[0] = 0;
        ItemUse();
        if (Distance >= 5001)
        {
            pad = 0x1040;
        }
        goto return_pad;
    }

    if ((ATTRIB_BITS & 0x400) != 0)
    {
        Me_THINK_C->actmode = 0;
    }

    if (Degree >= 501)
    {
        pad = 0x2000;
    }
    else if (Degree < -500)
    {
        pad = -0x8000;
    }

    if ((u32)(Distance - 1501) < 2499)
    {
        s32 attack_degree;

        attack_degree = Degree;
        if (attack_degree < 0)
        {
            attack_degree = -attack_degree;
        }
        if (attack_degree < 1000 &&
            rand() % (EngageLevel + 1) == 0 &&
            GameClock > AttackActionCount)
        {
            AttackActionCount = GameClock + EngageLevel * 10;
            if (rand() % 3 == 0)
            {
                pad = 0x4000;
            }
            return pad | 0x80;
        }
    }

    {
        s32 raw_degree;
        s32 degree;

        raw_degree = Degree;
        degree = (raw_degree >= 0) ? raw_degree : -raw_degree;
        if (degree >= 1501)
        {
            pad |= 0x4000;
            goto return_pad;
        }

        if (Distance >= 3001)
        {
            if (degree >= 200 || Distance < 3501)
            {
                goto return_with_1000;
            }
            if ((rand() & 1) != 0)
            {
                pad = SetCommand(&Me_THINK_C->pad, 1);
                goto return_pad;
            }
            if ((rand() & 1) != 0)
            {
                pad = SetCommand(&Me_THINK_C->pad, 0x21);
                goto return_pad;
            }
            ItemUse();
            goto return_pad;

        return_with_1000:
            pad |= 0x1000;
            goto return_pad;
        }

        if (Distance < 1500)
        {
            if (raw_degree >= 301)
            {
                pad = SetCommand(&Me_THINK_C->pad, 3);
                goto return_pad;
            }
            if (raw_degree < -300)
            {
                pad = SetCommand(&Me_THINK_C->pad, 4);
                goto return_pad;
            }
            if (Distance >= 1000)
            {
                pad |= 0x80;
                goto return_pad;
            }
            pad = 0xa0;
            if ((rand() & 1) != 0)
            {
                pad = 0x4040;
            }
            goto return_pad;
        }

        if ((rand() & 1) == 0)
        {
            goto return_pad;
        }
        if (Degree >= 101)
        {
            pad = SetCommand(&Me_THINK_C->pad, 4);
            goto return_pad;
        }
        if (Degree < -100)
        {
            pad = SetCommand(&Me_THINK_C->pad, 3);
            goto return_pad;
        }
        pad = SetCommand(&Me_THINK_C->pad, 2);
    }

return_pad:
    return pad;
}
