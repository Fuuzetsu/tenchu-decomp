#include "common.h"
#include "trial.exe.h"
#include "../main.exe/item.h"
#include "../main.exe/padcmd.h"
#include "../main.exe/sound.h"
#include "../main.exe/appear.h"
#include "../main.exe/infoview.h"

/* Shared AI programs from MAIN, with TRIAL's escort and alert behavior.
 * Dispatch tables establish the identities of the editor-specific handlers. */

enum trial_escort_policy
{
    TRIAL_ESCORT_DESTINATION = 1,
    TRIAL_ESCORT_PRINCESS = 2,
    TRIAL_ESCORT_CHECK_PERIOD = 30,
    TRIAL_ESCORT_ARRIVAL_RADIUS = 1500,
    TRIAL_ESCORT_PLAYER_RANGE = 8000,
    TRIAL_ESCORT_CALL_PERIOD = 150,
    TRIAL_ESCORT_ALERT_PAUSE = 60,
    TRIAL_ESCORT_ALERT_PERIOD = 90,
    TRIAL_ENGAGE_HARD = 1
};

#define RESET_TRIAL_ALERT_DURATION(tmp) \
    tmp = ALERT_DURATION_HARD; \
    if (EngageLevel != TRIAL_ENGAGE_HARD) \
    { \
        tmp = ALERT_DURATION; \
    } \
    EmergencyNotice = tmp;


s32 AttackActionCount = 0;
long EmergencyNotice = 0;
s32 StrainRatio = 0;
s16 EngageLevel = 1;
static Humanoid *Me = NULL;
static long Distance = 0;
static s16 Degree = 0;
static s16 SR = 0;
static PADtype *Pad = NULL;
static s16 Attrib = 0;
static u16 DeathIndex = 0;
static s16 atkd[N_WEAPON_ATTACK_CLASSES] = {
    [WEAPON_ATTACK_SHORT] = 3000,
    [WEAPON_ATTACK_GENERAL] = 3500,
    [WEAPON_ATTACK_LONG] = 4000,
    [WEAPON_ATTACK_RANGED] = 20000
};
static s16 atkd2[N_WEAPON_ATTACK_CLASSES] = {
    [WEAPON_ATTACK_SHORT] = 2000,
    [WEAPON_ATTACK_GENERAL] = 3000,
    [WEAPON_ATTACK_LONG] = 4000,
    [WEAPON_ATTACK_RANGED] = 20000
};

static ThinkFunc Think1Func[N_THINK1_PROGRAMS];
static ThinkFunc Think2Func[N_THINK2_PROGRAMS];
static ThinkFunc Think3Func[N_THINK3_PROGRAMS];
static ThinkFunc Think4Func[N_THINK4_PROGRAMS];
static ThinkFunc AttackFunc[N_WEAPON_ATTACK_CLASSES];


static void ItemUse(void);
static s16 SuccessionAttack(s32 dist, s16 deg);
static s16 AttackAnimal(void);

enum animal_attack_policy
{
    ANIMAL_ATTACK_TIMER_RESET = 0,
    ANIMAL_ATTACK_RANGE = 2000,
    ANIMAL_ATTACK_AIM = 200,
    ANIMAL_ATTACK_NOTICE_FRAME = 30,
    ANIMAL_ATTACK_FULL_STEER_FRAME = 90
};

enum think_idle_timing
{
    THINK_IDLE_PERIOD = 0x80,
    THINK_IDLE_TURN_LIMIT = 10
};

enum think4_search_timing
{
    THINK4_INITIAL_STEER_TICKS = 30,
    THINK4_ABANDON_TICKS = 91,
    THINK4_ARRIVAL_DISTANCE = 1000
};

#define UPDATE_IDLE_LOOK_PAD(pad_)                                         \
    {                                                                       \
        (pad_) = 0;                                                         \
        if ((Me->actcnt & (THINK_IDLE_PERIOD - 1)) == 0)                   \
        {                                                                   \
            (pad_) = PADLleft;                                              \
            if (Me->actflg != 0)                                           \
            {                                                               \
                (pad_) = PADLright;                                         \
            }                                                               \
            if (Me->actscnt++ > THINK_IDLE_TURN_LIMIT)                     \
            {                                                               \
                Me->actflg = rand() & 1;                                   \
                Me->actscnt = 0;                                           \
                Me->actcnt++;                                              \
            }                                                               \
        }                                                                   \
        else                                                                \
        {                                                                   \
            Me->actcnt++;                                                  \
        }                                                                   \
    }

extern s32 ProbeLevelLow;
extern s32 ProbeLevelHigh;
extern u16 ProbeAttrib[2];
/* Retail declares the pressed word as s16 here; the definition uses u16. */
extern s16 update_pressed_buttons(PADtype *pad, s16 pressed);

