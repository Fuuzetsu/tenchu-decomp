#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "conflict.h"
#include "humanoid.h"
#include "game_globals.h"
#include "item.h"
#include "padcmd.h"
#include "sound.h"

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

/*
 * StateTransition (0x8002aad0) is the central humanoid AI-state dispatcher:
 * it updates perception, alert/engage state, movement hints, obstacle probes,
 * and the synthesized controller input for one frame.
 *
 * Matching notes (3,776 bytes / 942 compared instructions):
 *  - This is THINK.C code, so its TU-local small globals need the explicit
 *    StateTransition gp-extern list in Build.hs/permute.py.  The random engage
 *    test also needs maspsx's `--expand-div` compatibility sequence.
 *  - The three alert arms deliberately contain the same Findenemies tail.
 *    gcc's cross-jump pass merges those copies while carrying the already
 *    loaded Humanoid pointer into the join. The first arm expresses the same
 *    actor-type and player-target eligibility test directly as the later
 *    alert arms.
 *  - The first FieldAttrib store is the comma side effect in the fifth
 *    GetAreaMapLevel argument.  This keeps the value live until all four
 *    register arguments have been loaded, matching the original store
 *    schedule and allocation without a fixed-register declaration.
 *  - The packed word at Humanoid+0xb0 is read with `word >> 16`, not a halfword
 *    pointer cast: the former gives the target `lh` plus delayed copy to pad.
 *  - The one-shot `do` around the case-0 chase resets emits no control-flow
 *    instructions.  Its loop-depth notes weight the two pointer uses enough
 *    for local-alloc to choose the target's $v0/$v1 order naturally.
 *  - Repeating `attacker->target` for the two coordinate reads makes
 *    cse preserve the loaded target pointer with the target's explicit copy.
 *    The >=-form ternaries likewise expand the two absolute values directly
 *    as abssi2.
 *  - reset_alert_duration has an old-style declaration intentionally.  The
 *    case-0 call carries the already-loaded life value in $a0; the callee takes
 *    no arguments, but preserving that harmless call-site value keeps jump.c
 *    from cross-jumping the call itself with the other alert arms.
 *  - The case-2 hold path can `break` normally; cse threads its known-nonzero
 *    value into the shared hint body. At the obstacle tail, an inverse guard
 *    removes the periodic label while the one-line obstacle action is safely
 *    duplicated to delete its acyclic label.
 *  - The case-1 and case-3 Findenemies paths use one short-circuit eligibility
 *    guard each: type/search/target are all prerequisites for the same count.
 *  - The alert attack filter is ordinary nested control flow. A downed player
 *    masks attack buttons immediately; otherwise a sufficiently separated
 *    target and non-ranged weapon take the same mask path, with unsuitable
 *    attacks falling into the difficulty-based random veto. The positive
 *    suitability test gives retail's mask-before-random block order directly.
 *  - The special forward-step probe combines its matching-level and small
 *    absolute-delta tests in one guard. An excessive positive delta is the
 *    `else if` case, so both outcomes fall naturally into the shared tail.
 */

extern Humanoid *Me_THINK_C;
extern s32 StrainRatio;
extern long EmergencyNotice;
extern s32 ProbeLevelLow;
extern s32 ProbeLevelHigh;
extern u16 ProbeAttrib[2];
extern s32 PlayerSSR;

