#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "appear.h"
#include "item.h"

/*
 * Multi-stage AI handler used while an alerted character circles in small
 * steps.  It either chooses a turn command, advances the circling timer, or
 * spawns a second humanoid when the target remains far away.
 *
 * This translation unit reads Attrib unsigned, unlike main.exe.h's signed
 * declaration, so the small set of globals used here is declared locally.
 */
extern Humanoid *Me_THINK_C;
extern u16 Attrib;
extern s16 Degree;
extern s32 Distance;
extern s32 FRAMES_UNTIL_END_OF_ALERT;
extern u8 gNannido;
extern s16 Humans;
extern u16 StageEnemies;
extern s16 AIDHumanType[][2];
extern think_func_ *Think1Func[];
extern think_func_ *Think2Func[];
extern think_func_ *Think3Func[];
extern think_func_ *Think4Func[];

extern s16 turn_towards_player_(s32 x_diff, s32 z_diff);
extern s16 GetDirection(s32 x_diff, s32 z_diff, s16 rotation);
extern int rand(void);
extern short Sound(Humanoid *human, s16 seid);
extern short SetNowMotion(Humanoid *human, short mid, short move);

/*
 * `nextState` intentionally carries each condition and its eventual result.
 * Reusing the dead `alertTime` local for StageID keeps the comparison operand
 * separate while allowing cc1 to reuse the condition register for the branch
 * delay-slot assignments.
 */

s16 think_setting_small_rotation_small_steps_(void)
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
        if (distance < 2000 || (Attrib & 0x400))
        {
            s32 alertTime;
            s32 nextState;

            alertTime = 300;
            if (gNannido == DIFFICULTY_HARD)
            {
                alertTime = 600;
            }
            FRAMES_UNTIL_END_OF_ALERT = alertTime;
            Sound((Humanoid *)Me_THINK_C, 0xE);

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
                result = ((Humanoid *)Me_THINK_C)->pad.data;
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
                        result = 0x2000;
                    }
                    goto done;
                }
                else
                {
                    s32 randomValue;

                    randomValue = rand();
                    if (randomValue % 5 != 0)
                    {
                        result = 0x2000;
                        if ((rand() & 1) == 0)
                        {
                            goto done;
                        }
                        result = -0x8000;
                    }
                    else
                    {
                        result = 0x1000;
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

        if (Attrib & 0x400)
        {
            s32 degree;
            s32 absoluteDegree;

            degree = Degree;
            absoluteDegree = __builtin_abs(degree);
            if (absoluteDegree > 1000 && Me_THINK_C->field76_0xb0 == 0)
            {
                s32 quotient;

                self = Me_THINK_C;
                quotient = 1000 / self->turn;
                self->field76_0xb0 = quotient |
                    (degree > 0 ? 0x80000000 : 0x20000000);
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
            think_func_ *think4;

            alertTime = 300;
            if (gNannido == DIFFICULTY_HARD)
            {
                alertTime = 600;
            }
            FRAMES_UNTIL_END_OF_ALERT = alertTime;
            Me_THINK_C->actscnt = 0;
            Me_THINK_C->actcnt = 1;

            randomValue = rand();
            soundId = 10;
            if (randomValue & 1)
            {
                soundId = 9;
            }
            Sound((Humanoid *)Me_THINK_C, soundId);

            type = AIDHumanType[StageID][rand() % 2];
            rotation = (SVECTOR *)Me_THINK_C->rotate;
            newRotation = rotation->vy + direction;
            new_var = Me_THINK_C->locate;
            rotation->vy = newRotation;
            position = new_var;
            human = BreedLife(type, position->vx, position->vy, position->vz,
                              newRotation);

            human->target = ((Humanoid *)Me_THINK_C)->target;
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
