#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "padcmd.h"
#include "sound.h"
#include "appear.h"

/* The demo symbols retain source-line provenance for THINK.C and its
 * THINK_1.C through THINK_4.C fragments. Retail emits them as one contiguous
 * object, with several functions deferred until the end; the manifest records
 * the demo source order separately from that retail emission order. */

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
static s16 atkd[N_WEAPON_ATTACK_CLASSES] = {3000, 3500, 4000, 20000};
static s16 atkd2[N_WEAPON_ATTACK_CLASSES] = {2000, 3000, 4000, 20000};

static ThinkFunc Think1Func[N_THINK1_PROGRAMS];
static ThinkFunc Think2Func[N_THINK2_PROGRAMS];
static ThinkFunc Think3Func[N_THINK3_PROGRAMS];
static ThinkFunc Think4Func[N_THINK4_PROGRAMS];
static ThinkFunc AttackFunc[N_WEAPON_ATTACK_CLASSES];
static character_kind
    AIDHumanType[N_STAGE_CONFIGS][N_STAGE_REINFORCEMENT_CHOICES];

static s16 ItemUse(void);
static s16 SuccessionAttack(s32 dist, s16 deg);
static s16 AttackAnimal(void);

enum animal_attack_timing
{
    ANIMAL_ATTACK_TIMER_RESET = 0,
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

#define RETURN_ATTACK_CONTINUATION(input_, range_, aim_)                    \
    if (Me->status == STAT_ATTACK)                                          \
    {                                                                        \
        s16 attack_result_;                                                  \
        s32 attack_degree_;                                                  \
                                                                             \
        do                                                                   \
        {                                                                    \
            if (Me->motion->count != BattleDB[Me->warid].contfrm)           \
            {                                                                \
                attack_result_ = 0;                                          \
                goto attack_continuation_return_;                            \
            }                                                                \
            if (Distance < (range_))                                         \
            {                                                                \
                attack_degree_ = Degree;                                     \
                if (attack_degree_ < 0)                                      \
                {                                                            \
                    attack_degree_ = -attack_degree_;                        \
                }                                                            \
                if (attack_degree_ < (aim_))                                 \
                {                                                            \
                    goto choose_attack_continuation_;                        \
                }                                                            \
            }                                                                \
            if (rand() % (EngageLevel + 1) != 0)                            \
            {                                                                \
                attack_result_ = input_;                                     \
                goto attack_continuation_return_;                            \
            }                                                                \
        } while (0);                                                         \
                                                                             \
    choose_attack_continuation_:                                             \
        if (Degree > 300)                                                    \
        {                                                                    \
            input_ = PADLright;                                              \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            input_ |= PADRleft;                                              \
            if (Degree < -300)                                               \
            {                                                                \
                input_ = PADLleft;                                           \
            }                                                                \
            else                                                             \
            {                                                                \
                goto attack_continuation_value_;                             \
            }                                                                \
        }                                                                    \
        input_ |= PADRleft;                                                  \
                                                                             \
    attack_continuation_value_:                                              \
        attack_result_ = input_;                                             \
    attack_continuation_return_:                                             \
        return attack_result_;                                               \
    }


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void StateTransition(struct Humanoid *human);
 *     THINK.C:99, 109 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     reg   $s2       short pad
 *     reg   $s1       short atr0
 *     reg   $s3       long ssr
 *     reg   $s0       short motid
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *StagePlayer;
 *     extern long StrainRatio;
 *     extern struct PADtype *Pad;
 *     extern short Attrib;
 *     extern short ActionHalt;
 *     extern long EmergencyNotice;
 *     extern long Distance;
 *     extern short Degree;
 *     extern short SR;
 *     extern unsigned long *GlobalAreaMap;
 *     extern short FieldAttrib;
 *     extern unsigned char gNannido;
 *     extern short EngageLevel;
 *     extern short Findenemies;
 *     extern long GameClock;
 * END PSX.SYM */

extern s32 ProbeLevelLow;
extern s32 ProbeLevelHigh;
extern u16 ProbeAttrib[2];
/* Retail declares the pressed word as s16 here; the definition uses u16. */
extern s16 update_pressed_buttons(PADtype *pad, s16 pressed);

void StateTransition(Humanoid *human)
{
    enum
    {
        STRAIN_NO_THREAT = 0x7fffffff,
        STRAIN_ALERTED = -0x8000,
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

    if (human == StagePlayer)
    {
        PlayerSSR = saved_strain_ratio;
        StrainRatio = STRAIN_NO_THREAT;
    }

    if ((u16)(human->status - STAT_DAMAGE) <= STAT_DEAD - STAT_DAMAGE)
    {
        if (human != StagePlayer && human->life > 0 && StrainRatio > 0)
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
        if ((u16)(human->type - PAGE_GUARD) < PAGE_BOSS - PAGE_GUARD)
        {
            target_dx = human->target->coord.t[0] -
                        human->locate->vx;
            target_dz = human->target->coord.t[2] -
                        human->locate->vz;
            target_direction = GetDirection(target_dx, target_dz,
                                            human->rotate->vy);
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
        if (human == StagePlayer && EmergencyNotice != 0)
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

    SR = SearchTarget(human, &Distance, &Degree);
    if (Me->target == &StagePlayer->model->locate)
    {
        player_distance = Distance;
    }
    else
    {
        s32 player_dx;
        s32 player_dy;
        s32 player_dz;

        player_dx = StagePlayer->locate->vx - Me->locate->vx;
        player_dy = StagePlayer->locate->vy - Me->locate->vy;
        player_dz = StagePlayer->locate->vz - Me->locate->vz;
        player_distance = SquareRoot0(player_dx * player_dx +
                                      player_dy * player_dy +
                                      player_dz * player_dz);
    }

    {
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
        if (Me->type >= KERAI_KATANA)
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
                    Humanoid *alert_actor;

                    reset_alert_duration();
                    alert_actor = Me;
                    if (alert_actor->type < PAGE_BOSS &&
                        alert_actor->target == &StagePlayer->model->locate)
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
            if (Me->life > 0)
            {
                Humanoid *alert_actor;

                reset_alert_duration();
                alert_actor = Me;
                if (alert_actor->type < PAGE_BOSS &&
                    alert_actor->target == &StagePlayer->model->locate)
                {
                    Findenemies++;
                }
            }
        }
        else if (EmergencyNotice < 2 &&
                 (u16)(SR - SR_GONE) <= SR_UNSEEN - SR_GONE)
        {
            if ((Me->type & PAGE_MASK) != PAGE_BOSS)
            {
                SetNowMotion(Me, MOT_STATE_SHEATHE, MOTION_MOVE_APPLY);
            }
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
                (rand() % (EngageLevel + 1) == 0 ||
                 (Me->type & PAGE_MASK) == PAGE_BOSS))
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
            Humanoid *alert_actor;

            Sound(Me, CHAR_VOICE_ALERT);
            reset_alert_duration();
            alert_actor = Me;
            if (alert_actor->type < PAGE_BOSS &&
                (Attrib & ATTR_SEARCH) == 0 &&
                alert_actor->target == &StagePlayer->model->locate)
            {
                Findenemies++;
            }
        }
        break;
    }
    if (Me->pad_hold != 0)
    {
        pad = Me->pad_hold >> 16;
        {
            s32 hold_frames;

            hold_frames = (u8)Me->pad_hold - 1;
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
                    goto tail;
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

tail:
    if (Me->life < 0)
    {
        StrainRatio = saved_strain_ratio;
    }
    Me->attribute = Attrib;

    update_pressed_buttons(Pad, pad);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GotoPosition(long vx, long vz);
 *     THINK.C:254, 11 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long vx
 *     param $a1       long vz
 * END PSX.SYM */

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
                    d2 = PADLright << 16;
                    d2 |= SIDESTEP_HOLD_TICKS;
                    Me->pad_hold = d2;
                }
                else if ((result & PADLleft) && (d2 != (u32)LEVEL_NONE))
                {
                    d2 = (u32)PADLleft << 16;
                    d2 |= SIDESTEP_HOLD_TICKS;
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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ChasetoTarget(long length);
 *     THINK.C:269, 21 src lines, frame 48 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s5       long length
 *     reg   $s3       long xx
 *     reg   $s2       long zz
 *     reg   $s4       long * chase
 *     reg   $s3       long vx
 *     reg   $s2       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Attrib;
 *     extern long Distance;
 * END PSX.SYM */

s16 ChasetoTarget(s32 length)
{
    Humanoid *me;
    long xx, zz;
    long *chase;
    long vx, vz;
    short deg;

    me = Me;
    chase = &me->chase[HUMANOID_CHASE_X];
    if (me->target == 0)
    {
        return 0;
    }

    xx = me->target->coord.t[0] +
         me->chase[HUMANOID_CHASE_X] - me->locate->vx;
    zz = me->target->coord.t[2] +
         chase[HUMANOID_CHASE_Z] - me->locate->vz;

    if (((xx >= 0 ? xx : -xx) < 500 &&
         (zz >= 0 ? zz : -zz) < 500) ||
        (Attrib & ATTR_WALL) != 0 || Distance < 1000)
    {
        return 0;
    }

    if ((Attrib & (ATTR_HIT | ATTR_PUSH)) != 0 ||
        (me->chase[HUMANOID_CHASE_X] | chase[HUMANOID_CHASE_Z]) == 0)
    {
        deg = rand();
        vx = rcos(deg) * length >> FIXED_SHIFT;
        me->chase[HUMANOID_CHASE_X] = vx;
        vz = rsin(deg) * length >> FIXED_SHIFT;
        chase[HUMANOID_CHASE_Z] = vz;
    }
    return GotoPosition(xx, zz);
}

void register_character_death(Humanoid *dead)
{
    VECTOR delta;
    SVECTOR passage;
    Humanoid *human;
    s16 scale;
    s16 next;
    s16 index;
    s32 alert_time;
    s32 chase_z;

    scale = 1;
    if ((dead->attribute & ATTR_SEARCH) == 0 && gNannido != DIFFICULTY_EASY)
    {
        next = DeathIndex + 1;
        DeathIndex = next;
        index = next % Humans;
        human = HumanGroup[index];
        DeathIndex = index;

        if ((human->attribute & (ATTR_SUSPEND | ATTR_PHASE)) == 0 &&
            human->status != STAT_DEAD && human->status != STAT_DAMAGE &&
            human != StagePlayer)
        {
            delta.vx = dead->locate->vx - human->locate->vx;
            delta.vz = dead->locate->vz - human->locate->vz;
            if (__builtin_abs(GetDirection(delta.vx, delta.vz,
                                           human->rotate->vy)) <= 900 &&
                SquareRoot0(delta.vx * delta.vx + delta.vz * delta.vz) <= 20000)
            {
                delta.vy = dead->locate->vy - human->locate->vy - human->height;

                while (__builtin_abs(delta.vx) > 500 ||
                       __builtin_abs(delta.vy) > 500 ||
                       __builtin_abs(delta.vz) > 500)
                {
                    scale <<= 1;
                    delta.vx >>= 1;
                    delta.vy >>= 1;
                    delta.vz >>= 1;
                }

                passage.vx = delta.vx;
                passage.vy = delta.vy;
                passage.vz = delta.vz;
                if (GetAreaMapPassage(GlobalAreaMap, human->locate,
                                      &passage, scale) == 0)
                {
                    RESET_ALERT_DURATION(alert_time);
                    Sound(human, CHAR_VOICE_NOTICE);
                    SetNowMotion(human, MOT_STATE_DRAW, MOTION_MOVE_APPLY);
                    dead->attribute |= ATTR_SEARCH;
                    human->attribute |= ATTR_SEARCH | PHASE_SUSPICIOUS;
                    human->chase[HUMANOID_CHASE_X] = dead->locate->vx;
                    chase_z = dead->locate->vz;
                    human->actcnt = 0;
                    human->actscnt = 0;
                    human->chase[HUMANOID_CHASE_Z] = chase_z;
                }
            }
        }
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1trace(void);
 *     THINK_1.C:16, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a0       long xx
 *     reg   $a1       long zz
 *     reg   $t0       short pad
 *     reg   $a0       long vx
 *     reg   $a1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Degree;
 *     extern short Attrib;
 * END PSX.SYM */


s16 Think1trace(void)
{
    s16 pad;

    pad = 0;
    if (Me->actcnt == 0)
    {
        u8 old_actscnt;

        old_actscnt = Me->actscnt;
        Me->actscnt = old_actscnt + 1;
        if (old_actscnt < 60)
        {
            Humanoid *self;
            s32 turn;
            s32 degree;
            s32 abs_degree;

            if (Me->actcnt || degree)
            {
                self = Me;
                degree = Degree;
                turn = self->turn;
                abs_degree = degree;
            }
            else
            {
                self = Me;
                degree = Degree;
                turn = self->turn;
                abs_degree = degree;
            }
            if (degree < 0)
            {
                abs_degree = -abs_degree;
            }
            if (turn < abs_degree)
            {
                if (self->actscnt < 30)
                {
                    pad = -PADLleft;
                    if (turn < degree)
                    {
                        pad = PADLright;
                    }
                }
            }
        }
        else
        {
            Me->actcnt = 1;
            Me->actscnt = 0;
        }
    }
    else
    {
        Me->actcnt++;
        if (Attrib & ATTR_TRACE)
        {
            pad = ControlTraceLine(Me);
        }
    }
    return pad;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1random(void);
 *     THINK_1.C:74, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a1       long xx
 *     reg   $v1       long zz
 *     reg   $s1       short pad
 *     reg   $a1       long vx
 *     reg   $v1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Attrib;
 * END PSX.SYM */

s16 Think1random(void)
{
    s16 pad;
    pad = 0;
    if (++Me->actcnt == 1)
    {
        Me->chase[HUMANOID_CHASE_X] = Me->point[HUMANOID_HOME_X] + rand() % 10000 - 5000;
        Me->chase[HUMANOID_CHASE_Z] = Me->point[HUMANOID_HOME_Z] + rand() % 10000 - 5000;
    }
    else
    {
        s32 vx, vz;
        VECTOR *locate;

        locate = Me->locate;
        vx = Me->chase[HUMANOID_CHASE_X] - locate->vx;
        vz = Me->chase[HUMANOID_CHASE_Z] - locate->vz;
        if ((((vx >= 0) ? vx : -vx) < 1000 &&
             ((vz >= 0) ? vz : -vz) < 1000) ||
            (Attrib & ATTR_WALL))
        {
            Me->actcnt = 0;
        }
        else
        {
            pad = GotoPosition(vx, vz);
        }
    }
    return pad;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1ninja(void);
 *     THINK_1.C:99, 2 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1chase(void);
 *     THINK_1.C:119, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a0       struct Humanoid * enemy
 *     reg   $s1       long xx
 *     reg   $s2       long zz
 *     reg   $s3       short pad
 *     reg   $s1       long vx
 *     reg   $s2       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 * END PSX.SYM */

s16 Think1chase(void)
{
    s16 pad;
    pad = 0;
    if (++Me->actcnt == 1)
    {
        Humanoid *enemy;

        enemy = GetNearestHumanoid(Me, 5000);
        if (enemy != 0)
        {
            Me->chase[HUMANOID_CHASE_X] = enemy->locate->vx;
            Me->chase[HUMANOID_CHASE_Z] = enemy->locate->vz;
        }
        else
        {
            Me->chase[HUMANOID_CHASE_X] =
                Me->point[HUMANOID_HOME_X] + rand() % 10000 - 5000;
            Me->chase[HUMANOID_CHASE_Z] =
                Me->point[HUMANOID_HOME_Z] + rand() % 10000 - 5000;
        }
    }
    else
    {
        pad = GotoPosition(
            Me->chase[HUMANOID_CHASE_X] - Me->locate->vx,
            Me->chase[HUMANOID_CHASE_Z] - Me->locate->vz);
        if ((s16)pad == 0)
        {
            pad |= PADRleft;
            Me->actcnt = 0;
        }
    }
    return pad;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1target(void);
 *     THINK_1.C:154, 110 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a0       long xx
 *     reg   $a1       long zz
 *     reg   $a0       long vx
 *     reg   $a1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern short SR;
 *     extern struct Humanoid *StagePlayer;
 *     extern short Attrib;
 *     extern unsigned char gNannido;
 *     extern long EmergencyNotice;
 * END PSX.SYM */

s16 Think1target(void)
{
    s32 xx;
    s32 zz;
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
    if ((GameClock & 0x1f) == 0)
    {
        s32 dy;
        s32 abs_dy;
        s32 direction;

        vx = xx = StagePlayer->locate->vx - Me->locate->vx;
        vz = zz = StagePlayer->locate->vz - Me->locate->vz;
        dy = StagePlayer->locate->vy - Me->locate->vy;
        distance = SquareRoot0(vx * xx + vz * zz);
        deg = GetDirection(xx, zz, Me->rotate->vy);
        if (distance <= 4000)
        {
            /* Retail keeps identical branches here; their original distinction is unknown. */
            if (distance != 0)
            {
                abs_dy = (dy >= 0) ? dy : -dy;
            }
            else
            {
                abs_dy = (dy >= 0) ? dy : -dy;
            }
            if (abs_dy <= 3000)
            {
                direction = (deg >= 0) ? deg : -deg;
                if (direction < 900 &&
                    StagePlayer->active_item != ACTIVE_ITEM_DISGUISE)
                {
                    s32 alert_time;

                    Me->target = &StagePlayer->model->locate;
                    Attrib = (Attrib & (u16)~ATTR_PHASE) | PHASE_ALERT;
                    SetNowMotion(Me, MOT_STATE_DRAW, MOTION_MOVE_APPLY);
                    Me->chase[HUMANOID_CHASE_Z] = 0;
                    Me->chase[HUMANOID_CHASE_X] = 0;
                    Sound(Me, CHAR_VOICE_ALERT);
                    RESET_ALERT_DURATION(alert_time);
                }
            }
        }
    }

    vx = Me->target->coord.t[0] - Me->locate->vx;
    vz = Me->target->coord.t[2] - Me->locate->vz;
    distance = SquareRoot0(vx * vx + vz * vz);
    if (distance < 200)
    {
        return 0;
    }
    if (distance < 4000)
    {
        s32 dy;

        dy = __builtin_abs(Me->target->coord.t[1] - Me->locate->vy);

        if (dy <= 2000)
        {
            return GotoPosition(vx, vz);
        }
        {
            UPDATE_IDLE_LOOK_PAD(pad);
            return pad;
        }
    }
    return GotoPosition(vx, vz);
}

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
        if (distance < 2000 || (Attrib & ATTR_WALL))
        {
            s32 alertTime;
            s32 nextState;

            RESET_ALERT_DURATION(alertTime);
            Sound(Me, CHAR_VOICE_REACTION);

            switch (gNannido)
            {
            case DIFFICULTY_EASY:
                Me->actcnt = 1;
                break;
            case DIFFICULTY_NORMAL:
                Me->actcnt = rand() & 1;
                break;
            case DIFFICULTY_HARD:
                Me->actcnt = 0;
                break;
            }

            self = Me;
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

        Me->actcnt = (Me->actcnt + 1) & 0x1F;
        count = Me->actcnt;
        if (count & 8)
        {
            if (count != 8)
            {
                result = Me->pad.data;
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
                                 Me->rotate->vy);
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
            if (absoluteDegree > 1000 && Me->pad_hold == 0)
            {
                s32 quotient;

                self = Me;
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
            Me->actscnt = ALARM_REACTION_APPROACH;
            Me->actcnt = 1;

            randomValue = rand();
            soundId = CHAR_VOICE_ACTION_B;
            if (randomValue & 1)
            {
                soundId = CHAR_VOICE_ACTION_A;
            }
            Sound(Me, soundId);

            type = AIDHumanType[StageID][
                rand() % N_STAGE_REINFORCEMENT_CHOICES];
            rotation = Me->rotate;
            newRotation = rotation->vy + direction;
            loc = Me->locate;
            rotation->vy = newRotation;
            position = loc;
            human = BreedLife(type, position->vx, position->vy, position->vz,
                              newRotation);

            human->target = Me->target;
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

            human->chase[HUMANOID_CHASE_X] = Me->chase[HUMANOID_CHASE_X] +
                              (rand() % 5 - 2) * 500;
            human->chase[HUMANOID_CHASE_Z] = Me->chase[HUMANOID_CHASE_Z] +
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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3callaid(void);
 *     THINK_3.C:31, frame 40 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       struct Humanoid * human
 *     reg   $v1       struct Humanoid * human
 *     reg   $a0       short type
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern int StageID;
 *     extern short Degree;
 *     extern short (*Think1Func[10])();
 *     extern short (*Think2Func[5])();
 *     extern short (*Think3Func[10])();
 *     extern struct PADtype *Pad;
 *     extern short (*Think4Func[6])();
 *     extern short Attrib;
 *     extern short StageEnemies;
 *     extern short StageCitizens;
 * END PSX.SYM */

/* Per-stage reinforcement pair (StageID*2 + coin flip) — the stage's
 * own guard faction (retail data): rouban/rounin, ninja A+B, rouban,
 * Manji cultists, pirates, tengu, oni, kabane, kerai, asigaru, sisi. */
short Think3callaid(void)
{
    Humanoid *human;
    Humanoid *newhuman;
    s32 r;

    if (Distance < 16500)
    {
        if (SR != SR_GONE)
        {
            SR = SR_NONE;
        }
        return Think3escape();
    }
    else
    {
        s16 ret;
        character_kind *aid = AIDHumanType[0];
        character_kind *type_ptr;
        character_kind type;
        ThinkFunc func;

        SR = SR_UNSEEN;
        r = rand();
        /* The flat byte-offset view retains the target's index-first address
         * expression; ordinary structured indexing recolors the table base. */
        type_ptr = (character_kind *)(
            (u8 *)aid +
            ((r % N_STAGE_REINFORCEMENT_CHOICES) * sizeof(character_kind) +
             StageID * N_STAGE_REINFORCEMENT_CHOICES *
                 sizeof(character_kind)));
        type = *type_ptr;
        newhuman = BreedLife(type,
                             Me->locate->vx,
                             Me->locate->vy,
                             Me->locate->vz,
                             (s32)Me->rotate->vy + (s32)Degree);
        human = Me;
        newhuman->target = human->target;
        KillHumanoid(human);
        newhuman->think[0] = Think1Func[THINK1_WATCH];
        newhuman->think[1] = Think2Func[THINK2_CONTACT];
        newhuman->think[2] = Think3Func[THINK3_ATK_CHASE];
        Pad = &newhuman->pad;
        func = Think4Func[THINK4_CONTACT];
        (Me = newhuman)->attribute |= ATTR_CUSTOMAI;
        newhuman->think[3] = func;
        EquipWeapon(newhuman, WEAPON_DRAWN);
        SetNowMotion(Me, MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
        Attrib = Me->attribute | PHASE_ALERT;
        ret = 0;
        if ((Me->type & PAGE_MASK) == PAGE_CIVILIAN)
        {
            StageEnemies++;
            StageCitizens--;
            ret = 0;
        }
        return ret;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3chase(void);
 *     THINK_3.C:60, 5 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern short EngageLevel;
 *     extern long GameClock;
 *     extern long AttackActionCount;
 *     extern short Degree;
 *     extern short (*AttackFunc[4])();
 * END PSX.SYM */

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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3attack(void);
 *     THINK_3.C:70, 29 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $v0       short rng
 *     reg   $s0       short pad
 *     reg   $a2       short idx
 *
 * Globals it touches, as the original declared them:
 *     extern short SR;
 *     extern long Distance;
 *     extern short Degree;
 *     extern short EngageLevel;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

/* Per-range-class engagement distances (retail data: 3000/3500/4000
 * for the melee classes, 20000 for the ranged class). */


s16 Think3attack(void)
{
    s16 rng;
    s16 pad;
    weapon_attack_class idx;

    pad = 0;
    idx = WEAPON_ATTACK_CLASS(Me->wpatk);

    if (Me->status == STAT_ATTACK)
    {
        if (idx != WEAPON_ATTACK_RANGED)
        {
            pad = SuccessionAttack(3000, 1500);
        }
        else
        {
            pad = SuccessionAttack(20000, 500);
        }
        return pad;
    }

    if (SR != SR_GONE &&
        ((idx == WEAPON_ATTACK_RANGED && Distance < 14000) || Distance < SR_CLEAR_RANGE))
    {
        SR = SR_NONE;
    }

    if ((s16)((N_WEAPON_ATTACK_CLASSES - idx) * Me->turn) < Degree)
    {
        pad = PADLright;
    }
    else if (Degree < -(s16)((N_WEAPON_ATTACK_CLASSES - idx) * Me->turn))
    {
        pad = PADLleft;
    }

    if (idx != WEAPON_ATTACK_RANGED)
    {
        rng = atkd[idx] / 2;
    }
    else
    {
        rng = 4000;
    }

    if (Distance < rng)
    {
        if (__builtin_abs(Degree) < 1000 &&
            Me->motion->count == 0)
        {
            if (Distance < 2000)
            {
                if (rand() % (EngageLevel + 1) != 0)
                {
                    pad |= PADRleft;
                }
                else
                {
                    pad = PADRleft | PADRright;
                }
            }
            else
            {
                pad |= PADRleft;
            }
        }
        else
        {
            pad |= PADLdown;
        }
    }
    else if (Distance < atkd[idx])
    {
        if (idx == WEAPON_ATTACK_RANGED)
        {
            if (pad == 0 && rand() % (EngageLevel * 4) == 0)
            {
                pad = PADRleft;
            }
        }
        else if (__builtin_abs(Degree) < 100 &&
                 atkd[idx] - 1000 < Distance)
        {
            pad = SetCommand(&Me->pad, CMD_LUNGE);
        }
        else if (Me->motion->count == 0 &&
                 __builtin_abs(Degree) < 1200)
        {
            pad |= PADRleft;
        }
        else if (rng + 500 < Distance)
        {
            pad |= PADLup;
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
            pad = SetCommand(&Me->pad, command);
        }
        else if (Me->motion->count == 0 && rand() % 3 == 0)
        {
            pad = SetCommand(&Me->pad, CMD_DASH_FORWARD);
        }
        else
        {
            ItemUse();
        }
    }

    if (Me->motion->count == 0 &&
        rand() % 30 == 0 &&
        Me->status == STAT_ENGAGE)
    {
        SetNowMotion(Me, MOT_ATTACK_TAUNT, MOTION_MOVE_APPLY); /* taunt */
    }

    return pad;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3escape(void);
 *     THINK_3.C:103, 21 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern short Degree;
 *     extern short Attrib;
 * END PSX.SYM */


s16 Think3escape(void)
{
    s32 result;
    s32 degree;

    result = 0;
    if (Distance < 16500 && SR != SR_GONE)
    {
        SR = SR_NONE;
    }
    if (Degree > 0)
    {
        result = (s16)PADLleft;
    }
    else if (Degree < 0)
    {
        if (result != 0)
        {
            result = PADLright;
        }
        else
        {
            result = PADLright;
        }
    }
    degree = Degree;
    if (degree < 0)
    {
        degree = -degree;
    }
    if (degree < 1000)
    {
        result |= PADLdown;
    }
    else
    {
        result |= PADLup;
    }
    if (Attrib & ATTR_WALL)
    {
        Humanoid *human;
        s32 degree2;
        s32 abs_degree2;

        degree2 = Degree;
        if (result != 0)
        {
            abs_degree2 = degree2;
        }
        else
        {
            abs_degree2 = degree2;
        }
        if (degree2 < 0)
        {
            abs_degree2 = -abs_degree2;
        }
        if (abs_degree2 > 1000 && Me->pad_hold == 0)
        {
            s32 quotient;

            if (result != 0)
            {
                human = Me;
            }
            else
            {
                human = Me;
            }
            quotient = 1000 / human->turn;
            human->pad_hold =
                PAD_HOLD(degree2 > 0 ? PADLright : PADLleft, quotient);
        }
    }
    return result;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3area(void);
 *     THINK_3.C:128, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       short pad
 *     reg   $s2       long xx
 *     reg   $s1       long zz
 *     reg   $s3       long dist
 *     reg   $s2       long vx
 *     reg   $s1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 * END PSX.SYM */

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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3hitaway(void);
 *     THINK_3.C:172, 33 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a1       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern short (*AttackFunc[4])();
 *     extern short Degree;
 *     extern short Attrib;
 * END PSX.SYM */

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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3firstattack(void);
 *     THINK_3.C:209, frame 16 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a0       short pad
 *     reg   $v1       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern short Attrib;
 *     extern short Degree;
 * END PSX.SYM */

/* Per-range-class first-attack distances, indexed by weapon attack class
 * (same shape as Think3attack.c's atkd table). */
s16 Think3firstattack(void)
{
    s16 pad;
    weapon_attack_class idx;
    s32 degree;

    pad = GotoPosition(0, 0);
    if (Distance < SR_CLEAR_RANGE && SR != SR_GONE)
    {
        SR = SR_NONE;
    }
    if ((Me->type & PAGE_MASK) == PAGE_CIVILIAN)
    {
        Attrib |= ATTR_SEARCH;
    }
    idx = WEAPON_ATTACK_CLASS(Me->wpatk);
    if (idx == WEAPON_ATTACK_RANGED)
    {
        s16 masked;

        masked = pad & PAD_TURN_BUTTONS_SIGNED;
        /* Empty loop retained for code layout; its original source construct is unknown. */
        do
        {
        } while (0);
        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }
        if (degree > 100)
        {
            return masked;
        }
        pad = masked;
    }
    if (Distance < atkd2[idx])
    {
        pad |= PADRleft;
        Attrib |= ATTR_SEARCH;
    }
    return pad;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short ItemUse(void);
 *     THINK_3.C:225, 18 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       short id
 *
 * Globals it touches, as the original declared them:
 *     extern short Degree;
 * END PSX.SYM */

static s16 ItemUse(void)
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
        return 5;
    }
    if (Me->status != STAT_ENGAGE)
    {
        return 5;
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
    return;
}

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

static short AttackShort(void)
{
    MotionManager *motion;
    s16 pad;
    s32 attack_result;

    pad = 0;
    if ((Me->type & PAGE_MASK) == PAGE_BEAST)
    {
        return AttackAnimal();
    }

    if (Me->status == STAT_ATTACK)
    {
        Humanoid *status_human;
        s16 pad;
        s32 status_raw;
        s32 status_degree;

        status_human = Me;
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
                Me->actmode = MELEE_ATTACK_ENGAGED;
                goto return_pad;
            }
        }

        pad = ChasetoTarget(2000);
        if (pad == 0)
        {
            Me->actmode = MELEE_ATTACK_ENGAGED;
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
            Me->actmode = MELEE_ATTACK_ENGAGED;
        }
    }

    else if ((motion->count & (MELEE_ATTACK_DECISION_PERIOD - 1)) != 0)
    {
        s32 raw_degree;
        s32 degree;

        pad = Me->pad.data;
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
                    pad = SetCommand(&Me->pad, CMD_DASH_FORWARD);
                }
                else
                {
                    pad = PADLup;
                }
            }
            else if (rand() % 30 == 0)
            {
                pad = SetCommand(&Me->pad, CMD_LUNGE);
            }
        }
    }
    else if (Distance > 4000)
    {
        Humanoid *me;

        Me->actmode = MELEE_ATTACK_CLOSING;
        me = Me;
        Me->chase[HUMANOID_CHASE_Z] = 0;
        me->chase[HUMANOID_CHASE_X] = 0;
        ItemUse();
        if (Distance > 5000)
        {
            pad = PADLup | PADRdown;
        }
    }
    else
    {
        if ((Attrib & ATTR_WALL) != 0)
        {
            Me->actmode = MELEE_ATTACK_CLOSING;
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
            }
            else if (Distance > 3000)
            {
                if (degree < 200 && Distance > 3500)
                {
                    if ((rand() & 1) != 0)
                    {
                        pad = SetCommand(&Me->pad, CMD_DASH_FORWARD);
                    }
                    else if ((rand() & 1) != 0)
                    {
                        pad = SetCommand(&Me->pad, CMD_LUNGE);
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
            }
            else if (Distance < 1500)
            {
                if (raw_degree > 300)
                {
                    pad = SetCommand(&Me->pad, CMD_DASH_LEFT);
                }
                else if (raw_degree < -300)
                {
                    pad = SetCommand(&Me->pad, CMD_DASH_RIGHT);
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
            }
            else if ((rand() & 1) != 0)
            {
                if (Degree > 100)
                {
                    pad = SetCommand(&Me->pad, CMD_DASH_RIGHT);
                }
                else if (Degree < -100)
                {
                    pad = SetCommand(&Me->pad, CMD_DASH_LEFT);
                }
                else
                {
                    pad = SetCommand(&Me->pad, CMD_DASH_BACKWARD);
                }
            }
        }
    }

return_pad:
    return pad;
}

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

static short AttackGeneral(void)
{
    s16 pad;

    pad = 0;
    RETURN_ATTACK_CONTINUATION(pad, 2000, 500);

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
        s32 deg;

        pad = ChasetoTarget(3000);
        if (pad == 0 || (Attrib & ATTR_HIT) != 0)
        {
            Me->actmode = MELEE_ATTACK_ENGAGED;
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

    if ((Me->motion->count &
         (MELEE_ATTACK_DECISION_PERIOD - 1)) != 0)
    {
        s32 d;
        s32 deg;

        pad = Me->pad.data;
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

        deg = Degree;
        if (deg < 0)
        {
            deg = -deg;
        }
        if (deg < 1200 && rand() % (EngageLevel + 1) == 0 &&
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
                    pad = SetCommand(&Me->pad, CMD_DASH_BACKWARD);
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
                    pad = SetCommand(&Me->pad, CMD_DASH_FORWARD);
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
                        pad = SetCommand(&Me->pad, CMD_LUNGE);
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
                pad = SetCommand(&Me->pad, CMD_DASH_RIGHT);
            }
            else if (Degree < -100)
            {
                pad = SetCommand(&Me->pad, CMD_DASH_LEFT);
            }
        }
    }

    return pad;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short AttackLong(void);
 *     THINK_3.C:450, 71 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a0       short ad
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

static short AttackLong(void)
{
    s16 ad;
    s16 pad;
    s32 degree;

    pad = 0;
    RETURN_ATTACK_CONTINUATION(pad, 3000, 1500);

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
        pad = ChasetoTarget(3000);
        if (pad == 0 || (Attrib & ATTR_HIT) != 0)
        {
            Me->actmode = MELEE_ATTACK_ENGAGED;
        }
        if (Me->motion->count == 0)
        {
            if (rand() % 5 == 0)
            {
                pad = PADLup | PADRdown;
            }
        }
        return pad;
    }

    if ((Me->motion->count &
         (MELEE_ATTACK_DECISION_PERIOD - 1)) != 0)
    {
        s32 deg;

        pad = Me->pad.data;
        if (Distance < 3000)
        {
            ad = Degree;
            deg = (ad >= 0) ? ad : -ad;
            if (deg < 500)
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

    if (Degree > 300)
    {
        pad = PADLright;
    }
    else if (Degree < -300)
    {
        pad = PADLleft;
    }

    if (Distance > 3000 && Distance < 4000)
    {
        if (rand() % (EngageLevel + 1) == 0)
        {
            AttackActionCount = GameClock;
            AttackActionCount += EngageLevel * ATTACK_COOLDOWN_PER_LEVEL;
            return pad | PADRleft;
        }
    }

    ad = Degree;
    degree = (ad >= 0) ? ad : -ad;

    if (degree > 1000 || Distance < 3000)
    {
        if (Distance < 1000)
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

    if (Distance <= 4000)
    {
        return pad;
    }

    if (degree < 50)
    {
        pad = SetCommand(&Me->pad, CMD_LUNGE);
    }
    else if (Me->motion->count != 0)
    {
        pad |= PADLup;
    }
    else if (ad > 50)
    {
        pad = SetCommand(&Me->pad, CMD_DASH_RIGHT);
    }
    else if (ad < -50)
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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short AttackIndirect(void);
 *     THINK_3.C:525, 54 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern struct BattleType BattleDB[78];
 *     extern long Distance;
 *     extern short Degree;
 *     extern short EngageLevel;
 *     extern short SR;
 * END PSX.SYM */

static short AttackIndirect(void)
{
    s16 pad;
    s32 degree;

    pad = 0;
    RETURN_ATTACK_CONTINUATION(pad, INDIRECT_RANGE, 500);
    if (Me->status == STAT_JUMP)
    {
        return pad;
    }

    if (Distance < INDIRECT_RANGE && SR != SR_GONE)
    {
        SR = SR_NONE;
    }

    if (Distance < 5000)
    {
        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }
        if (degree < 1000)
        {
            pad = GotoPosition(0, 0) & (PADLleft | PADLright);
            if ((u32)(Distance - 1000) > 3000 - 1000)
            {
                pad |= PADLdown;
            }

            degree = Degree;
            if (degree < 0)
            {
                degree = -degree;
            }
            if (degree < 200 && Me->motion->mid == MOT_ENGAGE_STANCE)
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
        if (rand() % (EngageLevel * 4) == 0)
        {
            degree = Degree;
            if (degree < 0)
            {
                degree = -degree;
            }
            if (degree < 200 && Me->motion->mid == MOT_ENGAGE_STANCE)
            {
                pad = PADRleft;
            }
        }

        if (Distance > 15000)
        {
            pad = GotoPosition(0, 0);
        }
        else if (Degree > 200)
        {
            pad = PADLright;
        }
        else if (Degree > 100)
        {
            pad = SetCommand(&Me->pad, CMD_DASH_RIGHT);
        }
        else if (Degree < -200)
        {
            pad = PADLleft;
        }
        else if (Degree < -100)
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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short AttackAnimal(void);
 *     THINK_3.C:583, 26 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short Degree;
 * END PSX.SYM */

static short AttackAnimal(void)
{
    s32 deg;
    s16 pad;
    u8 timer;

    if (Me->status == STAT_ATTACK || Me->status == STAT_JUMP)
    {
        Me->actmode = ANIMAL_ATTACK_TIMER_RESET;
        return 0;
    }
    if (Distance < 2000)
    {
        deg = Degree;
        if (deg < 0)
        {
            deg = -deg;
        }
        if (deg < 200)
        {
            return PADRleft; /* bite */
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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think4abandon(void);
 *     THINK_4.C:14, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $v1       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Attrib;
 *     extern short SR;
 *     extern long EmergencyNotice;
 * END PSX.SYM */

s16 Think4abandon(void)
{
    u16 cleared;
    s16 pad;

    cleared = Attrib & ~(ATTR_SEARCH | ATTR_PHASE);
    Me->chase[HUMANOID_CHASE_Z] = 0;
    Me->chase[HUMANOID_CHASE_X] = 0;
    if ((Me->type & PAGE_MASK) == PAGE_BOSS)
    {
        if ((u16)(SR - 1) < 2)
        {
            Attrib = cleared | PHASE_ALERT;
            if (Me->motion->count == 0)
            {
                s32 r;

                r = rand();
                if (r % 60 == 0)
                {
                    Sound(Me, CHAR_VOICE_ALERT);
                }
            }
        }
        return (GotoPosition(0, 0) & PAD_TURN_BUTTONS_SIGNED);
    }
    else if (EmergencyNotice != 0)
    {
        if (SR == SR_SEEN)
        {
            Attrib = cleared | PHASE_ALERT;
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
        if (SR != SR_SEEN)
        {
            if (SR >= 2)
            {
                if (SR != SR_GLIMPSE)
                {
                    return 0;
                }
            }
            else
            {
                if (SR >= SR_NONE)
                {
                    return 0;
                }
                if (SR < SR_GONE)
                {
                    return 0;
                }
                /* Target lost: stand down (0x80f sheathes and returns to
                 * idle — ActSTATE) with the give-up voice line. */
                Attrib = cleared;
                SetNowMotion(Me, MOT_STATE_SHEATHE, MOTION_MOVE_APPLY);
                Sound(Me, CHAR_VOICE_REACTION);
                return 0;
            }

            /* Only a glimpse: drop to suspicious and stand down. */
            Attrib = cleared | PHASE_SUSPICIOUS;
            SetNowMotion(Me, MOT_STATE_SHEATHE, MOTION_MOVE_APPLY);
            return 0;
        }
        Attrib = cleared | PHASE_ALERT;
        return 0;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think4contact(void);
 *     THINK_4.C:36, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a1       short pad
 *     reg   $s1       long xx
 *     reg   $s2       long zz
 *     reg   $s1       long vx
 *     reg   $s2       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short SR;
 *     extern short Attrib;
 *     extern short Degree;
 * END PSX.SYM */

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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think4chase(void);
 *     THINK_4.C:75, 189 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a0       short pad
 *     reg   $s1       long xx
 *     reg   $s2       long zz
 *     reg   $s1       long vx
 *     reg   $s2       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short SR;
 *     extern short Attrib;
 *     extern short Degree;
 * END PSX.SYM */

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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupThinkFunction(struct Humanoid *human, short type);
 *     THINK.C:212, 12 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short type
 *
 * Globals it touches, as the original declared them:
 *     extern short (*Think1Func[10])();
 *     extern short (*Think2Func[5])();
 *     extern short (*Think3Func[10])();
 *     extern short (*Think4Func[6])();
 * END PSX.SYM */

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

    duration = ALERT_DURATION;
    if (gNannido == DIFFICULTY_HARD)
    {
        duration = ALERT_DURATION_HARD;
    }
    EmergencyNotice = duration;
}

/* The `none` think program: an empty pad word, so the character never
 * presses anything. */
/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ThinkBasicNone(void);
 *     THINK.C:229, 2 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 * END PSX.SYM */

s16 ThinkBasicNone(void)
{
    return 0;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ThinkBasicHuman1(void);
 *     THINK.C:236, 7 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern enum TSystemFlag SystemFlag;
 * END PSX.SYM */

s16 ThinkBasicHuman1(void)
{
    s32 pad;

    pad = remap_buttons_(GetPad(PAD_CONTROLLER_1));
    if ((pad & PADselect) && (SystemFlag & SYSFLAG_DEBUGPRINT))
    {
        pad = 0;
    }
    if ((Me->map.attrib & MAP_DEATH) &&
        (Me->status == STAT_JUMP || Me->status == STAT_ATTACK))
    {
        /* Limit the complement to the 16-bit pad word. */
        pad &= (u16)~(PADLup | PADLdown | PADLleft | PADLright);
    }
    if (pad & PADR1)
    {
        /* Limit the complement to the 16-bit pad word. */
        pad = (pad & (u16)~PADR1) | PADRright;
    }
    return pad;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ThinkBasicHuman2(void);
 *     THINK.C:247, 2 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 * END PSX.SYM */

s16 ThinkBasicHuman2(void)
{
    return GetPad(PAD_CONTROLLER_2);
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1watch(void);
 *     THINK_1.C:50, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a0       long xx
 *     reg   $a1       long zz
 *     reg   $s0       short pad
 *     reg   $a0       long vx
 *     reg   $a1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 * END PSX.SYM */

s16 Think1watch(void)
{
    s16 pad;

    UPDATE_IDLE_LOOK_PAD(pad);
    return pad;
}

/* Think handlers return synthesized pad input; PADLup|PADL2 restarts the
 * scripted sleep action. */
/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1sleep(void);
 *     THINK_1.C:106, 8 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern short SR;
 *     extern long EmergencyNotice;
 *     extern short Attrib;
 * END PSX.SYM */

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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think2confirm(void);
 *     THINK_2.C:14, frame 16 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 * END PSX.SYM */

s16 Think2confirm(void)
{
    return GotoPosition(0, 0) & PAD_TURN_BUTTONS_SIGNED;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think2contact(void);
 *     THINK_2.C:21, frame 16 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 * END PSX.SYM */

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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short SuccessionAttack(long dist, short deg);
 *     THINK_3.C:247, 15 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long dist
 *     param $a1       short deg
 *
 * Globals it touches, as the original declared them:
 *     extern struct BattleType BattleDB[78];
 *     extern long Distance;
 *     extern short Degree;
 *     extern short EngageLevel;
 * END PSX.SYM */

static s16 SuccessionAttack(s32 dist, s16 deg)
{
    int t;
    int lev;
    s16 buttons;

    buttons = 0;
    if (Me->motion->count !=
        BattleDB[Me->warid].contfrm)
    {
        return 0;
    }
    if (Distance < dist)
    {
        int d;
        int raw;

        d = deg;
        raw = (int)Degree;
        raw = __builtin_abs(raw);
        t = raw < d;
        if (!t)
        {
            t = rand();
            lev = EngageLevel + 1;
            if (t % lev != 0)
            {
                goto ret;
            }
        }
    }
    else
    {
        t = rand();
        lev = EngageLevel + 1;
        if (t % lev != 0)
        {
            goto ret;
        }
    }
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
ret:
    return buttons;
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

static character_kind
AIDHumanType[N_STAGE_CONFIGS][N_STAGE_REINFORCEMENT_CHOICES] = {
    {ROUBAN_KATANA, ROUNIN_1YARI},
    {NINJAB, NINJAA},
    {ROUBAN_KATANA, ROUBAN_YUMI},
    {MANJI5_KEITOU, MANJI5_YUMI},
    {PIRATEA_TEPPO, PIRATEA_HALBERT},
    {TENGU_KON, TENGU_YUMI},
    {KIMEN13, ONIKERAI},
    {KABANE_HOUTOU, KABANE_YUMI},
    {KERAI_YUMI, KERAI_YARI},
    {ASIGARU_KATANA, ASIGARU_YUMI},
    {SISI_KATANA, SISI_YUMI}
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