void StateTransition(Humanoid *human)
{
    enum
    {
        HALT_TARGET_RANGE = 2000,
        ATTACK_HEIGHT_RANGE = 2000,
        DANGER_PROBE_DELTA = 5000,
        TARGET_ANGLE_LIMIT = 500,
        STEP_DELTA_TOLERANCE = 500,
        HIGH_STEP_DELTA = 6100,
        TERRAIN_CHECK_PERIOD = 90,
        PERIODIC_PROBE_DELTA_MAX = 2200,
        FORWARD_PROBE = 0,
        BACKWARD_PROBE = 1
    };
    s16 pad;
    s16 base_attrib;
    s32 saved_strain_ratio;
    pad_command dash_command;
    s32 player_distance;
    SVECTOR probe_offset;

    saved_strain_ratio = StrainRatio;
    Me = human;
    Pad = &human->pad;
    Attrib = human->attribute;
    base_attrib = Attrib & ~(ATTR_SEARCH | ATTR_PHASE);

    if (Me == StagePlayer)
    {
        PlayerSSR = saved_strain_ratio;
        StrainRatio = STRAIN_NO_THREAT;
    }
    else if (Me->type < KERAI_KATANA)
    {
        s32 input;

        input = 0;
        if (ActionHalt == ACTION_HALT_NONE)
        {
            input = Me->think[PHASE_CALM]();
        }
        update_pressed_buttons(Pad, input);
        return;
    }

    if ((u16)(Me->status - STAT_DAMAGE) <= STAT_DEAD - STAT_DAMAGE)
    {
        if (Me != StagePlayer && Me->life > 0 && StrainRatio > 0)
        {
            StrainRatio = STRAIN_ALERTED;
        }
        update_pressed_buttons(Pad, 0);
        return;
    }

    if (ActionHalt != ACTION_HALT_NONE)
    {
        s32 target_dx;
        s32 target_dz;
        s16 target_direction;

        pad = 0;
        if (Me->type >= PAGE_GUARD)
        {
            target_dx = Me->target->coord.t[0] -
                        Me->locate->vx;
            target_dz = Me->target->coord.t[2] -
                        Me->locate->vz;
            target_direction = GetDirection(target_dx, target_dz,
                                            Me->rotate->vy);
            if (target_direction > Me->turn)
            {
                pad = PADLright;
            }
            else if (-Me->turn > target_direction)
            {
                pad = PADLleft;
            }
            if (SquareRoot0(target_dx * target_dx + target_dz * target_dz) <
                HALT_TARGET_RANGE)
            {
                pad |= PADLdown;
            }
        }
        update_pressed_buttons(Pad, pad);
        return;
    }

    if ((Attrib & ATTR_CUSTOMAI) == 0)
    {
        if (Me == StagePlayer && EmergencyNotice != 0)
        {
            EmergencyNotice--;
            if (EmergencyNotice < 0)
            {
                EmergencyNotice = 0;
            }
        }
        pad = Me->think[PHASE_CALM]();
        update_pressed_buttons(Pad, pad);
        return;
    }

    SR = SearchTarget(Me, &Distance, &Degree);
    if (Me->target == &StagePlayer->model->locate)
    {
        player_distance = Distance;
    }
    else
    {
        GsCOORDINATE2 *previous_target;
        s16 player_direction;
        s16 player_visibility;

        /* Keep the first probe's Distance and Degree. This second probe
         * switches targets only when the player can actually be seen. */
        previous_target = Me->target;
        Me->target = &StagePlayer->model->locate;
        player_visibility = SearchTarget(Me, &player_distance, &player_direction);
        if (player_visibility == SR_SEEN)
        {
            SR = player_visibility;
            Findenemies++;
        }
        else
        {
            Me->target = previous_target;
        }
    }

    if (StagePlayer->active_item == ACTIVE_ITEM_DISGUISE ||
        StagePlayer->active_item == ACTIVE_ITEM_LURE)
    {
        if ((Me->type & PAGE_MASK) != PAGE_BOSS &&
            (Me->type & PAGE_MASK) != PAGE_BEAST &&
            (StagePlayer->active_item != ACTIVE_ITEM_LURE ||
             (Attrib & ATTR_PHASE) != PHASE_ALERT))
        {
            if (EmergencyNotice != 0)
            {
                EmergencyNotice = 0;
            }
            SR = SR_GONE;
        }
    }
    else if (EmergencyNotice != 0)
    {
        if (EmergencyNotice == 1)
        {
            SR = SR_UNSEEN;
        }
        else if ((Attrib & ATTR_PHASE) == PHASE_CALM)
        {
            SR = SR_GLIMPSE;
        }
        else if (SR == SR_GLIMPSE)
        {
            SR = SR_SEEN;
        }
    }

    GetMoveSpeed(&probe_offset, Me->rotate->vy,
                 (s16)(Me->width * 2), 0);
    ProbeLevelLow = GetAreaMapLevel(GlobalAreaMap,
                                    Me->locate->vx + probe_offset.vx,
                                    Me->locate->vy - EYE_HEIGHT,
                                    Me->locate->vz + probe_offset.vz,
                                    AREA_LEVEL_RETURN_DELTA |
                                        AREA_LEVEL_FIRST_HIT |
                                        AREA_LEVEL_REUSE_CACHED);
    {
        u16 forward_attrib;

        forward_attrib = FieldAttrib;
        ProbeLevelHigh = GetAreaMapLevel(GlobalAreaMap,
                                         Me->locate->vx - probe_offset.vx,
                                         Me->locate->vy - EYE_HEIGHT,
                                         Me->locate->vz - probe_offset.vz,
                                         (ProbeAttrib[FORWARD_PROBE] = forward_attrib,
                                          AREA_LEVEL_RETURN_DELTA |
                                              AREA_LEVEL_FIRST_HIT |
                                              AREA_LEVEL_REUSE_CACHED));
    }
    ProbeAttrib[BACKWARD_PROBE] = FieldAttrib;

    switch (Attrib & ATTR_PHASE)
    {
    case PHASE_CALM:
        if (player_distance < StrainRatio)
        {
            StrainRatio = player_distance;
        }
        pad = Me->think[PHASE_CALM]();
        {
            if (SR == SR_SEEN)
            {
                Humanoid *actor;
                s32 actor_life;

                SetNowMotion(Me, MOT_STATE_DRAW, MOTION_MOVE_APPLY);
                if (SetNowMotion(Me, MOT_ACTION_NOTICE, MOTION_MOVE_APPLY) == 0)
                {
                    Sound(Me, CHAR_VOICE_ALERT);
                }
                actor = Me;
                actor_life = actor->life;
                Attrib = base_attrib | PHASE_ALERT;
                actor->chase[HUMANOID_CHASE_X] =
                    actor->chase[HUMANOID_CHASE_Z] = 0;
                if (actor_life > 0)
                {
                    reset_alert_duration();
                    if (Me->target == &StagePlayer->model->locate)
                    {
                        Findenemies++;
                    }
                }
            }
            else if (SR == SR_GLIMPSE)
            {
                if (EmergencyNotice != 0)
                {
                    SetNowMotion(Me, MOT_STATE_DRAW, MOTION_MOVE_APPLY);
                    Me->chase[HUMANOID_CHASE_Z] = 0;
                    Me->chase[HUMANOID_CHASE_X] = 0;
                }
                Attrib = base_attrib | PHASE_SUSPICIOUS;
                Sound(Me, CHAR_VOICE_NOTICE);
            }
        }
        break;

    case PHASE_SUSPICIOUS:
        if (EmergencyNotice != 0 || (Attrib & ATTR_SEARCH) != 0)
        {
            if (StrainRatio > 0)
            {
                StrainRatio = STRAIN_ALERTED;
            }
            if (Attrib & ATTR_SEARCH)
            {
                pad = think_alarm_reaction_();
            }
            else
            {
                pad = Think2confirm();
            }
        }
        else
        {
            if (StrainRatio > 0 || StrainRatio < -player_distance)
            {
                StrainRatio = -player_distance;
            }
            pad = Me->think[PHASE_SUSPICIOUS]();
        }

        if (SR == SR_SEEN || ((Attrib & ATTR_HIT) != 0 && SR > 0))
        {
            Attrib = base_attrib | PHASE_ALERT;
            if ((Attrib & ATTR_WEAPON_DRAWN) == 0)
            {
                SetNowMotion(Me, MOT_STATE_DRAW, MOTION_MOVE_APPLY);
            }
            Me->chase[HUMANOID_CHASE_Z] = 0;
            Me->chase[HUMANOID_CHASE_X] = 0;
            Sound(Me, CHAR_VOICE_ALERT);
            reset_alert_duration();
            if (Me->life > 0 && Me->target == &StagePlayer->model->locate)
            {
                Findenemies++;
            }
        }
        else if (EmergencyNotice < 2 &&
                 (SR == SR_GONE || SR == SR_UNSEEN))
        {
            SetNowMotion(Me, MOT_STATE_SHEATHE, MOTION_MOVE_APPLY);
            Attrib = base_attrib;
        }
        break;

    case PHASE_ALERT:
    {
        if (Attrib & ATTR_WEAPON_DRAWN)
        {
            if (Me->target == &StagePlayer->model->locate)
            {
                StrainRatio = 0;
            }
            else if (StrainRatio > 0)
            {
                StrainRatio = STRAIN_ALERTED;
            }
        }
        else
        {
            StrainRatio = -1;
        }

        if (Attrib & ATTR_SEARCH)
        {
            pad = Me->think[PHASE_ALERT]();
        }
        else
        {
            if ((Attrib & ATTR_WEAPON_DRAWN) == 0)
            {
                SetNowMotion(Me, MOT_STATE_DRAW, MOTION_MOVE_APPLY);
            }
            pad = Think3firstattack();
        }

        if (pad & PADRleft)
        {
            Humanoid *attacker;
            s32 target_dy;

            if (StagePlayer->motion->mid == MOT_DAMAGE_DOWNED)
            {
                pad &= PAD_DIRECTION_BUTTONS;
            }
            else
            {
                attacker = Me;
                target_dy = attacker->target->coord.t[1] -
                            attacker->locate->vy;
                target_dy = target_dy >= 0 ? target_dy : -target_dy;
                if (target_dy >= ATTACK_HEIGHT_RANGE &&
                    WEAPON_ATTACK_CLASS(attacker->wpatk) !=
                        WEAPON_ATTACK_RANGED)
                {
                    pad &= PAD_DIRECTION_BUTTONS;
                }
                else
                {
                    if (rand() % 4 - 2 >= (s32)gNannido)
                    {
                        pad = 0;
                    }
                }
            }
        }

        if (SR == SR_GONE)
        {
            Humanoid *searcher;
            s32 last_seen_z;

            searcher = Me;
            Attrib = base_attrib | ATTR_SEARCH | PHASE_INVESTIGATE;
            searcher->chase[HUMANOID_CHASE_X] =
                searcher->target->coord.t[0];
            last_seen_z = searcher->target->coord.t[2];
            searcher->actscnt = 1;
            searcher->chase[HUMANOID_CHASE_Z] = last_seen_z;
        }

        if (Me->pad_hold == 0)
        {
            if ((pad & PADLdown) &&
                ((ProbeAttrib[BACKWARD_PROBE] & (MAP_DEATH | MAP_WATER)) ||
                 ProbeLevelHigh > DANGER_PROBE_DELTA))
            {
                Me->pad_hold = PAD_HOLD(PADLup, 30);
            }
            if ((pad & PADLup) &&
                ((ProbeAttrib[FORWARD_PROBE] & (MAP_DEATH | MAP_WATER)) ||
                 ProbeLevelLow > DANGER_PROBE_DELTA))
            {
                pad = GotoPosition(0, 0) & (PADLleft | PADLright);
            }
            if (StagePlayer->motion->mid == MOT_SYURI_RECOVER &&
                rand() % (EngageLevel + 1) == 0)
            {
                dash_command = (rand() & 1) ? CMD_DASH_LEFT : CMD_DASH_RIGHT;
                pad = SetCommand(&Me->pad, dash_command);
            }
            break;
        }
        break;
    }

    case PHASE_INVESTIGATE:
        if (StrainRatio > 0)
        {
            StrainRatio = STRAIN_ALERTED;
        }
        pad = Me->think[PHASE_INVESTIGATE]();
        if ((Attrib & ATTR_WALL) && Me->pad_hold == 0)
        {
            Me->pad_hold = Degree > 0 ? PAD_HOLD(PADLright, 8)
                                               : PAD_HOLD(PADLleft, 8);
        }
        if ((Attrib & ATTR_PHASE) == PHASE_ALERT)
        {
            Sound(Me, CHAR_VOICE_ALERT);
            reset_alert_duration();
            if ((Attrib & ATTR_SEARCH) == 0 &&
                Me->target == &StagePlayer->model->locate)
            {
                Findenemies++;
            }
        }
        break;
    }
    if (Me->pad_hold != 0)
    {
        pad = PAD_HOLD_BUTTONS(Me->pad_hold);
        {
            s32 hold_frames;

            hold_frames = PAD_HOLD_FRAMES(Me->pad_hold) - 1;
            if (hold_frames != 0)
            {
                Me->pad_hold = PAD_HOLD(pad, hold_frames);
            }
            else if (pad & (PADLleft | PADLright))
            {
                Me->pad_hold =
                    PAD_HOLD(PADLup, (rand() % 3 + 1) * 30);
            }
            else
            {
                Me->pad_hold = 0;
            }
        }
    }

    {
        Humanoid *actor;

        actor = Me;
        if (actor->status == STAT_HANG)
        {
            s32 target_angle;
            s32 abs_target_angle;

            target_angle = Degree;
            abs_target_angle = target_angle;
            abs_target_angle = abs_target_angle >= 0 ? abs_target_angle
                                                     : -abs_target_angle;
            pad = PADLup;
            if (abs_target_angle >= TARGET_ANGLE_LIMIT)
            {
                pad = PADLdown;
                if ((Attrib & ATTR_PHASE) == PHASE_CALM)
                {
                    pad = PADLup;
                }
                else
                {
                    s32 turn_hold;

                    turn_hold = PAD_HOLD(PADLleft, 15);
                    if (target_angle > 0)
                    {
                        turn_hold = PAD_HOLD(PADLright, 15);
                    }
                    actor->pad_hold = turn_hold;
                }
            }
        }
        else if ((ProbeAttrib[FORWARD_PROBE] & (MAP_DEATH | MAP_WATER)) &&
                 (pad & PADLup))
        {
            pad &= ~PADLup;
        }
        else if ((ProbeAttrib[BACKWARD_PROBE] & (MAP_DEATH | MAP_WATER)) &&
                 (pad & PADLdown))
        {
            pad &= ~PADLdown;
        }
        else
        {
            if (Me->motion->count == 0)
            {
                s32 abs_target_angle;

                abs_target_angle = Degree;
                if (abs_target_angle < 0)
                {
                    abs_target_angle = -abs_target_angle;
                }
                if (abs_target_angle < TARGET_ANGLE_LIMIT &&
                    (Me->think[PHASE_CALM] == Think1ninja ||
                     ((Me->type & PAGE_MASK) == PAGE_NINJA && gNannido != DIFFICULTY_EASY)))
                {
                    s32 current_level;
                    s32 forward_delta;

                    GetMoveSpeed(&probe_offset, Me->rotate->vy,
                                 (s16)(Me->width * 5), 0);
                    current_level = GetAreaMapLevel(
                        GlobalAreaMap,
                        Me->locate->vx,
                        Me->locate->vy - EYE_HEIGHT,
                        Me->locate->vz,
                        AREA_LEVEL_STEP_DOWN |
                            AREA_LEVEL_FIRST_HIT |
                            AREA_LEVEL_REUSE_CACHED);
                    forward_delta = GetAreaMapLevel(
                        GlobalAreaMap,
                        Me->locate->vx + probe_offset.vx,
                        Me->locate->vy - EYE_HEIGHT,
                        Me->locate->vz + probe_offset.vz,
                        AREA_LEVEL_RETURN_DELTA |
                            AREA_LEVEL_FIRST_HIT |
                            AREA_LEVEL_REUSE_CACHED);
                    if (current_level == Me->map.level &&
                        (forward_delta >= 0 ? forward_delta
                                            : -forward_delta) <
                            STEP_DELTA_TOLERANCE)
                    {
                        pad = PADLup | PADRdown;
                    }
                    else if (forward_delta > HIGH_STEP_DELTA)
                    {
                        pad = PADLup | PADRdown;
                    }
                    if (Me->life < 0)
                    {
                        StrainRatio = saved_strain_ratio;
                    }
                    Me->attribute = Attrib;
                    update_pressed_buttons(Pad, pad);
                    return;
                }
            }
            if (GameClock % TERRAIN_CHECK_PERIOD == 0 &&
                (((u16)Me->map.attrib & MAP_DAMAGE) ||
                 ((pad & PADLup) &&
                  ProbeLevelLow <= PERIODIC_PROBE_DELTA_MAX &&
                  ProbeLevelLow != LEVEL_NONE) ||
                 ((pad & PADLdown) &&
                  ProbeLevelHigh <= PERIODIC_PROBE_DELTA_MAX &&
                  ProbeLevelHigh != LEVEL_NONE)))
            {
                pad |= PADRdown;
            }
        }
    }

    if (Me->life < 0)
    {
        StrainRatio = saved_strain_ratio;
    }
    Me->attribute = Attrib;

    update_pressed_buttons(Pad, pad);
}


s16 GotoPosition(s32 vx, s32 vz)
{
    enum
    {
        FORWARD_AIM_TOLERANCE = 500,
        WALL_PROBE_Y_OFFSET = 500,
        SIDESTEP_HOLD_TICKS = 30
    };
    u16 dir;
    s32 turn;
    s32 result;
    s32 adir;

    result = 0;
    if (vx != 0 || vz != 0)
    {
        dir = GetDirection(vx, vz,
                           Me->rotate->vy);
    }
    else
    {
        dir = Degree;
    }
    turn = Me->turn;
    if (turn < (s16)dir)
    {
        result = PADLright;
    }
    else if ((s16)dir < -turn)
    {
        result = (s16)PADLleft;
    }
    adir = (s16)dir;
    if (adir < 0)
    {
        adir = -adir;
    }
    if (adir < FORWARD_AIM_TOLERANCE)
    {
        result |= PADLup;
    }
    if (!(Attrib & ATTR_PHASE))
    {
        if (Attrib & ATTR_WALL)
        {
            s32 cached;

            cached = ProbeLevelLow;
            if (cached != LEVEL_NONE)
            {
                return result;
            }
            if (Me->pad_hold == 0)
            {
                SVECTOR local;
                s32 d1, d2;

                GetMoveSpeed(&local,
                             Me->rotate->vy,
                             0, Me->width);
                d1 = GetAreaMapLevel(GlobalAreaMap,
                                     Me->locate->vx + local.vx,
                                     Me->locate->vy - WALL_PROBE_Y_OFFSET,
                                     Me->locate->vz + local.vz,
                                     AREA_LEVEL_RETURN_DELTA |
                                         AREA_LEVEL_FIRST_HIT |
                                         AREA_LEVEL_REUSE_CACHED);
                d2 = GetAreaMapLevel(GlobalAreaMap,
                                     Me->locate->vx - local.vx,
                                     Me->locate->vy - WALL_PROBE_Y_OFFSET,
                                     Me->locate->vz - local.vz,
                                     AREA_LEVEL_RETURN_DELTA |
                                         AREA_LEVEL_FIRST_HIT |
                                         AREA_LEVEL_REUSE_CACHED);
                /* pad_hold packs (button << 16) | frames: latch a 30-frame
                 * sidestep toward the clearer flank. */
                if ((result & PADLright) && (d1 != cached))
                {
                    d2 = PAD_HOLD(PADLright, SIDESTEP_HOLD_TICKS);
                    Me->pad_hold = d2;
                }
                else if ((result & PADLleft) && (d2 != (u32)LEVEL_NONE))
                {
                    d2 = PAD_HOLD((u32)PADLleft, SIDESTEP_HOLD_TICKS);
                    Me->pad_hold = d2;
                }
                else
                {
                    result = 0;
                }
            }
        }
    }
    return result;
}


