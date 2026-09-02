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

extern Humanoid *Me_THINK_C;
extern s32 AttackActionCount;

extern s16 ChasetoTarget(s32 distance);
extern s16 ItemUse(void);

short AttackGeneral(void)
{
    s16 pad;

    pad = 0;
    RETURN_ATTACK_CONTINUATION(pad, 2000, 500);

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

        Me_THINK_C->actmode = MELEE_ATTACK_CLOSING;
        me = Me_THINK_C;
        Me_THINK_C->chase[HUMANOID_CHASE_Z] = 0;
        me->chase[HUMANOID_CHASE_X] = 0;
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
