#include "common.h"
#include "sound.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "humanoid.h"
#include "game_globals.h"
#include "appear.h"
#include "item.h"

extern Humanoid *Me_THINK_C;
extern long EmergencyNotice;
extern character_kind
    AIDHumanType[N_STAGE_CONFIGS][N_STAGE_REINFORCEMENT_CHOICES];
extern s16 GotoPosition(s32 vx, s32 vz);
extern int rand(void);

/* actscnt belongs to whichever Think* handler is active.  In this handler it
 * is a state, not a counter: reach the reported position, circle there, or
 * move far enough away to call in another guard. */
enum alarm_reaction_state
{
    ALARM_REACTION_APPROACH = 0,
    ALARM_REACTION_CIRCLE = 1,
    ALARM_REACTION_CALL_BACKUP = 2
};

s16 think_alarm_reaction_(void)
{
    s32 x_diff;
    s32 z_diff;
    s16 result;
    u8 state;
    VECTOR *loc;
    Humanoid *self;

    result = 0;
    x_diff = Me_THINK_C->chase[HUMANOID_CHASE_X] -
             Me_THINK_C->locate->vx;
    z_diff = Me_THINK_C->chase[HUMANOID_CHASE_Z] -
             Me_THINK_C->locate->vz;
    state = Me_THINK_C->actscnt;

    if (state == ALARM_REACTION_APPROACH)
    {
        s32 distance;

        result = GotoPosition(x_diff, z_diff);
        distance = SquareRoot0(x_diff * x_diff + z_diff * z_diff);
        if (distance < 2000 || (Attrib & ATTR_WALL))
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
                    nextState = ALARM_REACTION_CIRCLE;
                }
                else
                {
                    nextState = STAGE_ID_TRAINING;
                    alertTime = StageID;
                    if (alertTime != nextState)
                    {
                        nextState = ALARM_REACTION_CALL_BACKUP;
                    }
                    else
                    {
                        nextState = ALARM_REACTION_CIRCLE;
                    }
                }
            }
            else
            {
                nextState = ALARM_REACTION_CIRCLE;
            }
            self->actscnt = nextState;
        }
    }
    else if (state == ALARM_REACTION_CIRCLE)
    {
        u8 count;

        Me_THINK_C->actcnt = (Me_THINK_C->actcnt + 1) & 0x1F;
        count = Me_THINK_C->actcnt;
        if (count & 8)
        {
            if (count != 8)
            {
                result = Me_THINK_C->pad.data;
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
                }
                else
                {
                    s32 randomValue;

                    randomValue = rand();
                    if (randomValue % 5 != 0)
                    {
                        result = PADLright;
                        if (rand() & 1)
                        {
                            result = -PADLleft;
                        }
                    }
                    else
                    {
                        result = PADLup;
                    }
                }
            }
        }
    }
    else
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

        if (Attrib & ATTR_WALL)
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

        if (Distance > 16500)
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
            Me_THINK_C->actscnt = ALARM_REACTION_APPROACH;
            Me_THINK_C->actcnt = 1;

            randomValue = rand();
            soundId = CHAR_VOICE_ACTION_B;
            if (randomValue & 1)
            {
                soundId = CHAR_VOICE_ACTION_A;
            }
            Sound(Me_THINK_C, soundId);

            type = AIDHumanType[StageID][
                rand() % N_STAGE_REINFORCEMENT_CHOICES];
            rotation = Me_THINK_C->rotate;
            newRotation = rotation->vy + direction;
            loc = Me_THINK_C->locate;
            rotation->vy = newRotation;
            position = loc;
            human = BreedLife(type, position->vx, position->vy, position->vz,
                              newRotation);

            human->target = Me_THINK_C->target;
            human->think[0] = Think1Func[THINK1_WATCH];
            human->think[1] = Think2Func[THINK2_CONTACT];
            human->think[2] = Think3Func[THINK3_ATK_CHASE];
            think4 = Think4Func[THINK4_CONTACT];
            human->attribute |= ATTR_CUSTOMAI;
            human->think[3] = think4;
            EquipWeapon(human, WEAPON_DRAWN);
            SetNowMotion(human, MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
            human->actscnt = ALARM_REACTION_APPROACH;
            human->actcnt = 1;
            human->attribute |= ATTR_SEARCH | PHASE_SUSPICIOUS;

            human->chase[HUMANOID_CHASE_X] = Me_THINK_C->chase[HUMANOID_CHASE_X] +
                              (rand() % 5 - 2) * 500;
            human->chase[HUMANOID_CHASE_Z] = Me_THINK_C->chase[HUMANOID_CHASE_Z] +
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

    return result;
}