s16 ChasetoTarget(s32 length)
{
    enum chase_target_policy
    {
        CHASE_TARGET_AXIS_TOLERANCE = 500,
        CHASE_TARGET_MIN_DISTANCE = 1000
    };
    Humanoid *self;
    long delta_x, delta_z;
    long *chase_offset;
    long offset_x, offset_z;
    short direction;

    self = Me;
    chase_offset = self->chase;
    if (self->target == 0)
    {
        return 0;
    }

    delta_x = self->target->coord.t[0] +
              chase_offset[HUMANOID_CHASE_X] - self->locate->vx;
    delta_z = self->target->coord.t[2] +
              chase_offset[HUMANOID_CHASE_Z] - self->locate->vz;

    if (((delta_x >= 0 ? delta_x : -delta_x) <
             CHASE_TARGET_AXIS_TOLERANCE &&
         (delta_z >= 0 ? delta_z : -delta_z) <
             CHASE_TARGET_AXIS_TOLERANCE) ||
        (Attrib & ATTR_WALL) != 0 || Distance < CHASE_TARGET_MIN_DISTANCE)
    {
        return 0;
    }

    if ((Attrib & (ATTR_HIT | ATTR_PUSH)) != 0 ||
        (chase_offset[HUMANOID_CHASE_X] |
         chase_offset[HUMANOID_CHASE_Z]) == 0)
    {
        direction = rand();
        offset_x = rcos(direction) * length >> FIXED_SHIFT;
        chase_offset[HUMANOID_CHASE_X] = offset_x;
        offset_z = rsin(direction) * length >> FIXED_SHIFT;
        chase_offset[HUMANOID_CHASE_Z] = offset_z;
    }
    return GotoPosition(delta_x, delta_z);
}

void register_character_death(Humanoid *dead)
{
    enum death_witness_policy
    {
        DEATH_WITNESS_VIEW_ANGLE = 900,
        DEATH_WITNESS_RANGE = 20000,
        DEATH_WITNESS_PASSAGE_COMPONENT_LIMIT = 500
    };
    VECTOR delta;
    SVECTOR passage;
    Humanoid *witness;
    s16 passage_scale;
    s16 next_index;
    s16 witness_index;
    s32 alert_duration;
    s32 target_z;

    passage_scale = 1;
    if ((dead->attribute & ATTR_SEARCH) == 0)
    {
        next_index = DeathIndex + 1;
        DeathIndex = next_index;
        witness_index = next_index % Humans;
        witness = HumanGroup[witness_index];
        DeathIndex = witness_index;

        if ((witness->attribute & (ATTR_SUSPEND | ATTR_PHASE)) == 0 &&
            witness->status != STAT_DEAD && witness->status != STAT_DAMAGE &&
            witness->type >= KERAI_KATANA)
        {
            delta.vx = dead->locate->vx - witness->locate->vx;
            delta.vz = dead->locate->vz - witness->locate->vz;
            if (__builtin_abs(GetDirection(delta.vx, delta.vz,
                                           witness->rotate->vy)) <=
                    DEATH_WITNESS_VIEW_ANGLE &&
                SquareRoot0(delta.vx * delta.vx + delta.vz * delta.vz) <=
                    DEATH_WITNESS_RANGE)
            {
                delta.vy = dead->locate->vy - witness->locate->vy -
                           witness->height;

                while (__builtin_abs(delta.vx) >
                           DEATH_WITNESS_PASSAGE_COMPONENT_LIMIT ||
                       __builtin_abs(delta.vy) >
                           DEATH_WITNESS_PASSAGE_COMPONENT_LIMIT ||
                       __builtin_abs(delta.vz) >
                           DEATH_WITNESS_PASSAGE_COMPONENT_LIMIT)
                {
                    passage_scale <<= 1;
                    delta.vx >>= 1;
                    delta.vy >>= 1;
                    delta.vz >>= 1;
                }

                passage.vx = delta.vx;
                passage.vy = delta.vy;
                passage.vz = delta.vz;
                if (GetAreaMapPassage(GlobalAreaMap, witness->locate,
                                      &passage, passage_scale) == 0)
                {
                    RESET_TRIAL_ALERT_DURATION(alert_duration);
                    Sound(witness, CHAR_VOICE_NOTICE);
                    SetNowMotion(witness, MOT_STATE_DRAW, MOTION_MOVE_APPLY);
                    dead->attribute |= ATTR_SEARCH;
                    witness->attribute |= ATTR_SEARCH | PHASE_SUSPICIOUS;
                    witness->chase[HUMANOID_CHASE_X] = dead->locate->vx;
                    target_z = dead->locate->vz;
                    witness->actcnt = 0;
                    witness->actscnt = 0;
                    witness->chase[HUMANOID_CHASE_Z] = target_z;
                }
            }
        }
    }
}

/* At the destination, all four AI phases stop generating input. */
static __inline__ void CompleteTrialEscort(Humanoid *human)
{
    human->think[0] = Think1Func[THINK_BASIC_NONE];
    human->think[1] = Think2Func[THINK_BASIC_NONE];
    human->think[2] = Think3Func[THINK_BASIC_NONE];
    human->think[3] = Think4Func[THINK_BASIC_NONE];
    human->attribute &= ~ATTR_CUSTOMAI;
}

s16 Think1trace(void)
{
    s16 pad;

    if (Me->type == HIME)
    {
        if (GameClock % TRIAL_ESCORT_CHECK_PERIOD == 0 &&
            __builtin_abs(Me->locate->vx -
                HumanGroup[TRIAL_ESCORT_DESTINATION]->locate->vx) <
                    TRIAL_ESCORT_ARRIVAL_RADIUS &&
            __builtin_abs(Me->locate->vz -
                HumanGroup[TRIAL_ESCORT_DESTINATION]->locate->vz) <
                    TRIAL_ESCORT_ARRIVAL_RADIUS &&
            Me->locate->vy == HumanGroup[TRIAL_ESCORT_DESTINATION]->locate->vy)
        {
            CompleteTrialEscort(Me);
            return 0;
        }
        if (StagePlayer->map.height != 0 ||
            StagePlayer->status == STAT_DAMAGE || StagePlayer->status == STAT_DEAD)
        {
            s16 direction;

            direction = GetDirection(StagePlayer->locate->vx - Me->locate->vx,
                                     StagePlayer->locate->vz - Me->locate->vz,
                                     Me->rotate->vy);
            if (direction > Me->turn)
            {
                pad = PADLright;
            }
            else if (direction < -Me->turn)
            {
                pad = -PADLleft;
            }
            else
            {
                pad = 0;
            }
            return pad;
        }
        if (EmergencyNotice != 0)
        {
            if (++Me->actcnt > TRIAL_ESCORT_ALERT_PERIOD)
            {
                Sound(Me, CHAR_VOICE_ACTION_B);
                Me->actcnt = 0;
            }
            else if (Me->actcnt < TRIAL_ESCORT_ALERT_PAUSE)
            {
                return 0;
            }
        }
        else if (__builtin_abs(Me->locate->vx - StagePlayer->locate->vx) >
                     TRIAL_ESCORT_PLAYER_RANGE ||
                 __builtin_abs(Me->locate->vz - StagePlayer->locate->vz) >
                     TRIAL_ESCORT_PLAYER_RANGE)
        {
            if (GameClock % TRIAL_ESCORT_CALL_PERIOD == 0)
            {
                Sound(Me, CHAR_VOICE_ACTION_A);
                SetNowMotion(Me, (rand() & 1) ? MOT_ACTION_FIDGET_A : MOT_ACTION_FIDGET_B,
                             MOTION_MOVE_APPLY);
            }
            return 0;
        }
    }
    else if (Me->type == TONO && HumanGroup[TRIAL_ESCORT_PRINCESS]->type == HIME &&
             GameClock % TRIAL_ESCORT_CHECK_PERIOD == 0 &&
             __builtin_abs(Me->locate->vx - HumanGroup[TRIAL_ESCORT_PRINCESS]->locate->vx) <
                 TRIAL_ESCORT_ARRIVAL_RADIUS &&
             __builtin_abs(Me->locate->vz - HumanGroup[TRIAL_ESCORT_PRINCESS]->locate->vz) <
                 TRIAL_ESCORT_ARRIVAL_RADIUS &&
             Me->locate->vy == HumanGroup[TRIAL_ESCORT_PRINCESS]->locate->vy)
    {
        CompleteTrialEscort(Me);
        return 0;
    }

    if (Attrib & ATTR_TRACE)
    {
        pad = ControlTraceLine(Me);
    }
    else
    {
        pad = Think1random();
    }
    return pad;
}


s16 Think1random(void)
{
    enum random_patrol_policy
    {
        RANDOM_PATROL_SPAN = 10000,
        RANDOM_PATROL_RADIUS = 5000,
        RANDOM_PATROL_ARRIVAL_TOLERANCE = 1000
    };
    s16 input;

    input = 0;
    if (++Me->actcnt == 1)
    {
        Me->chase[HUMANOID_CHASE_X] =
            Me->point[HUMANOID_HOME_X] + rand() % RANDOM_PATROL_SPAN -
            RANDOM_PATROL_RADIUS;
        Me->chase[HUMANOID_CHASE_Z] =
            Me->point[HUMANOID_HOME_Z] + rand() % RANDOM_PATROL_SPAN -
            RANDOM_PATROL_RADIUS;
    }
    else
    {
        s32 delta_x, delta_z;
        VECTOR *position;

        position = Me->locate;
        delta_x = Me->chase[HUMANOID_CHASE_X] - position->vx;
        delta_z = Me->chase[HUMANOID_CHASE_Z] - position->vz;
        if ((__builtin_abs(delta_x) < RANDOM_PATROL_ARRIVAL_TOLERANCE &&
             __builtin_abs(delta_z) < RANDOM_PATROL_ARRIVAL_TOLERANCE) ||
            (Attrib & ATTR_WALL) != 0)
        {
            Me->actcnt = 0;
        }
        else
        {
            input = GotoPosition(delta_x, delta_z);
        }
    }
    return input;
}


s16 Think1ninja(void)
{
    enum
    {
        PATH_CHECK_DELAY = 30,
        SAFE_STEP_DELTA = 500,
        BLOCKING_STEP_DELTA = 6100
    };
    u8 actscnt;
    s16 result;

    result = 0;
    if (Me->status == STAT_JUMP)
    {
        return 0;
    }
    actscnt = Me->actscnt;
    Me->actscnt++;
    if (actscnt > PATH_CHECK_DELAY)
    {
        result = Think1random();
        if (Me->motion->mid == MOT_MOVE &&
            Me->motion->count == 0)
        {
            SVECTOR move;
            s32 d1;
            s32 d2;

            GetMoveSpeed(&move, Me->rotate->vy,
                         (s16)(Me->width * 5), 0);
            d1 = GetAreaMapLevel(GlobalAreaMap, Me->locate->vx,
                                 Me->locate->vy - EYE_HEIGHT,
                                 Me->locate->vz,
                                 AREA_LEVEL_STEP_DOWN | AREA_LEVEL_FIRST_HIT |
                                     AREA_LEVEL_REUSE_CACHED);
            d2 = GetAreaMapLevel(GlobalAreaMap, Me->locate->vx + move.vx,
                                 Me->locate->vy - EYE_HEIGHT,
                                 Me->locate->vz + move.vz,
                                 AREA_LEVEL_RETURN_DELTA | AREA_LEVEL_FIRST_HIT |
                                     AREA_LEVEL_REUSE_CACHED);
            if ((d1 == Me->map.level &&
                 __builtin_abs(d2) < SAFE_STEP_DELTA) ||
                d2 > BLOCKING_STEP_DELTA)
            {
                result = PADLup | PADRdown;
            }
        }
    }
    return result;
}


