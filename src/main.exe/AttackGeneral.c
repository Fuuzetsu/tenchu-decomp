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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 *  - The cold close-range `% 4` switch needs an explicit default exit.
 *    Its cases may use `break` when an enclosing `else` keeps the ordinary
 *    range arm separate; omitting the default adds another predecessor and
 *    makes CSE reload the pre-switch Distance value.
 */

extern Humanoid *Me_THINK_C;
extern s32 AttackActionCount;

extern s16 ChasetoTarget(s32 distance);
extern s16 ItemUse(void);

short AttackGeneral(void)
{
    s16 pad;
    s16 attack_result;

    pad = 0;
    if (Me_THINK_C->status == STAT_ATTACK)
    {
        s32 deg;

        do
        {
            if (Me_THINK_C->motion->count !=
                BattleDB[Me_THINK_C->warid].contfrm)
            {
                attack_result = 0;
                goto attack_return;
            }
            if (Distance < 2000)
            {
                deg = Degree;
                if (deg < 0)
                {
                    deg = -deg;
                }
                if (deg < 500)
                {
                    goto choose_attack;
                }
            }
            if (rand() % (EngageLevel + 1) != 0)
            {
                attack_result = pad;
                goto attack_return;
            }
        } while (0);

    choose_attack:
        /* The doubled |= PADRleft around the goto is byte-required (the
         * flat else-if respell mismatches; measured in AttackLong's
         * identical block). */
        if (Degree > 300)
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
                goto attack_value;
            }
        }
        pad |= PADRleft;

    attack_value:
        attack_result = pad;
    attack_return:
        return attack_result;
    }

    if (Me_THINK_C->status == STAT_JUMP)
    {
        return 0;
    }

    if (Me_THINK_C->motion->mid == MOT_ENGAGE)
    {
        return (rand() % (EngageLevel + 1) != 0) ? PADLdown : 0;
    }

    if (Me_THINK_C->actmode == MELEE_ATTACK_CLOSING)
    {
        s32 deg;

        pad = ChasetoTarget(3000);
        if (pad == 0 || (Attrib & ATTR_HIT) != 0)
        {
            Me_THINK_C->actmode = MELEE_ATTACK_ENGAGED;
        }
        if (Distance > 5000)
        {
            deg = Degree;
            if (deg < 0)
            {
                deg = -deg;
            }
            if (deg < 100 && rand() % 5 == 0)
            {
                pad = PADLup | PADRdown;
            }
        }
        return pad;
    }

    if ((Me_THINK_C->motion->count &
         (MELEE_ATTACK_DECISION_PERIOD - 1)) != 0)
    {
        s32 d;
        s32 deg;

        pad = Me_THINK_C->pad.data;
        if (Distance < 2000)
        {
            d = Degree;
            /* The lone ternary abs (vs this file's five if-negate abs) is
             * measured byte-required. */
            deg = (d >= 0) ? d : -d;
            if (deg < 1000)
            {
                pad = PADLdown;
            }
            else if (deg > 1500)
            {
                pad = PADLup;
            }
        }
        return pad;
    }

    if (Distance > 5000)
    {
        Humanoid *me;

        /* Mid-sequence alias + mixed spellings: byte-required (measured in
         * AttackLong's identical block). */
        Me_THINK_C->actmode = MELEE_ATTACK_CLOSING;
        me = Me_THINK_C;
        Me_THINK_C->chase.point[HUMANOID_CHASE_Z] = 0;
        me->chase.point[HUMANOID_CHASE_X] = 0;
        ItemUse();
        return 0;
    }

    if ((Attrib & ATTR_WALL) != 0)
    {
        Me_THINK_C->actmode = MELEE_ATTACK_CLOSING;
    }

    if (Degree > 500)
    {
        pad = PADLright;
    }
    else if (Degree < -500)
    {
        pad = PADLleft;
    }

    if (Distance > 1000 && Distance < 3000)
    {
        s32 deg;

        do
        {
            deg = Degree;
            if (deg < 0)
            {
                deg = -deg;
            }
            if (deg >= 1200)
            {
                break;
            }
            if (rand() % (EngageLevel + 1) != 0)
            {
                break;
            }
            if (GameClock > AttackActionCount)
            {
                AttackActionCount = GameClock + EngageLevel * ATTACK_COOLDOWN_PER_LEVEL;
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

        if (degree > 1000 || Distance < 2000)
        {
            if (Distance < 1000)
            {
                switch (rand() % 4)
                {
                case 0:
                    pad = PADLdown | PADRdown;
                    break;
                case 1:
                    pad = PADRleft | PADRright;
                    break;
                case 2:
                    pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_BACKWARD);
                    break;
                case 3:
                    pad |= PADRleft;
                    break;
                default:
                    break;
                }
            }
            else
            {
                pad |= PADLdown;
            }
            return pad;
        }

        if (Distance > 3000)
        {
            pad |= PADLup;
            if (Distance > 4000)
            {
                if ((rand() & 1) != 0)
                {
                    pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_FORWARD);
                }
                else
                {
                    degree = Degree;
                    if (degree < 0)
                    {
                        degree = -degree;
                    }
                    if (degree < 500)
                    {
                        pad = SetCommand(&Me_THINK_C->pad, CMD_LUNGE);
                    }
                    else
                    {
                        ItemUse();
                    }
                }
            }
            return pad;
        }

        if ((rand() & 1) != 0)
        {
            if (Degree > 100)
            {
                pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_RIGHT);
            }
            else if (Degree < -100)
            {
                pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_LEFT);
            }
        }
    }

    return pad;
}
