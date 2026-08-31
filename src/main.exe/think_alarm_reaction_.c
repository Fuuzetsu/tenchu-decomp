#include "common.h"
#include "sound.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "humanoid.h"
#include "game_globals.h"
#include "appear.h"
#include "item.h"

/*
 * Multi-stage AI handler used while an alerted character circles in small
 * steps.  It either chooses a turn command, advances the circling timer, or
 * spawns a second humanoid when the target remains far away.
 *
 * This translation unit reads the recovered signed `Attrib` object's raw flag
 * bits through the shared unsigned `ATTRIB_BITS` view.
 */
extern Humanoid *Me_THINK_C;
extern long EmergencyNotice;
extern s16 AIDHumanType[][2];
extern s16 turn_towards_player_(s32 x_diff, s32 z_diff);
extern int rand(void);

/*
 * `nextState` intentionally carries each condition and its eventual result.
 * Reusing the dead `alertTime` local for StageID keeps the comparison operand
 * separate while allowing cc1 to reuse the condition register for the branch
 * delay-slot assignments.
 */

s16 think_alarm_reaction_(void)
{
    s32 x_diff;
    s32 z_diff;
    s16 result;
    u8 state;
    VECTOR *loc;
    Humanoid *self;

    result = 0;
    x_diff = Me_THINK_C->chase[0] -
             Me_THINK_C->locate->vx;
    z_diff = Me_THINK_C->chase[1] -
             Me_THINK_C->locate->vz;
    state = Me_THINK_C->actscnt;

    if (state == 0)
    {
        s32 distance;

        result = turn_towards_player_(x_diff, z_diff);
        distance = SquareRoot0(x_diff * x_diff + z_diff * z_diff);
        if (distance < 2000 || (ATTRIB_BITS & ATTR_WALL))
        {
            s32 alertTime;
            s32 nextState;

            RESET_ALERT_DURATION(alertTime);
            Sound(Me_THINK_C, CHAR_VOICE_REACTION);

            switch (gNannido)
            {
            case DIFFICULTY_EASY:
                Me_THINK_C->actcnt = 1;
                break;
            case DIFFICULTY_NORMAL:
                Me_THINK_C->actcnt = rand() & 1;
                break;
            case DIFFICULTY_HARD:
                Me_THINK_C->actcnt = 0;
                break;
            }

            self = Me_THINK_C;
            nextState = self->actcnt;
            if (nextState == 0)
            {
                nextState = Humans;
                nextState = nextState < 30;
                if (nextState == 0)
                {
                    nextState = 1;
                }
                else
                {
                    nextState = STAGE_CURE_PRINCESS;
                    alertTime = StageID;
                    if (alertTime != nextState)
                    {
                        nextState = 2;
                    }
                    else
                    {
                        nextState = 1;
                    }
                }
            }
            else
            {
                nextState = 1;
            }
            self->actscnt = nextState;
        }
        goto done;
    }

    if (state == 1)
    {
        u8 count;

        Me_THINK_C->actcnt = (Me_THINK_C->actcnt + 1) & 0x1F;
        count = Me_THINK_C->actcnt;
        if (count & 8)
        {
            if (count != 8)
            {
                result = Me_THINK_C->pad.data;
                goto done;
            }
            else
            {
                s32 degree;
                s32 absoluteDegree;

                degree = Degree;
                absoluteDegree = __builtin_abs(degree);
                if (absoluteDegree > 700)
                {
                    result = -PADLleft;
                    if (degree > 0)
                    {
                        result = PADLright;
                    }
                    goto done;
                }
                else
                {
                    s32 randomValue;

                    randomValue = rand();
                    if (randomValue % 5 != 0)
                    {
                        result = PADLright;
                        if ((rand() & 1) == 0)
                        {
                            goto done;
                        }
                        result = -PADLleft;
                    }
                    else
                    {
                        result = PADLup;
                    }
                }
            }
        }
        goto done;
    }

    {
        s16 direction;
        s32 direction2;
        s32 turnBits;

        direction = GetDirection(x_diff, z_diff,
                                 Me_THINK_C->rotate->vy);
        direction2 = direction;
        turnBits = PADLright;
        if (direction2 > 0)
        {
            turnBits = PADLleft;
        }
        if (direction2 < 0)
        {
            direction2 = -direction2;
        }
        result = turnBits | PADLdown;
        if (direction2 >= 1000)
        {
            result = turnBits | PADLup;
        }

        if (ATTRIB_BITS & ATTR_WALL)
        {
            s32 degree;
            s32 absoluteDegree;

            degree = Degree;
            absoluteDegree = __builtin_abs(degree);
            if (absoluteDegree > 1000 && Me_THINK_C->pad_hold == 0)
            {
                s32 quotient;

                self = Me_THINK_C;
                quotient = 1000 / self->turn;
                self->pad_hold =
                    PAD_HOLD(degree > 0 ? PADLleft : PADLright, quotient);
            }
        }

        if (Distance <= 16500)
        {
            goto done;
        }
        else
        {
            s32 alertTime;
            s16 soundId;
            s32 randomValue;
            s32 type;
            SVECTOR *rotation;
            VECTOR *position;
            s16 newRotation;
            Humanoid *human;
            ThinkFunc think4;

            RESET_ALERT_DURATION(alertTime);
            Me_THINK_C->actscnt = 0;
            Me_THINK_C->actcnt = 1;

            randomValue = rand();
            soundId = CHAR_VOICE_ACTION_B;
            if (randomValue & 1)
            {
                soundId = CHAR_VOICE_ACTION_A;
            }
            Sound(Me_THINK_C, soundId);

            type = AIDHumanType[StageID][rand() % 2];
            rotation = Me_THINK_C->rotate;
            newRotation = rotation->vy + direction;
            /* Staged locate read straddling the vy store: byte-required
             * (a direct position load recolors the base; measured). */
            loc = Me_THINK_C->locate;
            rotation->vy = newRotation;
            position = loc;
            human = BreedLife(type, position->vx, position->vy, position->vz,
                              newRotation);

            human->target = Me_THINK_C->target;
            human->think[0] = Think1Func[4];
            human->think[1] = Think2Func[4];
            human->think[2] = Think3Func[4];
            think4 = Think4Func[4];
            *(u16 *)&human->attribute |= ATTR_CUSTOMAI;
            human->think[3] = think4;
            EquipWeapon(human, 1);
            SetNowMotion(human, MOT_ENGAGE_STANCE, 1);
            human->actscnt = 0;
            human->actcnt = 1;
            *(u16 *)&human->attribute |= ATTR_SEARCH | PHASE_SUSPICIOUS;

            human->chase[0] = Me_THINK_C->chase[0] +
                              (rand() % 5 - 2) * 500;
            human->chase[1] = Me_THINK_C->chase[1] +
                              (rand() % 5 - 2) * 500;
            randomValue = rand();
            soundId = CHAR_VOICE_ACTION_B;
            if (randomValue & 1)
            {
                soundId = CHAR_VOICE_ACTION_A;
            }
            Sound(human, soundId);
            StageEnemies++;
        }
    }

done:
    return result;
}