s16 Think1chase(void)
{
    enum chase_think_policy
    {
        CHASE_SEARCH_DISTANCE = 5000,
        CHASE_FALLBACK_SPAN = 10000,
        CHASE_FALLBACK_RADIUS = 5000
    };
    s16 input;

    input = 0;
    if (++Me->actcnt == 1)
    {
        Humanoid *enemy;

        enemy = GetNearestHumanoid(Me, CHASE_SEARCH_DISTANCE);
        if (enemy != 0)
        {
            Me->chase[HUMANOID_CHASE_X] = enemy->locate->vx;
            Me->chase[HUMANOID_CHASE_Z] = enemy->locate->vz;
        }
        else
        {
            Me->chase[HUMANOID_CHASE_X] =
                Me->point[HUMANOID_HOME_X] + rand() % CHASE_FALLBACK_SPAN -
                CHASE_FALLBACK_RADIUS;
            Me->chase[HUMANOID_CHASE_Z] =
                Me->point[HUMANOID_HOME_Z] + rand() % CHASE_FALLBACK_SPAN -
                CHASE_FALLBACK_RADIUS;
        }
    }
    else
    {
        input = GotoPosition(
            Me->chase[HUMANOID_CHASE_X] - Me->locate->vx,
            Me->chase[HUMANOID_CHASE_Z] - Me->locate->vz);
        if (input == 0)
        {
            input |= PADRleft;
            Me->actcnt = 0;
        }
    }
    return input;
}


s16 Think1target(void)
{
    enum
    {
        TARGET_SCAN_INTERVAL = 32,
        TARGET_NEAR_DISTANCE = 4000,
        TARGET_ALERT_VERTICAL_LIMIT = 3000,
        TARGET_ALERT_HALF_ANGLE = 900,
        TARGET_REACHED_DISTANCE = 200,
        TARGET_FOLLOW_VERTICAL_LIMIT = 2000
    };
    s32 vx;
    s32 vz;
    s32 deg;
    s16 pad;
    s32 distance;

    if (Me->target == NULL)
    {
        UPDATE_IDLE_LOOK_PAD(pad);
        return pad;
    }

    SR = SR_UNSEEN;
    if ((GameClock & (TARGET_SCAN_INTERVAL - 1)) == 0)
    {
        s32 dy;

        vx = StagePlayer->locate->vx - Me->locate->vx;
        vz = StagePlayer->locate->vz - Me->locate->vz;
        dy = StagePlayer->locate->vy - Me->locate->vy;
        distance = SquareRoot0(vx * vx + vz * vz);
        deg = GetDirection(vx, vz, Me->rotate->vy);
        if (distance <= TARGET_NEAR_DISTANCE &&
            __builtin_abs(dy) <= TARGET_ALERT_VERTICAL_LIMIT &&
            __builtin_abs(deg) < TARGET_ALERT_HALF_ANGLE &&
            StagePlayer->active_item != ACTIVE_ITEM_DISGUISE)
        {
            s32 alert_time;

            Me->target = &StagePlayer->model->locate;
            Attrib = (Attrib & (u16)~ATTR_PHASE) | PHASE_ALERT;
            SetNowMotion(Me, MOT_STATE_DRAW, MOTION_MOVE_APPLY);
            Me->chase[HUMANOID_CHASE_Z] = 0;
            Me->chase[HUMANOID_CHASE_X] = 0;
            Sound(Me, CHAR_VOICE_ALERT);
            RESET_TRIAL_ALERT_DURATION(alert_time);
        }
    }

    vx = Me->target->coord.t[0] - Me->locate->vx;
    vz = Me->target->coord.t[2] - Me->locate->vz;
    distance = SquareRoot0(vx * vx + vz * vz);
    if (distance < TARGET_REACHED_DISTANCE)
    {
        return 0;
    }
    if (distance < TARGET_NEAR_DISTANCE &&
        __builtin_abs(Me->target->coord.t[1] - Me->locate->vy) >
            TARGET_FOLLOW_VERTICAL_LIMIT)
    {
        UPDATE_IDLE_LOOK_PAD(pad);
        return pad;
    }
    return GotoPosition(vx, vz);
}

/* actscnt is this handler's state: approach the reported position, then
 * circle there. TRIAL does not spawn the campaign's reinforcement guards. */
typedef u8 alarm_reaction_state;
enum alarm_reaction_state
{
    ALARM_REACTION_APPROACH = 0,
    ALARM_REACTION_CIRCLE = 1
};

enum
{
    ALARM_APPROACH_DISTANCE = 2000,
    ALARM_CIRCLE_PERIOD = 32,
    ALARM_CIRCLE_TURN_BIT = 8,
    ALARM_CIRCLE_TURN_THRESHOLD = 700,
    ALARM_CIRCLE_FORWARD_CHANCE = 5
};


