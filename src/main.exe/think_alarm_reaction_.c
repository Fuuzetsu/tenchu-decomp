#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "humanoid.h"
#include "game_globals.h"
#include "appear.h"
#include "item.h"

/*
 * think_alarm_reaction_ (0x8002c86c) — the alarmed-guard
 * converge/reinforce think handler, driven by actscnt over three
 * stages. State 0 walks to the remembered chase[] spot via
 * turn_towards_player_; arriving (SquareRoot0 distance under 2000, or
 * ATTR_WALL) refreshes EmergencyNotice to ALERT_DURATION, plays sound
 * 0xE, seeds actcnt by difficulty (easy 1, normal a coin flip, hard 0)
 * and picks the next state: nonzero actcnt goes to 1, otherwise 1 when
 * Humans has reached 30 or the stage is STAGE_CURE_PRINCESS, else 2.
 * State 1 cycles actcnt through 0..31 and only acts while bit 3 is
 * set: at exactly 8 it picks a fresh command — turn toward the target
 * when |Degree| is 701 or more, else 4-in-5 a random PADLright/
 * PADLleft and 1-in-5 PADLup — and repeats the humanoid's stored pad
 * for the rest of that window, returning 0 the rest of the cycle.
 * State 2 steers at the chase point with GetDirection, pairing the
 * turn bit (PADLleft when the error is positive, else PADLright) with
 * PADLup past 1000 of error and PADLdown below it, and on ATTR_WALL
 * with |Degree| over 1000 latches pad_hold with a 1000/turn-frame
 * sidestep. Past 16501 units it calls for help: EmergencyNotice
 * refreshed, actscnt 0 / actcnt 1, a bark (sound 9 or 10), then
 * BreedLife spawns AIDHumanType[StageID][coin flip] at its own spot
 * turned by that direction, wires think[0..3] from Think*Func[4],
 * raises attribute bits 4 then 0x11, equips a weapon, starts motion
 * 0x501, copies chase[] with a +/-1000 jitter in 500-unit steps, barks
 * again and bumps StageEnemies.
 */

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
    VECTOR *new_var;
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

            alertTime = ALERT_DURATION;
            if (gNannido == DIFFICULTY_HARD)
            {
                alertTime = ALERT_DURATION_HARD;
            }
            EmergencyNotice = alertTime;
            Sound(Me_THINK_C, 0xE);

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
                    nextState = 8;
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
            goto done;
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
                if (absoluteDegree >= 0x2BD)
                {
                    result = -0x8000;
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
                        result = -0x8000;
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
        s32 absoluteDirection;
        s32 turnBits;

        direction = GetDirection(x_diff, z_diff,
                                 Me_THINK_C->rotate->vy);
        absoluteDirection = direction;
        turnBits = 0x2000;
        if (absoluteDirection > 0)
        {
            turnBits = 0x8000;
        }
        if (absoluteDirection < 0)
        {
            absoluteDirection = -absoluteDirection;
        }
        result = turnBits | 0x4000;
        if (absoluteDirection >= 1000)
        {
            result = turnBits | 0x1000;
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
                    quotient | ((degree > 0 ? PADLleft : PADLright) << 16);
            }
        }

        if (Distance < 0x4075)
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

            alertTime = ALERT_DURATION;
            if (gNannido == DIFFICULTY_HARD)
            {
                alertTime = ALERT_DURATION_HARD;
            }
            EmergencyNotice = alertTime;
            Me_THINK_C->actscnt = 0;
            Me_THINK_C->actcnt = 1;

            randomValue = rand();
            soundId = 10;
            if (randomValue & 1)
            {
                soundId = 9;
            }
            Sound(Me_THINK_C, soundId);

            type = AIDHumanType[StageID][rand() % 2];
            rotation = (SVECTOR *)Me_THINK_C->rotate;
            newRotation = rotation->vy + direction;
            new_var = Me_THINK_C->locate;
            rotation->vy = newRotation;
            position = new_var;
            human = BreedLife(type, position->vx, position->vy, position->vz,
                              newRotation);

            human->target = Me_THINK_C->target;
            human->think[0] = Think1Func[4];
            human->think[1] = Think2Func[4];
            human->think[2] = Think3Func[4];
            think4 = Think4Func[4];
            *(u16 *)&human->attribute |= 4;
            human->think[3] = think4;
            EquipWeapon(human, 1);
            SetNowMotion(human, 0x501, 1);
            human->actscnt = 0;
            human->actcnt = 1;
            *(u16 *)&human->attribute |= 0x11;

            human->chase[0] = Me_THINK_C->chase[0] +
                              (rand() % 5 - 2) * 500;
            human->chase[1] = Me_THINK_C->chase[1] +
                              (rand() % 5 - 2) * 500;
            randomValue = rand();
            soundId = 10;
            if (randomValue & 1)
            {
                soundId = 9;
            }
            self = (Humanoid *)human;
            Sound((Humanoid *)self, soundId);
            StageEnemies++;
        }
    }

done:
    return result;
}