extern void reset_alert_duration(void);
extern s16 Think2confirm(void);
extern s16 think_alarm_reaction_(void);
extern s16 Think3firstattack(void);
extern s16 GotoPosition(s32 vx, s32 vz);
/* Retail's own prototype drift (def: u16 pressed) -- byte-required. */
extern s16 update_pressed_buttons(PADtype *pad, s16 pressed);
extern s16 Think1ninja(void);
extern s32 rand(void);

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
    Me_THINK_C = human;
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
            if (target_direction > Me_THINK_C->turn)
            {
                pad = PADLright;
            }
            else if (-Me_THINK_C->turn > target_direction)
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
        pad = Me_THINK_C->think[PHASE_CALM]();
        update_pressed_buttons(Pad, pad);
        return;
    }

    SR = SearchTarget(human, &Distance, &Degree);
    if (Me_THINK_C->target == &StagePlayer->model->locate)
    {
        player_distance = Distance;
    }
    else
    {
        s32 player_dx;
        s32 player_dy;
        s32 player_dz;

        player_dx = StagePlayer->locate->vx - Me_THINK_C->locate->vx;
        player_dy = StagePlayer->locate->vy - Me_THINK_C->locate->vy;
        player_dz = StagePlayer->locate->vz - Me_THINK_C->locate->vz;
        player_distance = SquareRoot0(player_dx * player_dx +
                                      player_dy * player_dy +
                                      player_dz * player_dz);
    }

    {
        if (StagePlayer->active_item == ACTIVE_ITEM_DISGUISE ||
            StagePlayer->active_item == ACTIVE_ITEM_LURE)
        {
            if ((Me_THINK_C->type & PAGE_MASK) != PAGE_BOSS &&
                (Me_THINK_C->type & PAGE_MASK) != PAGE_BEAST &&
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

    GetMoveSpeed(&probe_offset, Me_THINK_C->rotate->vy,
                 (s16)(Me_THINK_C->width * 2), 0);
    ProbeLevelLow = GetAreaMapLevel(GlobalAreaMap,
                                    Me_THINK_C->locate->vx + probe_offset.vx,
                                    Me_THINK_C->locate->vy - EYE_HEIGHT,
                                    Me_THINK_C->locate->vz + probe_offset.vz,
                                    AREA_LEVEL_RETURN_DELTA |
                                        AREA_LEVEL_FIRST_HIT |
                                        AREA_LEVEL_REUSE_CACHED);
    {
        u16 forward_attrib;

        forward_attrib = FieldAttrib;
        ProbeLevelHigh = GetAreaMapLevel(GlobalAreaMap,
                                         Me_THINK_C->locate->vx - probe_offset.vx,
                                         Me_THINK_C->locate->vy - EYE_HEIGHT,
                                         Me_THINK_C->locate->vz - probe_offset.vz,
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
        pad = Me_THINK_C->think[PHASE_CALM]();
        if (Me_THINK_C->type >= KERAI_KATANA)
        {
            if (SR == SR_SEEN)
            {
                Humanoid *actor;
                s32 actor_life;

                SetNowMotion(Me_THINK_C, MOT_STATE_DRAW, MOTION_MOVE_APPLY);
                if (SetNowMotion(Me_THINK_C, MOT_ACTION_NOTICE, MOTION_MOVE_APPLY) == 0)
                {
                    Sound(Me_THINK_C, CHAR_VOICE_ALERT);
                }
                actor = Me_THINK_C;
                actor_life = actor->life;
                Attrib = base_attrib | PHASE_ALERT;
                do
                {
                    actor->chase[HUMANOID_CHASE_Z] = 0;
                    actor->chase[HUMANOID_CHASE_X] = 0;
                } while (0);
                if (actor_life > 0)
                {
                    Humanoid *alert_actor;

                    reset_alert_duration();
                    alert_actor = Me_THINK_C;
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
                    SetNowMotion(Me_THINK_C, MOT_STATE_DRAW, MOTION_MOVE_APPLY);
                    Me_THINK_C->chase[HUMANOID_CHASE_Z] = 0;
                    Me_THINK_C->chase[HUMANOID_CHASE_X] = 0;
                }
                Attrib = base_attrib | PHASE_SUSPICIOUS;
                Sound(Me_THINK_C, CHAR_VOICE_NOTICE);
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
            pad = Me_THINK_C->think[PHASE_SUSPICIOUS]();
        }

        if (SR == SR_SEEN || ((Attrib & ATTR_HIT) != 0 && SR > 0))
        {
            Attrib = base_attrib | PHASE_ALERT;
            if ((Attrib & ATTR_WEAPON_DRAWN) == 0)
            {
                SetNowMotion(Me_THINK_C, MOT_STATE_DRAW, MOTION_MOVE_APPLY);
            }
            Me_THINK_C->chase[HUMANOID_CHASE_Z] = 0;
            Me_THINK_C->chase[HUMANOID_CHASE_X] = 0;
            Sound(Me_THINK_C, CHAR_VOICE_ALERT);
            if (Me_THINK_C->life > 0)
            {
                Humanoid *alert_actor;

                reset_alert_duration();
                alert_actor = Me_THINK_C;
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
            if ((Me_THINK_C->type & PAGE_MASK) != PAGE_BOSS)
            {
                SetNowMotion(Me_THINK_C, MOT_STATE_SHEATHE, MOTION_MOVE_APPLY);
            }
            Attrib = base_attrib;
        }
        break;

    case PHASE_ALERT:
    {
        if (Attrib & ATTR_WEAPON_DRAWN)
        {
            if (Me_THINK_C->target == &StagePlayer->model->locate)
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
            pad = Me_THINK_C->think[PHASE_ALERT]();
        }
        else
        {
            if ((Attrib & ATTR_WEAPON_DRAWN) == 0)
            {
                SetNowMotion(Me_THINK_C, MOT_STATE_DRAW, MOTION_MOVE_APPLY);
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
                attacker = Me_THINK_C;
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

            searcher = Me_THINK_C;
            Attrib = base_attrib | ATTR_SEARCH | PHASE_INVESTIGATE;
            searcher->chase[HUMANOID_CHASE_X] =
                searcher->target->coord.t[0];
            last_seen_z = searcher->target->coord.t[2];
            searcher->actscnt = 1;
            searcher->chase[HUMANOID_CHASE_Z] = last_seen_z;
        }

        if (Me_THINK_C->pad_hold == 0)
        {
            if ((pad & PADLdown) &&
                ((ProbeAttrib[BACKWARD_PROBE] & (MAP_DEATH | MAP_WATER)) ||
                 ProbeLevelHigh > DANGER_PROBE_DELTA))
            {
                Me_THINK_C->pad_hold = PAD_HOLD(PADLup, 30);
            }
            if ((pad & PADLup) &&
                ((ProbeAttrib[FORWARD_PROBE] & (MAP_DEATH | MAP_WATER)) ||
                 ProbeLevelLow > DANGER_PROBE_DELTA))
            {
                pad = GotoPosition(0, 0) & (PADLleft | PADLright);
            }
            if (StagePlayer->motion->mid == MOT_SYURI_RECOVER &&
                (rand() % (EngageLevel + 1) == 0 ||
                 (Me_THINK_C->type & PAGE_MASK) == PAGE_BOSS))
            {
                dash_command = (rand() & 1) ? CMD_DASH_LEFT : CMD_DASH_RIGHT;
                pad = SetCommand(&Me_THINK_C->pad, dash_command);
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
        pad = Me_THINK_C->think[PHASE_INVESTIGATE]();
        if ((Attrib & ATTR_WALL) && Me_THINK_C->pad_hold == 0)
        {
            Me_THINK_C->pad_hold = Degree > 0 ? PAD_HOLD(PADLright, 8)
                                               : PAD_HOLD(PADLleft, 8);
        }
        if ((Attrib & ATTR_PHASE) == PHASE_ALERT)
        {
            Humanoid *alert_actor;

            Sound(Me_THINK_C, CHAR_VOICE_ALERT);
            reset_alert_duration();
            alert_actor = Me_THINK_C;
            if (alert_actor->type < PAGE_BOSS &&
                (Attrib & ATTR_SEARCH) == 0 &&
                alert_actor->target == &StagePlayer->model->locate)
            {
                Findenemies++;
            }
        }
        break;
    }
    if (Me_THINK_C->pad_hold != 0)
    {
        pad = Me_THINK_C->pad_hold >> 16;
        {
            s32 hold_frames;

            hold_frames = (u8)Me_THINK_C->pad_hold - 1;
            if (hold_frames != 0)
            {
                Me_THINK_C->pad_hold = PAD_HOLD(pad, hold_frames);
            }
            else if (pad & (PADLleft | PADLright))
            {
                Me_THINK_C->pad_hold =
                    PAD_HOLD(PADLup, (rand() % 3 + 1) * 30);
            }
            else
            {
                Me_THINK_C->pad_hold = 0;
            }
        }
    }

    {
        Humanoid *actor;

        actor = Me_THINK_C;
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
            if (Me_THINK_C->motion->count == 0)
            {
                s32 abs_target_angle;

                abs_target_angle = Degree;
                if (abs_target_angle < 0)
                {
                    abs_target_angle = -abs_target_angle;
                }
                if (abs_target_angle < TARGET_ANGLE_LIMIT &&
                    (Me_THINK_C->think[PHASE_CALM] == Think1ninja ||
                     ((Me_THINK_C->type & PAGE_MASK) == PAGE_NINJA && gNannido != DIFFICULTY_EASY)))
                {
                    s32 current_level;
                    s32 forward_delta;

                    GetMoveSpeed(&probe_offset, Me_THINK_C->rotate->vy,
                                 (s16)(Me_THINK_C->width * 5), 0);
                    current_level = GetAreaMapLevel(
                        GlobalAreaMap,
                        Me_THINK_C->locate->vx,
                        Me_THINK_C->locate->vy - EYE_HEIGHT,
                        Me_THINK_C->locate->vz,
                        AREA_LEVEL_STEP_DOWN |
                            AREA_LEVEL_FIRST_HIT |
                            AREA_LEVEL_REUSE_CACHED);
                    forward_delta = GetAreaMapLevel(
                        GlobalAreaMap,
                        Me_THINK_C->locate->vx + probe_offset.vx,
                        Me_THINK_C->locate->vy - EYE_HEIGHT,
                        Me_THINK_C->locate->vz + probe_offset.vz,
                        AREA_LEVEL_RETURN_DELTA |
                            AREA_LEVEL_FIRST_HIT |
                            AREA_LEVEL_REUSE_CACHED);
                    if (current_level == Me_THINK_C->map.level &&
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
                (((u16)Me_THINK_C->map.attrib & MAP_DAMAGE) ||
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
    if (Me_THINK_C->life < 0)
    {
        StrainRatio = saved_strain_ratio;
    }
    Me_THINK_C->attribute = Attrib;

    update_pressed_buttons(Pad, pad);
}
