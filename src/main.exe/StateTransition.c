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
 *    loaded Humanoid pointer into the join.  The case-0 boolean switch leaves
 *    a real label barrier so jump.c does not also merge its preceding type
 *    check with case 1.
 *  - The first FieldAttrib store is the comma side effect in the fifth
 *    GetAreaMapLevel argument.  This keeps the value live until all four
 *    register arguments have been loaded, matching the original store
 *    schedule and allocation without a fixed-register declaration.
 *  - The packed word at Humanoid+0xb0 is read with `word >> 16`, not a halfword
 *    pointer cast: the former gives the target `lh` plus delayed copy to pad.
 *  - The one-shot `do` around the case-0 chase resets emits no control-flow
 *    instructions.  Its loop-depth notes weight the two pointer uses enough
 *    for local-alloc to choose the target's $v0/$v1 order naturally.
 *  - Repeating `me->target` for the two coordinate reads makes cse preserve
 *    the loaded target pointer with the target's explicit copy.  The >=-form
 *    ternaries likewise expand the two absolute values directly as abssi2.
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
extern s16 turn_towards_player_(s32 x, s32 z);
/* Retail's own prototype drift (def: u16 pressed) -- byte-required. */
extern s16 update_pressed_buttons(PADtype *pad, s16 pressed);
extern s16 Think1ninja(void);
extern s32 rand(void);