s16 think_alarm_reaction_(void)
{
    s32 x_diff;
    s32 z_diff;
    s16 result;
    alarm_reaction_state state;
    Humanoid *self;

    result = 0;
    x_diff = Me->chase[HUMANOID_CHASE_X] -
             Me->locate->vx;
    z_diff = Me->chase[HUMANOID_CHASE_Z] -
             Me->locate->vz;
    state = Me->actscnt;

    if (state == ALARM_REACTION_APPROACH)
    {
        s32 distance;

        result = GotoPosition(x_diff, z_diff);
        distance = SquareRoot0(x_diff * x_diff + z_diff * z_diff);
        if (distance < ALARM_APPROACH_DISTANCE || (Attrib & ATTR_WALL))
        {
            s32 alertTime;

            RESET_TRIAL_ALERT_DURATION(alertTime);
            Sound(Me, CHAR_VOICE_REACTION);

            self = Me;
            self->actscnt = ALARM_REACTION_CIRCLE;
            self->actcnt = 1;
        }
    }
    else if (state == ALARM_REACTION_CIRCLE)
    {
        u8 count;

        Me->actcnt = (Me->actcnt + 1) & (ALARM_CIRCLE_PERIOD - 1);
        count = Me->actcnt;
        if (count & ALARM_CIRCLE_TURN_BIT)
        {
            if (count != ALARM_CIRCLE_TURN_BIT)
            {
                result = Me->pad.data;
            }
            else
            {
                s32 degree;
                s32 absoluteDegree;

                degree = Degree;
                absoluteDegree = __builtin_abs(degree);
                if (absoluteDegree > ALARM_CIRCLE_TURN_THRESHOLD)
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
                    if (randomValue % ALARM_CIRCLE_FORWARD_CHANCE != 0)
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
    return result;
}





s16 Think3chase(void)
{
    s32 degree;
    u16 result;

    if (Distance < SR_CLEAR_RANGE && SR != SR_GONE)
    {
        SR = SR_NONE;
    }
    if (AttackActionCount + EngageLevel * 30 < GameClock)
    {
        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }
        if (degree < 500)
        {
            if (Distance > 4000)
            {
                result = Me->pad.data;
            }
            else if (Distance > 3000)
            {
                result = SetCommand(&Me->pad, CMD_LUNGE);
            }
            else
            {
                result = PADRleft;
                if (Distance < 2000)
                {
                    result = PADRleft | PADRright;
                }
            }
            AttackActionCount = GameClock + EngageLevel * ATTACK_COOLDOWN_PER_LEVEL;
        }
        else
        {
            result = AttackFunc[WEAPON_ATTACK_CLASS(Me->wpatk)]();
        }
    }
    else
    {
        result = AttackFunc[WEAPON_ATTACK_CLASS(Me->wpatk)]();
    }
    return result;
}


/* Per-range-class engagement distances (retail data: 3000/3500/4000
 * for the melee classes, 20000 for the ranged class). */


s16 Think3attack(void)
{
    enum
    {
        MELEE_CONTINUATION_RANGE = 3000,
        MELEE_CONTINUATION_AIM = 1500,
        RANGED_CONTINUATION_RANGE = 20000,
        RANGED_CONTINUATION_AIM = 500,
        RANGED_NOTICE_CLEAR_RANGE = 14000,
        RANGED_CLOSE_RANGE = 4000,
        CLOSE_ATTACK_AIM = 1000,
        COMBINATION_ATTACK_RANGE = 2000,
        RANGED_ATTACK_CHANCE_SCALE = 4,
        LUNGE_AIM = 100,
        LUNGE_RANGE_MARGIN = 1000,
        MIDRANGE_ATTACK_AIM = 1200,
        ADVANCE_RANGE_MARGIN = 500,
        RANDOM_DASH_DIVISOR = 3,
        RANDOM_TAUNT_DIVISOR = 30
    };
    s16 close_range;
    s16 input;
    weapon_attack_class attack_class;

    input = 0;
    attack_class = WEAPON_ATTACK_CLASS(Me->wpatk);

    if (Me->status == STAT_ATTACK)
    {
        if (attack_class != WEAPON_ATTACK_RANGED)
        {
            input = SuccessionAttack(MELEE_CONTINUATION_RANGE,
                                     MELEE_CONTINUATION_AIM);
        }
        else
        {
            input = SuccessionAttack(RANGED_CONTINUATION_RANGE,
                                     RANGED_CONTINUATION_AIM);
        }
        return input;
    }

    if (SR != SR_GONE &&
        ((attack_class == WEAPON_ATTACK_RANGED &&
          Distance < RANGED_NOTICE_CLEAR_RANGE) ||
         Distance < SR_CLEAR_RANGE))
    {
        SR = SR_NONE;
    }

    if ((s16)((N_WEAPON_ATTACK_CLASSES - attack_class) * Me->turn) < Degree)
    {
        input = PADLright;
    }
    else if (Degree <
             -(s16)((N_WEAPON_ATTACK_CLASSES - attack_class) * Me->turn))
    {
        input = PADLleft;
    }

    if (attack_class != WEAPON_ATTACK_RANGED)
    {
        close_range = atkd[attack_class] / 2;
    }
    else
    {
        close_range = RANGED_CLOSE_RANGE;
    }

    if (Distance < close_range)
    {
        if (__builtin_abs(Degree) < CLOSE_ATTACK_AIM &&
            Me->motion->count == 0)
        {
            if (Distance < COMBINATION_ATTACK_RANGE)
            {
                if (rand() % (EngageLevel + 1) != 0)
                {
                    input |= PADRleft;
                }
                else
                {
                    input = PADRleft | PADRright;
                }
            }
            else
            {
                input |= PADRleft;
            }
        }
        else
        {
            input |= PADLdown;
        }
    }
    else if (Distance < atkd[attack_class])
    {
        if (attack_class == WEAPON_ATTACK_RANGED)
        {
            if (input == 0 &&
                rand() % (EngageLevel * RANGED_ATTACK_CHANCE_SCALE) == 0)
            {
                input = PADRleft;
            }
        }
        else if (__builtin_abs(Degree) < LUNGE_AIM &&
                 atkd[attack_class] - LUNGE_RANGE_MARGIN < Distance)
        {
            input = SetCommand(&Me->pad, CMD_LUNGE);
        }
        else if (Me->motion->count == 0 &&
                 __builtin_abs(Degree) < MIDRANGE_ATTACK_AIM)
        {
            input |= PADRleft;
        }
        else if (close_range + ADVANCE_RANGE_MARGIN < Distance)
        {
            input |= PADLup;
        }
    }
    else if (Me->status == STAT_ENGAGE)
    {
        if (StagePlayer->status == STAT_SYURI)
        {
            pad_command command;
            s32 random;

            random = rand();
            command = CMD_DASH_RIGHT;
            if ((random & 1) != 0)
            {
                command = CMD_DASH_LEFT;
            }
            input = SetCommand(&Me->pad, command);
        }
        else if (Me->motion->count == 0 &&
                 rand() % RANDOM_DASH_DIVISOR == 0)
        {
            input = SetCommand(&Me->pad, CMD_DASH_FORWARD);
        }
        else
        {
            ItemUse();
        }
    }

    if (Me->motion->count == 0 &&
        rand() % RANDOM_TAUNT_DIVISOR == 0 &&
        Me->status == STAT_ENGAGE)
    {
        SetNowMotion(Me, MOT_ATTACK_TAUNT, MOTION_MOVE_APPLY);
    }

    return input;
}

enum
{
    ESCAPE_CLEAR_RANGE = 16500,
    ESCAPE_TURN_ANGLE = 1000
};

static inline s32 GetEscapeTurnHold(Humanoid *human, s32 direction)
{
    s32 frames;

    frames = ESCAPE_TURN_ANGLE / human->turn;
    if (direction > 0)
    {
        return PAD_HOLD(PADLright, frames);
    }
    return PAD_HOLD(PADLleft, frames);
}

s16 Think3escape(void)
{
    s16 input;
    s32 absolute_degree;

    input = 0;
    if (Distance < ESCAPE_CLEAR_RANGE && SR != SR_GONE)
    {
        SR = SR_NONE;
    }
    if (Degree > 0)
    {
        input = (s16)PADLleft;
    }
    else if (Degree < 0)
    {
        input = PADLright;
    }
    absolute_degree = __builtin_abs(Degree);
    if (absolute_degree < ESCAPE_TURN_ANGLE)
    {
        input |= PADLdown;
    }
    else
    {
        input |= PADLup;
    }
    if (Attrib & ATTR_WALL)
    {
        Humanoid *human;
        s32 direction;
        s32 absolute_direction;

        direction = Degree;
        absolute_direction = __builtin_abs(direction);
        if (absolute_direction > ESCAPE_TURN_ANGLE && Me->pad_hold == 0)
        {
            human = Me;
            human->pad_hold = GetEscapeTurnHold(human, direction);
        }
    }
    return input;
}


s16 Think3area(void)
{
    enum
    {
        AREA_ENGAGE_RANGE = 4000
    };
    s16 pad = 0;
    s32 xx;
    s32 zz;
    s32 dist;
    if (Me->status == STAT_ATTACK)
    {
        return SuccessionAttack(AREA_ENGAGE_RANGE, 500);
    }

    if (Distance < SR_CLEAR_RANGE && SR != SR_GONE)
    {
        SR = SR_NONE;
    }

    if (WEAPON_ATTACK_CLASS(Me->wpatk) == WEAPON_ATTACK_RANGED)
    {
        return Think3attack();
    }

    xx = Me->point[HUMANOID_HOME_X] - Me->locate->vx;
    zz = Me->point[HUMANOID_HOME_Z] - Me->locate->vz;
    dist = SquareRoot0(xx * xx + zz * zz);

    if (Me->actflg != 0)
    {
        pad = AttackFunc[WEAPON_ATTACK_CLASS(Me->wpatk)]();
        if (Distance < AREA_ENGAGE_RANGE)
        {
            Me->actcnt++;
            if (Me->actcnt == 0 && dist > AREA_ENGAGE_RANGE)
            {
                Me->actflg = 0;
            }
        }
    }
    else
    {
        if ((Attrib & ATTR_HIT) != 0)
        {
            Me->actflg = 1;
        }

        if (dist < 2000)
        {
            s32 degree;

            if ((Me->motion->count & 7) != 0)
            {
                pad = Me->pad.data;
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
                SetNowMotion(Me, MOT_ATTACK_TAUNT,
                             MOTION_MOVE_APPLY); /* taunt */
                Me->actflg = 1;
            }

            if (Distance < AREA_ENGAGE_RANGE)
            {
                degree = __builtin_abs(Degree);

                if (degree < 100)
                {
                    pad = SetCommand(&Me->pad, CMD_LUNGE);
                }
                else if (degree < 1000)
                {
                    pad |= PADRleft;
                }
                Me->actflg = 1;
            }
        }
        else
        {
            pad = GotoPosition(xx, zz);
            if ((Attrib & ATTR_WALL) != 0)
            {
                Me->actflg = 1;
            }
            if (Me->motion->count == 0 && Distance < 3000)
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
        }
    }

    return pad;
}


s16 Think3hitaway(void)
{
    u16 pad;

    if (Distance < SR_CLEAR_RANGE && SR != SR_GONE)
    {
        SR = SR_NONE;
    }
    if (Me->status == STAT_ATTACK)
    {
        Me->actflg = 0;
        Me->chase[HUMANOID_CHASE_Z] = 0;
        Me->chase[HUMANOID_CHASE_X] = 0;
        return SuccessionAttack(3000, 1500);
    }
    else if (Me->actflg != 0)
    {
        pad = AttackFunc[WEAPON_ATTACK_CLASS(Me->wpatk)]();
    }
    else
    {
        if (__builtin_abs(Degree) < 1000)
        {
            pad = GotoPosition(0, 0);
            pad = (pad & (PADLleft | PADLdown | PADLright)) | PADLdown;
        }
        else
        {
            pad = ChasetoTarget(5000);
        }
        if (Distance < 2000)
        {
            if (rand() % 30 == 0)
            {
                pad |= PADRdown;
            }
        }
        if (Distance > 4000 || (Attrib & ATTR_WALL))
        {
            Me->actflg = 1;
        }
    }
    return pad;
}


/* Per-range-class first-attack distances, indexed by weapon attack class
 * (same shape as Think3attack.c's atkd table). */
s16 Think3firstattack(void)
{
    enum
    {
        RANGED_FIRST_ATTACK_AIM = 100
    };
    s16 input;
    weapon_attack_class attack_class;
    s32 absolute_degree;

    input = GotoPosition(0, 0);
    if (Distance < SR_CLEAR_RANGE && SR != SR_GONE)
    {
        SR = SR_NONE;
    }
    if ((Me->type & PAGE_MASK) == PAGE_CIVILIAN)
    {
        Attrib |= ATTR_SEARCH;
    }
    attack_class = WEAPON_ATTACK_CLASS(Me->wpatk);
    if (attack_class == WEAPON_ATTACK_RANGED)
    {
        input &= PAD_TURN_BUTTONS_SIGNED;
        absolute_degree = __builtin_abs(Degree);
        if (absolute_degree > RANGED_FIRST_ATTACK_AIM)
        {
            return input;
        }
    }
    if (Distance < atkd2[attack_class])
    {
        input |= PADRleft;
        Attrib |= ATTR_SEARCH;
    }
    return input;
}


static void ItemUse(void)
{
    enum
    {
        SHURIKEN_AIM_TOLERANCE = 100,
        FIRE_AIM_TOLERANCE = 300
    };
    Humanoid *me;
    s16 id;

    if (Me->motion->count != 0)
    {
        return;
    }
    if (Me->status != STAT_ENGAGE)
    {
        return;
    }

    if (Me->item[ITEM_KUSURI] != 0 &&
        Me->life < Me->lifemax / 3)
    {
        ReqItemDefault(Me, ITEM_KUSURI);
        return;
    }

    me = Me;
    if (me->item[ITEM_SHURIKEN] == 0 ||
        __builtin_abs(Degree) >= SHURIKEN_AIM_TOLERANCE)
    {
        if (me->item[ITEM_FIRE] == 0 ||
            __builtin_abs(Degree) >= FIRE_AIM_TOLERANCE)
        {
            return;
        }
        id = MOT_ITEM_THROW;
    }
    else
    {
        id = MOT_SYURI;
    }

    SetNowMotion(me, id, MOTION_MOVE_APPLY);
}


static __inline__ s16 SuccessionAttack(s32 dist, s16 deg)
{
    s16 buttons;

    buttons = 0;
    if (Me->motion->count !=
        BattleDB[Me->warid].contfrm)
    {
        return 0;
    }
    if ((Distance < dist && __builtin_abs((s32)Degree) < deg) ||
        rand() % (EngageLevel + 1) == 0)
    {
        if (Degree > 300)
        {
            buttons = PADLright;
        }
        else
        {
            if (Degree < -300)
            {
                /* The signed view keeps PADLleft in addiu's immediate range. */
                buttons = (s16)PADLleft;
            }
        }
        buttons |= PADRleft;
    }
    return buttons;
}


static short AttackShort(void)
{
    enum
    {
        SHORT_ATTACK_POINT_BLANK_DISTANCE = 1000,
        SHORT_ATTACK_CLOSE_DISTANCE = 1500,
        SHORT_ATTACK_CONTINUATION_DISTANCE = 2000,
        SHORT_ATTACK_ACQUIRE_DISTANCE = 2500,
        SHORT_ATTACK_ADVANCE_DISTANCE = 3000,
        SHORT_ATTACK_TACTIC_DISTANCE = 3500,
        SHORT_ATTACK_REENGAGE_DISTANCE = 4000,
        SHORT_ATTACK_RETREAT_DISTANCE = 5000,
        SHORT_ATTACK_PRECISE_AIM = 100,
        SHORT_ATTACK_TACTIC_AIM = 200,
        SHORT_ATTACK_EVADE_ANGLE = 300,
        SHORT_ATTACK_STEER_ANGLE = 500,
        SHORT_ATTACK_AIM = 1000,
        SHORT_ATTACK_OUTER_AIM = 1500,
        SHORT_ATTACK_RETREAT_CHANCE = 5,
        SHORT_ATTACK_LUNGE_CHANCE = 30,
        SHORT_ATTACK_BACKOFF_CHANCE = 3
    };
    MotionManager *motion;
    s16 input;

    input = 0;
    if ((Me->type & PAGE_MASK) == PAGE_BEAST)
    {
        return AttackAnimal();
    }

    if (Me->status == STAT_ATTACK)
    {
        return SuccessionAttack(SHORT_ATTACK_CONTINUATION_DISTANCE,
                                SHORT_ATTACK_AIM);
    }

    if (Me->status == STAT_JUMP)
    {
        return 0;
    }

    motion = Me->motion;
    if (motion->mid == MOT_ENGAGE)
    {
        return (rand() % (EngageLevel + 1) != 0) ? PADLdown : 0;
    }

    if (Me->actmode == MELEE_ATTACK_CLOSING)
    {
        s32 raw_degree;
        s32 abs_degree;
        s32 target_in_attack_arc;

        target_in_attack_arc = Distance < SHORT_ATTACK_ACQUIRE_DISTANCE;
        if (target_in_attack_arc)
        {
            raw_degree = Degree;
            abs_degree = (raw_degree >= 0) ? raw_degree : -raw_degree;
            target_in_attack_arc = abs_degree < SHORT_ATTACK_OUTER_AIM;
        }

        if (target_in_attack_arc)
        {
            if ((motion->count &
                 (MELEE_ATTACK_DECISION_PERIOD - 1)) != 0)
            {
                return 0;
            }
            if (raw_degree > SHORT_ATTACK_STEER_ANGLE)
            {
                input = PADLright;
            }
            else if (raw_degree < -SHORT_ATTACK_STEER_ANGLE)
            {
                input = PADLleft;
            }
            input |= PADRleft;
            Me->actmode = MELEE_ATTACK_ENGAGED;
        }
        else
        {
            input = ChasetoTarget(SHORT_ATTACK_CONTINUATION_DISTANCE);
            if (input == 0)
            {
                Me->actmode = MELEE_ATTACK_ENGAGED;
            }
            if (Distance > SHORT_ATTACK_REENGAGE_DISTANCE)
            {
                abs_degree = Degree;
                if (abs_degree < 0)
                {
                    abs_degree = -abs_degree;
                }
                if (abs_degree < SHORT_ATTACK_PRECISE_AIM &&
                    rand() % SHORT_ATTACK_RETREAT_CHANCE == 0)
                {
                    input = PADLup | PADRdown;
                }
            }
            if ((Attrib & ATTR_HIT) != 0)
            {
                Me->actmode = MELEE_ATTACK_ENGAGED;
            }
        }
    }

    else if ((motion->count & (MELEE_ATTACK_DECISION_PERIOD - 1)) != 0)
    {
        s32 raw_degree;
        s32 abs_degree;

        input = Me->pad.data;
        if (Distance < SHORT_ATTACK_CLOSE_DISTANCE)
        {
            raw_degree = Degree;
            abs_degree = (raw_degree >= 0) ? raw_degree : -raw_degree;
            if (abs_degree < SHORT_ATTACK_AIM)
            {
                input = PADLdown;
            }
            else if (abs_degree > SHORT_ATTACK_OUTER_AIM)
            {
                if (Distance < SHORT_ATTACK_POINT_BLANK_DISTANCE)
                {
                    input = SetCommand(&Me->pad, CMD_DASH_FORWARD);
                }
                else
                {
                    input = PADLup;
                }
            }
            else if (rand() % SHORT_ATTACK_LUNGE_CHANCE == 0)
            {
                input = SetCommand(&Me->pad, CMD_LUNGE);
            }
        }
    }
    else if (Distance > SHORT_ATTACK_REENGAGE_DISTANCE)
    {
        Humanoid *human;

        Me->actmode = MELEE_ATTACK_CLOSING;
        human = Me;
        Me->chase[HUMANOID_CHASE_Z] = 0;
        human->chase[HUMANOID_CHASE_X] = 0;
        ItemUse();
        if (Distance > SHORT_ATTACK_RETREAT_DISTANCE)
        {
            input = PADLup | PADRdown;
        }
    }
    else
    {
        if ((Attrib & ATTR_WALL) != 0)
        {
            Me->actmode = MELEE_ATTACK_CLOSING;
        }

        if (Degree > SHORT_ATTACK_STEER_ANGLE)
        {
            input = PADLright;
        }
        else if (Degree < -SHORT_ATTACK_STEER_ANGLE)
        {
            input = PADLleft;
        }

        if (Distance > SHORT_ATTACK_CLOSE_DISTANCE &&
            Distance < SHORT_ATTACK_REENGAGE_DISTANCE)
        {
            s32 attack_abs_degree;

            attack_abs_degree = Degree;
            if (attack_abs_degree < 0)
            {
                attack_abs_degree = -attack_abs_degree;
            }
            if (attack_abs_degree < SHORT_ATTACK_AIM &&
                rand() % (EngageLevel + 1) == 0 &&
                GameClock > AttackActionCount)
            {
                AttackActionCount = GameClock + EngageLevel * ATTACK_COOLDOWN_PER_LEVEL;
                if (rand() % SHORT_ATTACK_BACKOFF_CHANCE == 0)
                {
                    input = PADLdown;
                }
                return input | PADRleft;
            }
        }

        {
            s32 raw_degree;
            s32 abs_degree;

            raw_degree = Degree;
            abs_degree = (raw_degree >= 0) ? raw_degree : -raw_degree;
            if (abs_degree > SHORT_ATTACK_OUTER_AIM)
            {
                input |= PADLdown;
            }
            else if (Distance > SHORT_ATTACK_ADVANCE_DISTANCE)
            {
                if (abs_degree < SHORT_ATTACK_TACTIC_AIM &&
                    Distance > SHORT_ATTACK_TACTIC_DISTANCE)
                {
                    if ((rand() & 1) != 0)
                    {
                        input = SetCommand(&Me->pad, CMD_DASH_FORWARD);
                    }
                    else if ((rand() & 1) != 0)
                    {
                        input = SetCommand(&Me->pad, CMD_LUNGE);
                    }
                    else
                    {
                        ItemUse();
                    }
                }
                else
                {
                    input |= PADLup;
                }
            }
            else if (Distance < SHORT_ATTACK_CLOSE_DISTANCE)
            {
                if (raw_degree > SHORT_ATTACK_EVADE_ANGLE)
                {
                    input = SetCommand(&Me->pad, CMD_DASH_LEFT);
                }
                else if (raw_degree < -SHORT_ATTACK_EVADE_ANGLE)
                {
                    input = SetCommand(&Me->pad, CMD_DASH_RIGHT);
                }
                else if (Distance >= SHORT_ATTACK_POINT_BLANK_DISTANCE)
                {
                    input |= PADRleft;
                }
                else
                {
                    input = PADRleft | PADRright;
                    if ((rand() & 1) != 0)
                    {
                        input = PADLdown | PADRdown;
                    }
                }
            }
            else if ((rand() & 1) != 0)
            {
                if (Degree > SHORT_ATTACK_PRECISE_AIM)
                {
                    input = SetCommand(&Me->pad, CMD_DASH_RIGHT);
                }
                else if (Degree < -SHORT_ATTACK_PRECISE_AIM)
                {
                    input = SetCommand(&Me->pad, CMD_DASH_LEFT);
                }
                else
                {
                    input = SetCommand(&Me->pad, CMD_DASH_BACKWARD);
                }
            }
        }
    }

    return input;
}


static short AttackGeneral(void)
{
    enum general_attack_policy
    {
        GENERAL_ATTACK_POINT_BLANK_DISTANCE = 1000,
        GENERAL_ATTACK_CLOSE_DISTANCE = 2000,
        GENERAL_ATTACK_ENGAGE_DISTANCE = 3000,
        GENERAL_ATTACK_ACTION_DISTANCE = 4000,
        GENERAL_ATTACK_CHASE_DISTANCE = 5000,
        GENERAL_ATTACK_PRECISE_AIM = 100,
        GENERAL_ATTACK_CONTINUATION_AIM = 500,
        GENERAL_ATTACK_CLOSE_AIM = 1000,
        GENERAL_ATTACK_ACTION_AIM = 1200,
        GENERAL_ATTACK_OUTER_AIM = 1500,
        GENERAL_ATTACK_RECOVERY_CHANCE = 5,
        GENERAL_ATTACK_BACKOFF_CHANCE = 3
    };
    enum general_close_attack_choice
    {
        GENERAL_CLOSE_ATTACK_BACKFLIP,
        GENERAL_CLOSE_ATTACK_CROUCH_STRIKE,
        GENERAL_CLOSE_ATTACK_DASH_BACKWARD,
        GENERAL_CLOSE_ATTACK_STRIKE,
        N_GENERAL_CLOSE_ATTACK_CHOICES
    };
    s16 input;

    input = 0;
    if (Me->status == STAT_ATTACK)
    {
        return SuccessionAttack(GENERAL_ATTACK_CLOSE_DISTANCE,
                                GENERAL_ATTACK_CONTINUATION_AIM);
    }

    if (Me->status == STAT_JUMP)
    {
        return 0;
    }

    if (Me->motion->mid == MOT_ENGAGE)
    {
        return (rand() % (EngageLevel + 1) != 0) ? PADLdown : 0;
    }

    if (Me->actmode == MELEE_ATTACK_CLOSING)
    {
        s32 aim_error;

        input = ChasetoTarget(GENERAL_ATTACK_ENGAGE_DISTANCE);
        if (input == 0 || (Attrib & ATTR_HIT) != 0)
        {
            Me->actmode = MELEE_ATTACK_ENGAGED;
        }
        if (Distance > GENERAL_ATTACK_CHASE_DISTANCE)
        {
            aim_error = Degree;
            if (aim_error < 0)
            {
                aim_error = -aim_error;
            }
            if (aim_error < GENERAL_ATTACK_PRECISE_AIM &&
                rand() % GENERAL_ATTACK_RECOVERY_CHANCE == 0)
            {
                input = PADLup | PADRdown;
            }
        }
        return input;
    }

    if ((Me->motion->count &
         (MELEE_ATTACK_DECISION_PERIOD - 1)) != 0)
    {
        s32 raw_degree;
        s32 aim_error;

        input = Me->pad.data;
        if (Distance < GENERAL_ATTACK_CLOSE_DISTANCE)
        {
            raw_degree = Degree;
            aim_error = (raw_degree >= 0) ? raw_degree : -raw_degree;
            if (aim_error < GENERAL_ATTACK_CLOSE_AIM)
            {
                input = PADLdown;
            }
            else if (aim_error > GENERAL_ATTACK_OUTER_AIM)
            {
                input = PADLup;
            }
        }
        return input;
    }

    if (Distance > GENERAL_ATTACK_CHASE_DISTANCE)
    {
        Humanoid *actor;

        Me->actmode = MELEE_ATTACK_CLOSING;
        actor = Me;
        Me->chase[HUMANOID_CHASE_Z] = 0;
        actor->chase[HUMANOID_CHASE_X] = 0;
        ItemUse();
        return 0;
    }

    if ((Attrib & ATTR_WALL) != 0)
    {
        Me->actmode = MELEE_ATTACK_CLOSING;
    }

    if (Degree > GENERAL_ATTACK_CONTINUATION_AIM)
    {
        input = PADLright;
    }
    else if (Degree < -GENERAL_ATTACK_CONTINUATION_AIM)
    {
        input = PADLleft;
    }

    if (Distance > GENERAL_ATTACK_POINT_BLANK_DISTANCE &&
        Distance < GENERAL_ATTACK_ENGAGE_DISTANCE)
    {
        s32 aim_error;

        aim_error = Degree;
        if (aim_error < 0)
        {
            aim_error = -aim_error;
        }
        if (aim_error < GENERAL_ATTACK_ACTION_AIM &&
            rand() % (EngageLevel + 1) == 0 &&
            GameClock > AttackActionCount)
        {
            AttackActionCount = GameClock + EngageLevel * ATTACK_COOLDOWN_PER_LEVEL;
            if (rand() % GENERAL_ATTACK_BACKOFF_CHANCE == 0)
            {
                input = PADLdown;
            }
            return input | PADRleft;
        }
    }

    {
        s32 aim_error;

        aim_error = Degree;
        if (aim_error < 0)
        {
            aim_error = -aim_error;
        }

        if (aim_error > GENERAL_ATTACK_CLOSE_AIM ||
            Distance < GENERAL_ATTACK_CLOSE_DISTANCE)
        {
            if (Distance < GENERAL_ATTACK_POINT_BLANK_DISTANCE)
            {
                switch (rand() % N_GENERAL_CLOSE_ATTACK_CHOICES)
                {
                case GENERAL_CLOSE_ATTACK_BACKFLIP:
                    input = PADLdown | PADRdown;
                    break;
                case GENERAL_CLOSE_ATTACK_CROUCH_STRIKE:
                    input = PADRleft | PADRright;
                    break;
                case GENERAL_CLOSE_ATTACK_DASH_BACKWARD:
                    input = SetCommand(&Me->pad, CMD_DASH_BACKWARD);
                    break;
                case GENERAL_CLOSE_ATTACK_STRIKE:
                    input |= PADRleft;
                    break;
                default:
                    break;
                }
            }
            else
            {
                input |= PADLdown;
            }
            return input;
        }

        if (Distance > GENERAL_ATTACK_ENGAGE_DISTANCE)
        {
            input |= PADLup;
            if (Distance > GENERAL_ATTACK_ACTION_DISTANCE)
            {
                if ((rand() & 1) != 0)
                {
                    input = SetCommand(&Me->pad, CMD_DASH_FORWARD);
                }
                else
                {
                    aim_error = Degree;
                    if (aim_error < 0)
                    {
                        aim_error = -aim_error;
                    }
                    if (aim_error < GENERAL_ATTACK_CONTINUATION_AIM)
                    {
                        input = SetCommand(&Me->pad, CMD_LUNGE);
                    }
                    else
                    {
                        ItemUse();
                    }
                }
            }
            return input;
        }

        if ((rand() & 1) != 0)
        {
            if (Degree > GENERAL_ATTACK_PRECISE_AIM)
            {
                input = SetCommand(&Me->pad, CMD_DASH_RIGHT);
            }
            else if (Degree < -GENERAL_ATTACK_PRECISE_AIM)
            {
                input = SetCommand(&Me->pad, CMD_DASH_LEFT);
            }
        }
    }

    return input;
}


static short AttackLong(void)
{
    enum long_attack_policy
    {
        LONG_ATTACK_POINT_BLANK_DISTANCE = 1000,
        LONG_ATTACK_ENGAGE_DISTANCE = 3000,
        LONG_ATTACK_ACTION_DISTANCE = 4000,
        LONG_ATTACK_CHASE_DISTANCE = 5000,
        LONG_ATTACK_PRECISE_AIM = 50,
        LONG_ATTACK_TURN_ANGLE = 300,
        LONG_ATTACK_CLOSE_AIM = 500,
        LONG_ATTACK_EVASIVE_AIM = 1000,
        LONG_ATTACK_OUTER_AIM = 1500,
        LONG_ATTACK_RECOVERY_CHANCE = 5
    };
    s16 raw_degree;
    s16 pad;
    s32 aim_error;

    pad = 0;
    if (Me->status == STAT_ATTACK)
    {
        return SuccessionAttack(LONG_ATTACK_ENGAGE_DISTANCE,
                                LONG_ATTACK_OUTER_AIM);
    }

    if (Me->status == STAT_JUMP)
    {
        return 0;
    }

    if (Me->motion->mid == MOT_ENGAGE)
    {
        return (rand() % (EngageLevel + 1) != 0) ? PADLdown : 0;
    }

    if (Me->actmode == MELEE_ATTACK_CLOSING)
    {
        pad = ChasetoTarget(LONG_ATTACK_ENGAGE_DISTANCE);
        if (pad == 0 || (Attrib & ATTR_HIT) != 0)
        {
            Me->actmode = MELEE_ATTACK_ENGAGED;
        }
        if (Me->motion->count == 0 &&
            rand() % LONG_ATTACK_RECOVERY_CHANCE == 0)
        {
            pad = PADLup | PADRdown;
        }
        return pad;
    }

    if ((Me->motion->count &
         (MELEE_ATTACK_DECISION_PERIOD - 1)) != 0)
    {
        s32 tracking_error;

        pad = Me->pad.data;
        if (Distance < LONG_ATTACK_ENGAGE_DISTANCE)
        {
            raw_degree = Degree;
            tracking_error = (raw_degree >= 0) ? raw_degree : -raw_degree;
            if (tracking_error < LONG_ATTACK_CLOSE_AIM)
            {
                pad = PADLdown;
            }
            else if (tracking_error > LONG_ATTACK_OUTER_AIM)
            {
                pad = PADLup;
            }
        }
        return pad;
    }

    if (Distance > LONG_ATTACK_CHASE_DISTANCE)
    {
        Humanoid *me;

        Me->actmode = MELEE_ATTACK_CLOSING;
        me = Me;
        Me->chase[HUMANOID_CHASE_Z] = 0;
        me->chase[HUMANOID_CHASE_X] = 0;
        ItemUse();
        return 0;
    }

    if ((Attrib & ATTR_WALL) != 0)
    {
        Me->actmode = MELEE_ATTACK_CLOSING;
    }

    if (Degree > LONG_ATTACK_TURN_ANGLE)
    {
        pad = PADLright;
    }
    else if (Degree < -LONG_ATTACK_TURN_ANGLE)
    {
        pad = PADLleft;
    }

    if (Distance > LONG_ATTACK_ENGAGE_DISTANCE &&
        Distance < LONG_ATTACK_ACTION_DISTANCE)
    {
        if (rand() % (EngageLevel + 1) == 0)
        {
            AttackActionCount = GameClock +
                                EngageLevel * ATTACK_COOLDOWN_PER_LEVEL;
            return pad | PADRleft;
        }
    }

    raw_degree = Degree;
    aim_error = (raw_degree >= 0) ? raw_degree : -raw_degree;

    if (aim_error > LONG_ATTACK_EVASIVE_AIM ||
        Distance < LONG_ATTACK_ENGAGE_DISTANCE)
    {
        if (Distance < LONG_ATTACK_POINT_BLANK_DISTANCE)
        {
            pad = PADRleft | PADRright;
            if ((rand() & 1) != 0)
            {
                pad = PADLdown | PADRdown;
            }
        }
        else
        {
            pad = PADLright | PADRleft;
            if ((rand() & 1) != 0)
            {
                pad = PADLleft | PADRleft;
            }
        }
        return pad;
    }

    if (Distance <= LONG_ATTACK_ACTION_DISTANCE)
    {
        return pad;
    }

    if (aim_error < LONG_ATTACK_PRECISE_AIM)
    {
        pad = SetCommand(&Me->pad, CMD_LUNGE);
    }
    else if (Me->motion->count != 0)
    {
        pad |= PADLup;
    }
    else if (raw_degree > LONG_ATTACK_PRECISE_AIM)
    {
        pad = SetCommand(&Me->pad, CMD_DASH_RIGHT);
    }
    else if (raw_degree < -LONG_ATTACK_PRECISE_AIM)
    {
        pad = SetCommand(&Me->pad, CMD_DASH_LEFT);
    }
    else if ((rand() & 1) != 0)
    {
        if ((rand() & 1) != 0)
        {
            pad = SetCommand(&Me->pad, CMD_DASH_FORWARD);
        }
        else
        {
            pad = PADRleft | PADRdown;
        }
    }
    else
    {
        ItemUse();
    }
    return pad;
}


static short AttackIndirect(void)
{
    enum indirect_attack_policy
    {
        INDIRECT_CONTINUATION_AIM = 500,
        INDIRECT_CLOSE_RANGE = 5000,
        INDIRECT_CLOSE_AIM = 1000,
        INDIRECT_RETREAT_DISTANCE = 1000,
        INDIRECT_ADVANCE_DISTANCE = 3000,
        INDIRECT_FIRE_AIM = 200,
        INDIRECT_ATTACK_CHANCE_PER_LEVEL = 4,
        INDIRECT_PURSUIT_DISTANCE = 15000,
        INDIRECT_TURN_ANGLE = 200,
        INDIRECT_DASH_ANGLE = 100
    };
    s16 pad;
    s32 aim_error;

    pad = 0;
    if (Me->status == STAT_ATTACK)
    {
        return SuccessionAttack(INDIRECT_RANGE, INDIRECT_CONTINUATION_AIM);
    }
    if (Me->status == STAT_JUMP)
    {
        return pad;
    }

    if (Distance < INDIRECT_RANGE && SR != SR_GONE)
    {
        SR = SR_NONE;
    }

    if (Distance < INDIRECT_CLOSE_RANGE)
    {
        aim_error = Degree;
        if (aim_error < 0)
        {
            aim_error = -aim_error;
        }
        if (aim_error < INDIRECT_CLOSE_AIM)
        {
            pad = GotoPosition(0, 0) & (PADLleft | PADLright);
            if (Distance < INDIRECT_RETREAT_DISTANCE ||
                Distance > INDIRECT_ADVANCE_DISTANCE)
            {
                pad |= PADLdown;
            }

            aim_error = Degree;
            if (aim_error < 0)
            {
                aim_error = -aim_error;
            }
            if (aim_error < INDIRECT_FIRE_AIM &&
                Me->motion->mid == MOT_ENGAGE_STANCE)
            {
                pad = PADRleft;
            }
        }
        else
        {
            pad = PADLup;
        }
    }
    else
    {
        if (rand() % (EngageLevel * INDIRECT_ATTACK_CHANCE_PER_LEVEL) == 0)
        {
            aim_error = Degree;
            if (aim_error < 0)
            {
                aim_error = -aim_error;
            }
            if (aim_error < INDIRECT_FIRE_AIM &&
                Me->motion->mid == MOT_ENGAGE_STANCE)
            {
                pad = PADRleft;
            }
        }

        if (Distance > INDIRECT_PURSUIT_DISTANCE)
        {
            pad = GotoPosition(0, 0);
        }
        else if (Degree > INDIRECT_TURN_ANGLE)
        {
            pad = PADLright;
        }
        else if (Degree > INDIRECT_DASH_ANGLE)
        {
            pad = SetCommand(&Me->pad, CMD_DASH_RIGHT);
        }
        else if (Degree < -INDIRECT_TURN_ANGLE)
        {
            pad = PADLleft;
        }
        else if (Degree < -INDIRECT_DASH_ANGLE)
        {
            pad = SetCommand(&Me->pad, CMD_DASH_LEFT);
        }
        else
        {
            ItemUse();
        }
    }

    if (pad == PADRleft)
    {
        Me->rotate->vy += Degree;
    }
    return pad;
}


static short AttackAnimal(void)
{
    s32 aim_error;
    s16 pad;
    u8 timer;

    if (Me->status == STAT_ATTACK || Me->status == STAT_JUMP)
    {
        Me->actmode = ANIMAL_ATTACK_TIMER_RESET;
        return 0;
    }
    if (Distance < ANIMAL_ATTACK_RANGE)
    {
        aim_error = Degree;
        if (aim_error < 0)
        {
            aim_error = -aim_error;
        }
        if (aim_error < ANIMAL_ATTACK_AIM)
        {
            return PADRleft;
        }
    }
    Me->actmode++;
    pad = GotoPosition(0, 0);
    timer = Me->actmode;
    if (timer < ANIMAL_ATTACK_NOTICE_FRAME)
    {
        pad = PADLup;
    }
    else if (timer == ANIMAL_ATTACK_NOTICE_FRAME)
    {
        Sound(Me, CHAR_VOICE_NOTICE);
    }
    else if (timer < ANIMAL_ATTACK_FULL_STEER_FRAME)
    {
        pad = pad & (PADLleft | PADLright);
    }
    return pad;
}


s16 Think4abandon(void)
{
    u16 base_attrib;
    s16 pad;

    base_attrib = Attrib & ~(ATTR_SEARCH | ATTR_PHASE);
    Me->chase[HUMANOID_CHASE_Z] = 0;
    Me->chase[HUMANOID_CHASE_X] = 0;
    if (EmergencyNotice != 0)
    {
        if (SR == SR_SEEN)
        {
            Attrib = base_attrib | PHASE_ALERT;
        }
        return (GotoPosition(0, 0) & PAD_TURN_BUTTONS_SIGNED);
    }
    else
    {
        if (Me->think[3] == Think4abandon)
        {
            pad = (GotoPosition(0, 0) & PAD_TURN_BUTTONS_SIGNED);
            if (pad != 0)
            {
                return pad;
            }
        }
        switch (SR)
        {
        case SR_GONE:
        case SR_UNSEEN:
            Attrib = base_attrib;
            SetNowMotion(Me, MOT_STATE_SHEATHE, MOTION_MOVE_APPLY);
            if (HumanGroup[TRIAL_ESCORT_PRINCESS]->type == HIME &&
                (Me->type & PAGE_MASK) != PAGE_CIVILIAN)
            {
                Me->target = &HumanGroup[TRIAL_ESCORT_PRINCESS]->model->locate;
            }
            Sound(Me, CHAR_VOICE_REACTION);
            break;
        case SR_GLIMPSE:
            Attrib = base_attrib | PHASE_SUSPICIOUS;
            SetNowMotion(Me, MOT_STATE_SHEATHE, MOTION_MOVE_APPLY);
            break;
        case SR_SEEN:
            Attrib = base_attrib | PHASE_ALERT;
            break;
        default:
            break;
        }
        return 0;
    }
}


s16 Think4contact(void)
{
    s16 pad;

    if (SR == SR_SEEN)
    {
        Attrib = (Attrib & (u16)~ATTR_PHASE) | PHASE_ALERT;
        return 0;
    }

    if (Me->chase[HUMANOID_CHASE_X] == 0 && Me->chase[HUMANOID_CHASE_Z] == 0)
    {
        if (Me->actcnt >= THINK4_ABANDON_TICKS)
        {
            return Think4abandon();
        }
        else
        {
            Me->actcnt++;
            pad = 0;
            if (Me->turn < Degree)
            {
                pad = PADLright;
            }
            else if (Degree < -Me->turn)
            {
                pad = -PADLleft;
            }
        }
    }
    else
    {
        s32 dx, dz;

        Me->actscnt++;
        dx = Me->chase[HUMANOID_CHASE_X] - Me->locate->vx;
        dz = Me->chase[HUMANOID_CHASE_Z] - Me->locate->vz;
        pad = GotoPosition(dx, dz);
        if (SquareRoot0(dx * dx + dz * dz) < THINK4_ARRIVAL_DISTANCE ||
            Me->actscnt == 0)
        {
            Me->chase[HUMANOID_CHASE_Z] = 0;
            Me->chase[HUMANOID_CHASE_X] = 0;
            Me->actcnt = 0;
        }
    }
    return pad;
}


s16 Think4chase(void)
{
    s16 pad;

    if (SR == SR_SEEN)
    {
        Attrib = (Attrib & (u16)~ATTR_PHASE) | PHASE_ALERT;
        return 0;
    }

    if (Me->chase[HUMANOID_CHASE_X] == 0 && Me->chase[HUMANOID_CHASE_Z] == 0)
    {
        if (Me->actcnt >= THINK4_ABANDON_TICKS)
        {
            return Think4abandon();
        }
        else
        {
            Me->actcnt++;
            pad = PADLup;
            if (Me->actcnt < THINK4_INITIAL_STEER_TICKS)
            {
                if (Degree > Me->turn)
                {
                    pad = PADLup | PADLright;
                }
                else if (Degree < -Me->turn)
                {
                    /* The signed view keeps this combination in addiu's
                     * immediate range. */
                    pad = (s16)(PADLleft | PADLup);
                }
            }
        }
    }
    else
    {
        s32 dx, dz;

        Me->actscnt++;
        dx = Me->chase[HUMANOID_CHASE_X] - Me->locate->vx;
        dz = Me->chase[HUMANOID_CHASE_Z] - Me->locate->vz;
        pad = GotoPosition(dx, dz);
        if (SquareRoot0(dx * dx + dz * dz) < THINK4_ARRIVAL_DISTANCE ||
            Me->actscnt == 0)
        {
            Me->chase[HUMANOID_CHASE_Z] = 0;
            Me->chase[HUMANOID_CHASE_X] = 0;
            Me->actcnt = 0;
        }
    }
    return pad;
}


void SetupThinkFunction(Humanoid *human, TThinkType type)
{
    human->think[0] = Think1Func[THINK1_FROM_MIX(type)];
    human->think[1] = Think2Func[THINK2_FROM_MIX(type)];
    human->think[2] = Think3Func[THINK3_FROM_MIX(type)];
    human->think[3] = Think4Func[THINK4_FROM_MIX(type)];
    if (type == THINK_MIX_NONE || type == THINK_MIX_PLAYER ||
        type == THINK_MIX_PAD2)
    {
        human->attribute &= ~ATTR_CUSTOMAI;
    }
    else
    {
        human->attribute |= ATTR_CUSTOMAI;
    }
}

void reset_alert_duration(void)
{
    s32 duration;

    RESET_TRIAL_ALERT_DURATION(duration);
}

/* The `none` think program: an empty pad word, so the character never
 * presses anything. */

s16 ThinkBasicNone(void)
{
    return 0;
}


s16 ThinkBasicHuman1(void)
{
    s32 pad;
    s32 raw_pad;

    raw_pad = remap_buttons_(GetPad(PAD_CONTROLLER_1));
    pad = raw_pad & (u16)~PADselect;
    if ((Me->map.attrib & MAP_DEATH) &&
        (Me->status == STAT_JUMP || Me->status == STAT_ATTACK))
    {
        /* Limit the complement to the 16-bit pad word. */
        pad = raw_pad & (u16)~(PADLup | PADLdown | PADLleft | PADLright | PADselect);
    }
    if (pad & PADR1)
    {
        /* Limit the complement to the 16-bit pad word. */
        pad = (pad & (u16)~PADR1) | PADRright;
    }
    return pad;
}


s16 ThinkBasicHuman2(void)
{
    s32 pad;
    s32 raw_pad;

    raw_pad = remap_buttons_(GetPad(PAD_CONTROLLER_2));
    pad = raw_pad & (u16)~PADselect;
    if ((Me->map.attrib & MAP_DEATH) &&
        (Me->status == STAT_JUMP || Me->status == STAT_ATTACK))
    {
        /* Limit the complement to the 16-bit pad word. */
        pad = raw_pad & (u16)~(PADLup | PADLdown | PADLleft | PADLright | PADselect);
    }
    if (pad & PADR1)
    {
        /* Limit the complement to the 16-bit pad word. */
        pad = (pad & (u16)~PADR1) | PADRright;
    }
    return pad;
}


s16 Think1watch(void)
{
    s16 pad;

    UPDATE_IDLE_LOOK_PAD(pad);
    return pad;
}

/* Think handlers return synthesized pad input; PADLup|PADL2 restarts the
 * scripted sleep action. */

s16 Think1sleep(void)
{
    MotionManager *motion = Me->motion;
    s16 pad = 0;

    if (motion->mid == MOT_ACTION)
    {
        SR = SR_UNSEEN;
    }
    else if (motion->count == 0)
    {
        pad = PADLup | PADL2;
    }
    if ((EmergencyNotice != 0) || ((Attrib & ATTR_PUSH) != 0))
    {
        pad = GotoPosition(0, 0) & (PADLleft | PADLright);
    }
    return pad;
}


s16 Think2confirm(void)
{
    return GotoPosition(0, 0) & PAD_TURN_BUTTONS_SIGNED;
}


s16 Think2contact(void)
{
    if ((Attrib & ATTR_WALL) && (Me->pad_hold == 0))
    {
        s32 hint;

        hint = PAD_HOLD((u32)PADLleft, 8);
        if (Degree > 0)
        {
            hint = PAD_HOLD(PADLright, 8);
        }
        Me->pad_hold = hint;
    }
    return GotoPosition(0, 0);
}

short Think3callaid(void)
{
    return Think3escape();
}

extern u8 think_name_pad1[];
extern u8 think_name_pad2[];
extern u8 think_name_trace[];
extern u8 think_name_watch[];
extern u8 think_name_random[];
extern u8 think_name_ninja[];
extern u8 think_name_sleep[];
extern u8 think_name_chase[];
extern u8 think_name_confirm[];
extern u8 think_name_contact[];
extern u8 think_name_call_aid[];
extern u8 think_name_attack_chase[];
extern u8 think_name_attack_point[];
extern u8 think_name_escape[];
extern u8 think_name_attack_area[];
extern u8 think_name_attack_hitaway[];
extern u8 think_name_abandon[];
extern u8 think_name_investigate_contact[];
extern u8 think_name_investigate_chase[];

static ThinkFunc Think1Func[N_THINK1_PROGRAMS] = {
    ThinkBasicNone,
    ThinkBasicHuman1,
    ThinkBasicHuman2,
    Think1trace,
    Think1watch,
    Think1random,
    Think1ninja,
    Think1sleep,
    Think1chase,
    Think1target
};

static ThinkFunc Think2Func[N_THINK2_PROGRAMS] = {
    ThinkBasicNone,
    ThinkBasicHuman1,
    ThinkBasicHuman2,
    Think2confirm,
    Think2contact,
    think_alarm_reaction_
};

static ThinkFunc Think3Func[N_THINK3_PROGRAMS] = {
    ThinkBasicNone,
    ThinkBasicHuman1,
    ThinkBasicHuman2,
    Think3callaid,
    Think3chase,
    Think3attack,
    Think3escape,
    Think3area,
    Think3hitaway,
    Think3firstattack
};

static ThinkFunc Think4Func[N_THINK4_PROGRAMS] = {
    ThinkBasicNone,
    ThinkBasicHuman1,
    ThinkBasicHuman2,
    Think4abandon,
    Think4contact,
    Think4chase
};

ThinkDBtype ThinkDB[20] = {
    {.name = think_name_pad1, .value = THINK_MIX_PLAYER},
    {.name = think_name_pad2, .value = THINK_MIX_PAD2},
    {.name = think_name_trace,
     .value = THINK_MIX(THINK1_TRACE, THINK_BASIC_NONE, THINK_BASIC_NONE,
                        THINK_BASIC_NONE)},
    {.name = think_name_watch,
     .value = THINK_MIX(THINK1_WATCH, THINK_BASIC_NONE, THINK_BASIC_NONE,
                        THINK_BASIC_NONE)},
    {.name = think_name_random,
     .value = THINK_MIX(THINK1_RANDOM, THINK_BASIC_NONE, THINK_BASIC_NONE,
                        THINK_BASIC_NONE)},
    {.name = think_name_ninja,
     .value = THINK_MIX(THINK1_NINJA, THINK_BASIC_NONE, THINK_BASIC_NONE,
                        THINK_BASIC_NONE)},
    {.name = think_name_sleep,
     .value = THINK_MIX(THINK1_SLEEP, THINK_BASIC_NONE, THINK_BASIC_NONE,
                        THINK_BASIC_NONE)},
    {.name = think_name_chase,
     .value = THINK_MIX(THINK1_CHASE, THINK_BASIC_NONE, THINK_BASIC_NONE,
                        THINK_BASIC_NONE)},
    {.name = think_name_confirm,
     .value = THINK_MIX(THINK_BASIC_NONE, THINK2_CONFIRM, THINK_BASIC_NONE,
                        THINK_BASIC_NONE)},
    {.name = think_name_contact,
     .value = THINK_MIX(THINK_BASIC_NONE, THINK2_CONTACT, THINK_BASIC_NONE,
                        THINK_BASIC_NONE)},
    {.name = think_name_call_aid,
     .value = THINK_MIX(THINK_BASIC_NONE, THINK_BASIC_NONE, THINK3_CALLAID,
                        THINK_BASIC_NONE)},
    {.name = think_name_attack_chase,
     .value = THINK_MIX(THINK_BASIC_NONE, THINK_BASIC_NONE,
                        THINK3_ATK_CHASE, THINK_BASIC_NONE)},
    {.name = think_name_attack_point,
     .value = THINK_MIX(THINK_BASIC_NONE, THINK_BASIC_NONE,
                        THINK3_ATK_POINT, THINK_BASIC_NONE)},
    {.name = think_name_escape,
     .value = THINK_MIX(THINK_BASIC_NONE, THINK_BASIC_NONE, THINK3_ESCAPE,
                        THINK_BASIC_NONE)},
    {.name = think_name_attack_area,
     .value = THINK_MIX(THINK_BASIC_NONE, THINK_BASIC_NONE,
                        THINK3_ATK_AREA, THINK_BASIC_NONE)},
    {.name = think_name_attack_hitaway,
     .value = THINK_MIX(THINK_BASIC_NONE, THINK_BASIC_NONE,
                        THINK3_ATK_HITAWAY, THINK_BASIC_NONE)},
    {.name = think_name_abandon,
     .value = THINK_MIX(THINK_BASIC_NONE, THINK_BASIC_NONE,
                        THINK_BASIC_NONE, THINK4_ABANDON)},
    {.name = think_name_investigate_contact,
     .value = THINK_MIX(THINK_BASIC_NONE, THINK_BASIC_NONE,
                        THINK_BASIC_NONE, THINK4_CONTACT)},
    {.name = think_name_investigate_chase,
     .value = THINK_MIX(THINK_BASIC_NONE, THINK_BASIC_NONE,
                        THINK_BASIC_NONE, THINK4_CHASE)},
    {.name = NULL, .value = THINK_MIX_NONE}
};

static ThinkFunc AttackFunc[N_WEAPON_ATTACK_CLASSES] = {
    AttackShort,
    AttackGeneral,
    AttackLong,
    AttackIndirect
};
