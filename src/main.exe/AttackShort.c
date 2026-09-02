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
 * static short AttackShort(void);
 *     THINK_3.C:266, 98 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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

extern Humanoid *Me_THINK_C;
extern s32 AttackActionCount;

extern s16 AttackAnimal(void);
extern s16 ChasetoTarget(s32 distance);
extern s16 ItemUse(void);

short AttackShort(void)
{
    MotionManager *motion;
    s16 pad;
    s32 attack_result;

    pad = 0;
    if ((Me_THINK_C->type & PAGE_MASK) == PAGE_BEAST)
    {
        return AttackAnimal();
    }

    if (Me_THINK_C->status == STAT_ATTACK)
    {
        Humanoid *status_human;
        s16 pad;
        s32 status_raw;
        s32 status_degree;

        status_human = Me_THINK_C;
        status_raw = 0;
        pad = status_raw;
        /* Empty loop retained for code layout; its original source construct is unknown. */
        do
        {
        } while (0);
        if (Degree != 0)
        {
            status_raw = (s32)pad;
        }
        else
        {
            status_raw = (s32)pad;
        }
        if (status_human->motion->count ==
            BattleDB[status_human->warid].contfrm)
        {
            goto attack_continue;
        }
        attack_result = 0;
        goto attack_return;

    attack_continue:
        if (Distance < 2000)
        {
            status_degree = Degree;
            if (status_degree < 0)
            {
                status_degree = -status_degree;
            }
            if (status_degree < 1000)
            {
                goto choose_attack;
            }
        }
        if (rand() % (EngageLevel + 1) != 0)
        {
            attack_result = status_raw;
            goto attack_return;
        }

    choose_attack:
        if (Degree > 300)
        {
            status_raw = PADLright;
        }
        else
        {
            status_raw |= PADRleft;
            if (Degree < -300)
            {
                status_raw = (s16)PADLleft;
            }
            else
            {
                goto attack_value;
            }
        }
        status_raw |= PADRleft;

    attack_value:
        attack_result = status_raw;
    attack_return:
        return (s16)attack_result;
    }

    if (Me_THINK_C->status == STAT_JUMP)
    {
        return 0;
    }

    motion = Me_THINK_C->motion;
    if (motion->mid == MOT_ENGAGE)
    {
        return (rand() % (EngageLevel + 1) != 0) ? PADLdown : 0;
    }

    if (Me_THINK_C->actmode == MELEE_ATTACK_CLOSING)
    {
        s32 raw_degree;
        s32 degree;

        if (Distance < 2500)
        {
            raw_degree = Degree;
            degree = (raw_degree >= 0) ? raw_degree : -raw_degree;
            if (degree < 1500)
            {
                if ((motion->count &
                     (MELEE_ATTACK_DECISION_PERIOD - 1)) != 0)
                {
                    return 0;
                }
                if (raw_degree > 500)
                {
                    pad = PADLright;
                }
                else if (raw_degree < -500)
                {
                    pad = PADLleft;
                }
                pad |= PADRleft;
                Me_THINK_C->actmode = MELEE_ATTACK_ENGAGED;
                goto return_pad;
            }
        }

        pad = ChasetoTarget(2000);
        if (pad == 0)
        {
            Me_THINK_C->actmode = MELEE_ATTACK_ENGAGED;
        }
        if (Distance > 4000)
        {
            degree = Degree;
            if (degree < 0)
            {
                degree = -degree;
            }
            if (degree < 100 && rand() % 5 == 0)
            {
                pad = PADLup | PADRdown;
            }
        }
        if ((Attrib & ATTR_HIT) != 0)
        {
            Me_THINK_C->actmode = MELEE_ATTACK_ENGAGED;
        }
        goto return_pad;
    }

    if ((motion->count & (MELEE_ATTACK_DECISION_PERIOD - 1)) != 0)
    {
        s32 raw_degree;
        s32 degree;

        pad = Me_THINK_C->pad.data;
        if (Distance < 1500)
        {
            raw_degree = Degree;
            degree = (raw_degree >= 0) ? raw_degree : -raw_degree;
            if (degree < 1000)
            {
                pad = PADLdown;
            }
            else if (degree > 1500)
            {
                if (Distance < 1000)
                {
                    pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_FORWARD);
                }
                else
                {
                    pad = PADLup;
                }
            }
            else if (rand() % 30 == 0)
            {
                pad = SetCommand(&Me_THINK_C->pad, CMD_LUNGE);
            }
        }
        goto return_pad;
    }

    if (Distance > 4000)
    {
        Humanoid *me;

        Me_THINK_C->actmode = MELEE_ATTACK_CLOSING;
        me = Me_THINK_C;
        Me_THINK_C->chase[HUMANOID_CHASE_Z] = 0;
        me->chase[HUMANOID_CHASE_X] = 0;
        ItemUse();
        if (Distance > 5000)
        {
            pad = PADLup | PADRdown;
        }
        goto return_pad;
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

    if (Distance > 1500 && Distance < 4000)
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
            AttackActionCount = GameClock + EngageLevel * ATTACK_COOLDOWN_PER_LEVEL;
            if (rand() % 3 == 0)
            {
                pad = PADLdown;
            }
            return pad | PADRleft;
        }
    }

    {
        s32 raw_degree;
        s32 degree;

        raw_degree = Degree;
        degree = (raw_degree >= 0) ? raw_degree : -raw_degree;
        if (degree > 1500)
        {
            pad |= PADLdown;
            goto return_pad;
        }

        if (Distance > 3000)
        {
            if (degree < 200 && Distance > 3500)
            {
                if ((rand() & 1) != 0)
                {
                    pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_FORWARD);
                }
                else if ((rand() & 1) != 0)
                {
                    pad = SetCommand(&Me_THINK_C->pad, CMD_LUNGE);
                }
                else
                {
                    ItemUse();
                }
            }
            else
            {
                pad |= PADLup;
            }
            goto return_pad;
        }

        if (Distance < 1500)
        {
            if (raw_degree > 300)
            {
                pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_LEFT);
            }
            else if (raw_degree < -300)
            {
                pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_RIGHT);
            }
            else if (Distance >= 1000)
            {
                pad |= PADRleft;
            }
            else
            {
                pad = PADRleft | PADRright;
                if ((rand() & 1) != 0)
                {
                    pad = PADLdown | PADRdown;
                }
            }
            goto return_pad;
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
            else
            {
                pad = SetCommand(&Me_THINK_C->pad, CMD_DASH_BACKWARD);
            }
        }
    }

return_pad:
    return pad;
}