void StateTransition(Humanoid *human)
{
    s16 pad;
    s16 atr0;
    s32 ssr;
    s16 motid;
    s32 distance;
    SVECTOR vect;

    ssr = StrainRatio;
    Me_THINK_C = human;
    Pad = &human->pad;
    Attrib = human->attribute;
    atr0 = Attrib & ~(ATTR_SEARCH | ATTR_PHASE);

    if (human == StagePlayer)
    {
        PlayerSSR = ssr;
        StrainRatio = 0x7fffffff;
    }

    if ((u16)(human->status - STAT_DAMAGE) < 2)
    {
        if (human != StagePlayer && human->life > 0 && StrainRatio > 0)
        {
            StrainRatio = -0x8000;
        }
        update_pressed_buttons(Pad, 0);
        return;
    }

    if (ActionHalt != 0)
    {
        s32 dx;
        s32 dz;
        s16 direction;

        pad = 0;
        if ((u16)(human->type - 0x10) < 0x70)
        {
            dx = human->target->locate.coord.t[0] - human->locate->vx;
            dz = human->target->locate.coord.t[2] - human->locate->vz;
            direction = GetDirection(dx, dz, human->rotate->vy);
            if (direction > Me_THINK_C->turn)
            {
                pad = PADLright;
            }
            else if (-Me_THINK_C->turn > direction)
            {
                pad = PADLleft;
            }
            if (SquareRoot0(dx * dx + dz * dz) < 2000)
            {
                pad |= PADLdown;
            }
        }
        update_pressed_buttons(Pad, pad);
        return;
    }

    if ((Attrib & 4) == 0)
    {
        if (human == StagePlayer && EmergencyNotice != 0)
        {
            EmergencyNotice--;
            if (EmergencyNotice < 0)
            {
                EmergencyNotice = 0;
            }
        }
        pad = Me_THINK_C->think[0]();
        update_pressed_buttons(Pad, pad);
        return;
    }

    SR = SearchTarget(human, &Distance, &Degree);
    if (Me_THINK_C->target == (ModelType *)StagePlayer->model)
    {
        distance = Distance;
    }
    else
    {
        s32 dx;
        s32 dy;
        s32 dz;

        dx = StagePlayer->locate->vx - Me_THINK_C->locate->vx;
        dy = StagePlayer->locate->vy - Me_THINK_C->locate->vy;
        dz = StagePlayer->locate->vz - Me_THINK_C->locate->vz;
        distance = SquareRoot0(dx * dx + dy * dy + dz * dz);
    }

    {
        if (StagePlayer->itmctl == ITEM_HENSHIN ||
            StagePlayer->itmctl == ITEM_MANEBUE)
        {
            if ((Me_THINK_C->type & PAGE_MASK) != PAGE_BOSS &&
                (Me_THINK_C->type & PAGE_MASK) != PAGE_BEAST &&
                (StagePlayer->itmctl != ITEM_MANEBUE ||
                 (Attrib & 3) != PHASE_ALERT))
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
            else if ((Attrib & 3) == PHASE_CALM)
            {
                SR = SR_GLIMPSE;
            }
            else if (SR == SR_GLIMPSE)
            {
                SR = SR_SEEN;
            }
        }
    }

    GetMoveSpeed(&vect, Me_THINK_C->rotate->vy,
                 (s16)(Me_THINK_C->width * 2), 0);
    ProbeLevelLow = GetAreaMapLevel(GlobalAreaMap,
                                    Me_THINK_C->locate->vx + vect.vx,
                                    Me_THINK_C->locate->vy - EYE_HEIGHT,
                                    Me_THINK_C->locate->vz + vect.vz,
                                    AREA_LEVEL_RETURN_DELTA |
                                        AREA_LEVEL_FIRST_HIT |
                                        AREA_LEVEL_REUSE_CACHED);
    {
        u16 field_attrib;

        field_attrib = FieldAttrib;
        ProbeLevelHigh = GetAreaMapLevel(GlobalAreaMap,
                                         Me_THINK_C->locate->vx - vect.vx,
                                         Me_THINK_C->locate->vy - EYE_HEIGHT,
                                         Me_THINK_C->locate->vz - vect.vz,
                                         (ProbeAttrib[0] = field_attrib,
                                          AREA_LEVEL_RETURN_DELTA |
                                              AREA_LEVEL_FIRST_HIT |
                                              AREA_LEVEL_REUSE_CACHED));
    }
    ProbeAttrib[1] = FieldAttrib;

    switch (Attrib & 3)
    {
    case 0:
        if (distance < StrainRatio)
        {
            StrainRatio = distance;
        }
        pad = Me_THINK_C->think[0]();
        if (Me_THINK_C->type >= 7)
        {
            if (SR == SR_SEEN)
            {
                Humanoid *me;
                s32 life;

                SetNowMotion(Me_THINK_C, MOT_STATE_DRAW, 1);
                if (SetNowMotion(Me_THINK_C, MOT_ACTION_NOTICE, 1) == 0)
                {
                    Sound(Me_THINK_C, CHAR_VOICE_ALERT);
                }
                me = Me_THINK_C;
                life = me->life;
                Attrib = atr0 | PHASE_ALERT;
                do
                {
                    me->chase[HUMANOID_CHASE_Z] = 0;
                    me->chase[HUMANOID_CHASE_X] = 0;
                } while (0);
                if (life > 0)
                {
                    Humanoid *alert_me;

                    reset_alert_duration();
                    alert_me = Me_THINK_C;
                    switch (alert_me->type < PAGE_BOSS)
                    {
                    case 0:
                        break;
                    default:
                        if (alert_me->target == (ModelType *)StagePlayer->model)
                        {
                            Findenemies++;
                        }
                        break;
                    }
                }
            }
            else if (SR == SR_GLIMPSE)
            {
                if (EmergencyNotice != 0)
                {
                    SetNowMotion(Me_THINK_C, MOT_STATE_DRAW, 1);
                    Me_THINK_C->chase[HUMANOID_CHASE_Z] = 0;
                    Me_THINK_C->chase[HUMANOID_CHASE_X] = 0;
                }
                Attrib = atr0 | PHASE_SUSPICIOUS;
                Sound(Me_THINK_C, CHAR_VOICE_NOTICE);
            }
        }
        break;

    case 1:
        if (EmergencyNotice != 0 || (Attrib & ATTR_SEARCH) != 0)
        {
            if (StrainRatio > 0)
            {
                StrainRatio = -0x8000;
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
            if (StrainRatio > 0 || StrainRatio < -distance)
            {
                StrainRatio = -distance;
            }
            pad = Me_THINK_C->think[1]();
        }

        if (SR == SR_SEEN || ((Attrib & ATTR_HIT) != 0 && SR > 0))
        {
            Attrib = atr0 | PHASE_ALERT;
            if ((Attrib & ATTR_ALERT) == 0)
            {
                SetNowMotion(Me_THINK_C, MOT_STATE_DRAW, 1);
            }
            Me_THINK_C->chase[HUMANOID_CHASE_Z] = 0;
            Me_THINK_C->chase[HUMANOID_CHASE_X] = 0;
            Sound(Me_THINK_C, CHAR_VOICE_ALERT);
            if (Me_THINK_C->life > 0)
            {
                Humanoid *me;

                reset_alert_duration();
                me = Me_THINK_C;
                if (me->type < PAGE_BOSS &&
                    me->target == (ModelType *)StagePlayer->model)
                {
                    Findenemies++;
                }
            }
        }
        else if (EmergencyNotice < 2 &&
                 (u16)(SR + 2) < 2)
        {
            if ((Me_THINK_C->type & PAGE_MASK) != PAGE_BOSS)
            {
                SetNowMotion(Me_THINK_C, MOT_STATE_SHEATHE, 1);
            }
            Attrib = atr0;
        }
        break;

    case 2:
    {
        if (Attrib & ATTR_ALERT)
        {
            if (Me_THINK_C->target == (ModelType *)StagePlayer->model)
            {
                StrainRatio = 0;
            }
            else if (StrainRatio > 0)
            {
                StrainRatio = -0x8000;
            }
        }
        else
        {
            StrainRatio = -1;
        }

        if (Attrib & ATTR_SEARCH)
        {
            pad = Me_THINK_C->think[2]();
        }
        else
        {
            if ((Attrib & ATTR_ALERT) == 0)
            {
                SetNowMotion(Me_THINK_C, MOT_STATE_DRAW, 1);
            }
            pad = Think3firstattack();
        }

        if (pad & PADRleft)
        {
            Humanoid *me;
            s32 dy;

            if (StagePlayer->motion->mid == MOT_DAMAGE_DOWNED)
            {
                goto mask_attack;
            }

            me = Me_THINK_C;
            dy = me->target->locate.coord.t[1] - me->locate->vy;
            dy = dy >= 0 ? dy : -dy;
            if (dy < 2000)
            {
                goto random_attack;
            }
            if (WPATK_CLASS(me->wpatk) == WPATK_CLASS_RANGED)
            {
                goto random_attack;
            }

        mask_attack:
            pad &= (PADLleft | PADLdown | PADLright | PADLup);
            goto attack_checked;

        random_attack:
            if (rand() % 4 - 2 >= (s32)gNannido)
            {
                pad = 0;
            }
        }

    attack_checked:
        if (SR == SR_GONE)
        {
            Humanoid *me;
            s32 target_z;

            me = Me_THINK_C;
            Attrib = atr0 | ATTR_SEARCH | PHASE_INVESTIGATE;
            me->chase[HUMANOID_CHASE_X] = me->target->locate.coord.t[0];
            target_z = me->target->locate.coord.t[2];
            me->actscnt = 1;
            me->chase[HUMANOID_CHASE_Z] = target_z;
        }

        if (Me_THINK_C->pad_hold == 0)
        {
            if ((pad & PADLdown) &&
                ((ProbeAttrib[1] & (MAP_DEATH | MAP_WATER)) || ProbeLevelHigh > 5000))
            {
                Me_THINK_C->pad_hold = PAD_HOLD(PADLup, 30);
            }
            if ((pad & PADLup) &&
                ((ProbeAttrib[0] & (MAP_DEATH | MAP_WATER)) || ProbeLevelLow > 5000))
            {
                pad = turn_towards_player_(0, 0) & (PADLleft | PADLright);
            }
            if (StagePlayer->motion->mid == MOT_SYURI_RECOVER &&
                (rand() % (EngageLevel + 1) == 0 ||
                 (Me_THINK_C->type & PAGE_MASK) == PAGE_BOSS))
            {
                motid = (rand() & 1) ? CMD_DASH_LEFT : CMD_DASH_RIGHT;
                pad = SetCommand(&Me_THINK_C->pad, motid);
            }
            break;
        }
        break;
    }

    case 3:
        if (StrainRatio > 0)
        {
            StrainRatio = -0x8000;
        }
        pad = Me_THINK_C->think[3]();
        if ((Attrib & ATTR_WALL) && Me_THINK_C->pad_hold == 0)
        {
            Me_THINK_C->pad_hold = Degree > 0 ? PAD_HOLD(PADLright, 8)
                                               : PAD_HOLD(PADLleft, 8);
        }
        if ((Attrib & 3) == PHASE_ALERT)
        {
            Humanoid *me;

            Sound(Me_THINK_C, CHAR_VOICE_ALERT);
            reset_alert_duration();
            me = Me_THINK_C;
            if (me->type < PAGE_BOSS &&
                (Attrib & ATTR_SEARCH) == 0 &&
                me->target == (ModelType *)StagePlayer->model)
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
            s32 count;

            count = (u8)Me_THINK_C->pad_hold - 1;
            if (count != 0)
            {
                Me_THINK_C->pad_hold = PAD_HOLD(pad, count);
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
        Humanoid *me;

        me = Me_THINK_C;
        if (me->status == STAT_HANG)
        {
            s32 degree;
            s32 abs_degree;

            degree = Degree;
            abs_degree = degree;
            abs_degree = abs_degree >= 0 ? abs_degree : -abs_degree;
            pad = PADLup;
            if (abs_degree >= 500)
            {
                pad = PADLdown;
                if ((Attrib & 3) == PHASE_CALM)
                {
                    pad = PADLup;
                }
                else
                {
                    s32 hint;

                    hint = PAD_HOLD(PADLleft, 15);
                    if (degree > 0)
                    {
                        hint = PAD_HOLD(PADLright, 15);
                    }
                    me->pad_hold = hint;
                }
            }
        }
        else if ((ProbeAttrib[0] & (MAP_DEATH | MAP_WATER)) && (pad & PADLup))
        {
            pad &= (PADLleft | PADLdown | PADLright | PADstart | PADj | PADi | PADselect | PADRleft | PADRdown | PADRright | PADRup | PADR1 | PADL1 | PADR2 | PADL2);
        }
        else if ((ProbeAttrib[1] & (MAP_DEATH | MAP_WATER)) && (pad & PADLdown))
        {
            pad &= (PADLleft | PADLright | PADLup | PADstart | PADj | PADi | PADselect | PADRleft | PADRdown | PADRright | PADRup | PADR1 | PADL1 | PADR2 | PADL2);
        }
        else
        {
            if (Me_THINK_C->motion->count == 0)
            {
                s32 abs_degree;

                abs_degree = Degree;
                if (abs_degree < 0)
                {
                    abs_degree = -abs_degree;
                }
                if (abs_degree < 500 &&
                    (Me_THINK_C->think[0] == Think1ninja ||
                     ((Me_THINK_C->type & PAGE_MASK) == PAGE_NINJA && gNannido != DIFFICULTY_EASY)))
                {
                    s32 level;
                    s32 next_level;

                    GetMoveSpeed(&vect, Me_THINK_C->rotate->vy,
                                 (s16)(Me_THINK_C->width * 5), 0);
                    level = GetAreaMapLevel(GlobalAreaMap,
                                            Me_THINK_C->locate->vx,
                                            Me_THINK_C->locate->vy - EYE_HEIGHT,
                                            Me_THINK_C->locate->vz,
                                            AREA_LEVEL_STEP_DOWN |
                                                AREA_LEVEL_FIRST_HIT |
                                                AREA_LEVEL_REUSE_CACHED);
                    next_level = GetAreaMapLevel(GlobalAreaMap,
                                                 Me_THINK_C->locate->vx + vect.vx,
                                                 Me_THINK_C->locate->vy - EYE_HEIGHT,
                                                 Me_THINK_C->locate->vz + vect.vz,
                                                 AREA_LEVEL_RETURN_DELTA |
                                                     AREA_LEVEL_FIRST_HIT |
                                                     AREA_LEVEL_REUSE_CACHED);
                    if (level == Me_THINK_C->map.level)
                    {
                        if ((next_level >= 0 ? next_level : -next_level) < 500)
                        {
                            pad = PADLup | PADRdown;
                            goto tail;
                        }
                    }
                    if (next_level > 6100)
                    {
                        pad = PADLup | PADRdown;
                    }
                    goto tail;
                }
            }
            if (GameClock % 90 == 0 &&
                (((u16)Me_THINK_C->map.attrib & MAP_DAMAGE) ||
                 ((pad & PADLup) && ProbeLevelLow <= 2200 &&
                  ProbeLevelLow != LEVEL_NONE) ||
                 ((pad & PADLdown) && ProbeLevelHigh <= 2200 &&
                  ProbeLevelHigh != LEVEL_NONE)))
            {
                pad |= PADRdown;
            }
        }
    }

tail:
    if (Me_THINK_C->life < 0)
    {
        StrainRatio = ssr;
    }
    Me_THINK_C->attribute = Attrib;

    update_pressed_buttons(Pad, pad);
}
