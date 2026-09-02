#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "appear.h"
#include "humanoid.h"
#include "item.h"
#include "sound.h"
#include "effect.h"

extern Humanoid *Me_MOTION_C;
extern s16 ARMOUR_EQUIPPED_;
extern Humanoid *DeadHumanoid;
/* MOTION.C's original severity-and-direction damage-animation table. */
extern s16 damagemotion[N_DAMAGE_MOTIONS];

extern int ReqLifeBar(Humanoid *h);
extern void reset_alert_duration(void);
extern s16 PlayMotion(MotionManager *mmp, s16 mode);
extern void SetBlood(VECTOR *pos, s16 n, s16 time);

static inline void RecordPlayerKill(Humanoid *victim)
{
    if ((victim->type & PAGE_MASK) == PAGE_CIVILIAN)
    {
        FriendHits++;
    }
    else if ((victim->attribute & (ATTR_WEAPON_DRAWN | PHASE_ALERT)) == 0)
    {
        Criticals++;
    }
    else
    {
        Murders++;
    }
}

static inline void RecoilAttacker(Humanoid *attacker, short damage,
                                  int rumble_power, int rumble_release)
{
    if (attacker->status == STAT_ATTACK)
    {
        attacker->motion->loop = damage / -3 - 1;
        attacker->vector.vz = 0;
        attacker->vector.vx = 0;
        if (StagePlayer == attacker)
        {
            PadShockAR(PAD_PORT_1, rumble_power, RUMBLE_ATTACK_NORMAL,
                       rumble_release);
        }
    }
}

static inline void RequestDamageFeedback(Humanoid *attacker,
                                         int rumble_release)
{
    Humanoid *who;

    if (StagePlayer == Me_MOTION_C)
    {
        PadShockAR(PAD_PORT_1, RUMBLE_POWER_HALF, RUMBLE_ATTACK_NORMAL,
                   rumble_release);
        who = attacker;
    }
    else
    {
        who = Me_MOTION_C;
    }
    ReqLifeBar(who);
}

static inline void PlayRandomHurtVoice(void)
{
    int random;
    short sound;

    random = rand();
    sound = CHAR_VOICE_HURT_ALT;
    if ((random & 1) != 0)
    {
        sound = CHAR_VOICE_HURT;
    }
    Sound(Me_MOTION_C, sound);
}

static inline void SnapToWaistConflict(void)
{
    int conflict_id;

    conflict_id = Me_MOTION_C->model->object[MODEL_PART_WAIST]->id;
    if (conflict_id >= 0)
    {
        dtL->vx = ConflictObject[conflict_id].position.vx;
        dtL->vz = ConflictObject[conflict_id].position.vz;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DamageControl(void);
 *     MOTION.C:408, 236 src lines, frame 72 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct VECTOR p
 *     reg   $s1       struct VECTOR * pp
 *     reg   $s2       short i
 *     reg   $s0       short id
 *     reg   $s4       short deg
 *     reg   $s1       short dmg
 *     reg   $s5       short did
 *     reg   $s3       struct Humanoid * enemy
 *     reg   $v1       short i
 *     stack sp+32     struct SVECTOR pv
 *     reg   $v1       short i
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short MotionUpdateMode;
 *     extern short motID;
 *     extern struct Humanoid *StagePlayer;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct BattleType BattleDB[78];
 *     extern struct VECTOR *dtL;
 *     extern short motMODE;
 *     extern short Criticals;
 *     extern short Murders;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern short ActionHalt;
 *     extern struct MotionManager *dtM;
 *     extern struct SVECTOR ConflictDistance;
 *     extern struct SVECTOR *dtR;
 *     extern short FriendHits;
 *     extern unsigned long *GlobalAreaMap;
 *     extern unsigned char gNannido;
 *     extern short EngageLevel;
 *     extern struct SVECTOR *dtV;
 * END PSX.SYM */

void DamageControl(void)
{
    MotionManager *mmp;
    short did;
    short deg;
    short t;
    short newvy;
    int abs_direction;
    Humanoid *enemy;
    int id;
    short dmg;
    SVECTOR dir;
    VECTOR p;
    SVECTOR pv;

    id = (u16)Me_MOTION_C->vector.pad;
    dmg = 0;
    if (Me_MOTION_C->life < 1)
    {
        return;
    }
    if (MotionUpdateMode != 0)
    {
        return;
    }
    if (motID == MOT_SWIM_EXIT)
    {
        return;
    }
    if (Me_MOTION_C == StagePlayer)
    {
        SetCameraMode(CMODE_NORMAL);
    }
    if ((Me_MOTION_C->type & PAGE_MASK) == PAGE_BEAST)
    {
        enemy = ConflictObject[(short)id].common;
        if (enemy != (Humanoid *)CONFLICT_OWNER_ITEM)
        {
            Sound(enemy, CHAR_SE_IMPACT);
            DeleteConflict(ConflictObject[(short)id].model);
            deg = GetAttackDBID(enemy, enemy->motion->mid);
            {
                s16 hp;

                hp = (u16)Me_MOTION_C->life - (u16)BattleDB[deg].power;
                Me_MOTION_C->life = hp;
                if (hp < 0 || (Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN) == 0)
                {
                    Me_MOTION_C->life = 0;
                }
            }
            p.vx = dtL->vx;
            p.vy = dtL->vy - Me_MOTION_C->height / 2;
            p.vz = dtL->vz;
            SetImpact(&p, 6 * FIXED_ONE, IMPACT_SPRITE_HIT);
            if (StagePlayer == enemy)
            {
                PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX,
                           RUMBLE_ATTACK_NORMAL, RUMBLE_RELEASE_SHORT);
            }
        }
        else
        {
            Me_MOTION_C->life = 0;
        }
        ReqLifeBar(Me_MOTION_C);
        if (Me_MOTION_C->life != 0)
        {
            SET_MOTION(MOT_DAMAGE, MOTION_MOVE_APPLY);
            Sound(Me_MOTION_C, CHAR_VOICE_HURT);
            reset_alert_duration();
        }
        else
        {
            SET_MOTION(MOT_DEAD, MOTION_MOVE_APPLY);
            if ((Me_MOTION_C->type != NINKEN) &&
                ((StagePlayer == enemy ||
                  enemy == (Humanoid *)CONFLICT_OWNER_ITEM)))
            {
                if ((Me_MOTION_C->attribute & (ATTR_WEAPON_DRAWN | PHASE_ALERT)) == 0)
                {
                    Criticals++;
                }
                else
                {
                    Murders++;
                }
            }
            if ((Me_MOTION_C->attribute & (ATTR_WEAPON_DRAWN | PHASE_ALERT)) != 0)
            {
                Sound(Me_MOTION_C, CHAR_VOICE_HURT_HEAVY);
                reset_alert_duration();
            }
        }
        SET_NOW_MOTION_UNLESS_CVA(goto attack_cancel);
    attack_cancel:
        AttackCancelControl(ATTACK_CANCEL_ALL);
        return;
    }
    if (motID < MOT_ATTACK_STEALTH_BACK)
    {
        goto resolve_hit;
    }
    if (motID <= MOT_ATTACK_STEALTH_SIDE_AYAME)
    {
        goto attack_break;
    }
    if (motID == MOT_DAMAGE_DOWNED)
    {
        return;
    }
    goto resolve_hit;
/* motID in the stealth-kill band (MOT_ATTACK_STEALTH_*): the hit
 * cancels the move — reset ActionHalt, pick recover/idle, nudge down */
attack_break:
    ActionHalt = ACTION_HALT_NONE;
    SELECT_RETURN_MOTION();
    dtL->vy--;
    return;
resolve_hit:
    dtM->mask = MOTION_MASK_ALL;
    AttackCancelControl(ATTACK_CANCEL_ALL);
    {
        Humanoid *conflict;

        t = id;
        conflict = ConflictObject[t].common;
        if (conflict == (Humanoid *)CONFLICT_OWNER_ITEM)
        {
            if (Me_MOTION_C->status == STAT_DAMAGE)
            {
                return;
            }
            GetConflictResult((ModelType *)Me_MOTION_C->model, t);
            t = GetItemType(t);
            switch (t)
            {
            case ITEM_MAKIBISHI:
                dmg = DMG_MAKIBISHI;
                SET_MOTION(MOT_DAMAGE_MAKIBISHI, MOTION_MOVE_APPLY);
                break;
            case ITEM_SHURIKEN:
                if (dmg == 0)
                {
                    dmg = DMG_SHURIKEN;
                }
                if ((Me_MOTION_C->type == NINJA_0) || (Me_MOTION_C->type == NINJA_1))
                {
                    Me_MOTION_C->item[ITEM_SHURIKEN]++;
                }
                /* fall through: the shared zero-damage test preserves the shuriken's DMG_SHURIKEN */
            case ITEM_HAPPOU:
                if (dmg == 0)
                {
                    dmg = DMG_HAPPOU;
                }
                /* fall through */
            case ITEM_GUN:
                if (dmg == 0)
                {
                    dmg = DMG_GUN;
                }
                /* fall through */
            case ITEM_ARROW:
                if (dmg == 0)
                {
                    dmg = DMG_ARROW;
                }
                p.vx = dtL->vx;
                motID = MOT_DAMAGE;
                p.vy = dtL->vy - Me_MOTION_C->height / 2;
                p.vz = dtL->vz;
                motMODE = MOTION_MOVE_APPLY;
                SetBlood(&p, 5, 90);
                break;
            case ITEM_NAPALM:
                dmg = DMG_NAPALM;
                if ((rand() & 1) == 0)
                {
                    motID = MOT_DAMAGE_BACK_LIGHT;
                }
                else
                {
                    motID = MOT_DAMAGE_FRONT_MID;
                }
                motMODE = MOTION_MOVE_APPLY;
                break;
            case ITEM_FIRE:
            case ITEM_JIRAI:
            case ITEM_LIGHTNINGBOLT:
                dmg = DMG_FIRE;
                if (t == ITEM_JIRAI)
                {
                    dmg = DMG_JIRAI;
                }
                if ((Me_MOTION_C->map.attrib & MAP_WATER) == 0)
                {
                    Me_MOTION_C->map.height = 1;
                }
                break;
            default:
                dmg = 0;
                break;
            }
            if (Me_MOTION_C->map.height > 0)
            {
                int abs_direction;

                did = GetDirection(ConflictDistance.vx, ConflictDistance.vz, dtR->vy);
                abs_direction = did;
                if (abs_direction < 0)
                {
                    abs_direction = -abs_direction;
                }
                if (abs_direction < ANGLE_QUADRANT)
                {
                    SET_MOTION(MOT_DAMAGE_LAUNCH_BACK, MOTION_MOVE_NONE);
                    dtR->vy += did;
                    MoveHumanoid(Me_MOTION_C, -DAMAGE_LAUNCH_SPEED, 0);
                }
                else
                {
                    SET_MOTION(MOT_DAMAGE_LAUNCH_FORE, MOTION_MOVE_NONE);
                    dtR->vy = (ANGLE_HALF + did) + dtR->vy;
                    MoveHumanoid(Me_MOTION_C, DAMAGE_LAUNCH_SPEED, 0);
                }
            }
            if ((Me_MOTION_C == StagePlayer) && (ARMOUR_EQUIPPED_ != 0))
            {
                dmg = (dmg * 7) / 10;
            }
            {
                s16 hp;

                hp = (u16)Me_MOTION_C->life - dmg;
                Me_MOTION_C->life = hp;
                if (hp <= 0)
                {
                    Me_MOTION_C->life = 0;
                    if (motID != MOT_DAMAGE_LAUNCH_BACK && motID != MOT_DAMAGE_LAUNCH_FORE)
                    {
                        SET_MOTION(MOT_DEAD, MOTION_MOVE_APPLY);
                    }
                    Sound(Me_MOTION_C, CHAR_VOICE_HURT_HEAVY);
                    {
                        TItemType item_type;

                        item_type = GetItemType((s16)id);
                        if ((item_type < ITEM_GUN) ||
                            (item_type > ITEM_ARROW && item_type != ITEM_LIGHTNINGBOLT))
                        {
                            RecordPlayerKill(Me_MOTION_C);
                        }
                    }
                }
                else
                {
                    PlayRandomHurtVoice();
                }
            }
            if (StagePlayer == Me_MOTION_C)
            {
                PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX,
                           RUMBLE_ATTACK_NORMAL, RUMBLE_RELEASE_MEDIUM);
            }
            else
            {
                ReqLifeBar(Me_MOTION_C);
            }
            reset_alert_duration();
        }
        else
        {
            enemy = conflict;
            if (((Me_MOTION_C->type & PAGE_MASK) == PAGE_BOSS) &&
                (enemy != StagePlayer))
            {
                return;
            }
            t = 1;
            dir.vx = enemy->locate->vx - dtL->vx;
            dir.vy = enemy->locate->vy - dtL->vy;
            dir.vz = enemy->locate->vz - dtL->vz;
            while (__builtin_abs(dir.vx) > 100 || __builtin_abs(dir.vy) > 100 ||
                   __builtin_abs(dir.vz) > 100)
            {
                t = t << 1;
                dir.vx >>= 1;
                dir.vy >>= 1;
                dir.vz >>= 1;
            }
            p = *dtL;
            p.vy -= 1000;
            if (GetAreaMapPassage(GlobalAreaMap, &p, &dir, t) != 0)
            {
                return;
            }
            {
                int abs_direction;

                did = GetDirection(enemy->locate->vx - dtL->vx,
                                   enemy->locate->vz - dtL->vz,
                                   dtR->vy);
                deg = GetAttackDBID(enemy, enemy->motion->mid);
                if (Me_MOTION_C != StagePlayer &&
                    Me_MOTION_C->status != STAT_ATTACK &&
                    (Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN) != 0 &&
                    Me_MOTION_C->map.height == 0 &&
                    gNannido != DIFFICULTY_EASY)
                {
                    if (rand() % (EngageLevel + 1) == 0)
                    {
                        if (((Me_MOTION_C->type == NINJA_0) ||
                             (Me_MOTION_C->type == NINJA_1)) &&
                            (rand() & 1) != 0)
                        {
                            motID = MOT_CHASE_BACK;
                        }
                    }
                    else
                    {
                        motID = MOT_CHASE_BACK;
                    }
                }
                abs_direction = did;
                if (abs_direction < 0)
                {
                    abs_direction = -abs_direction;
                }
                if (abs_direction < 700)
                {
                    if (motID == MOT_CHASE_BACK)
                    {
                        goto counter_attack;
                    }
                    if (motID == MOT_ENGAGE)
                    {
                        return;
                    }
                }
                if (motID != MOT_DAMAGE_GETUP)
                {
                    goto take_damage;
                }
            counter_attack:
                /* Retail's own redundancy: unreachable here with
                 * MOT_ENGAGE (both entries guard on MOT_CHASE_BACK/MOT_DAMAGE_GETUP), yet
                 * the binary carries the duplicate test — reproduced
                 * faithfully. */
                if (motID == MOT_ENGAGE)
                {
                    return;
                }
                if (UpdateMotion(dtM, MOT_ENGAGE) != 0)
                {
                    /* the blood/impact spawn point */
                    VECTOR *pp;

                    SnapToWaistConflict();
                    mmp = dtM;
                    dtR->vy += did;
                    Me_MOTION_C->status = STAT_ENGAGE;
                    mmp->count = 0;
                    PlayMotion(mmp, 1);
                    dmg = (u16)BattleDB[deg].power;
                    dtM->loop = -dmg - 8;
                    MoveHumanoid(Me_MOTION_C, -((short)((dmg * 5) / 2) + 0x50), 0);
                    RecoilAttacker(enemy, dmg, RUMBLE_POWER_HALF,
                                   RUMBLE_RELEASE_NONE);
                    DeleteConflict(ConflictObject[(short)id].model);
                    pp = GetAbsolutePosition(
                        Me_MOTION_C->model->object[MODEL_PART_HEAD], 0,
                        (short)(dmg * 10 + 100), 0);
                    t = 0;
                    do
                    {
                        pv.vx = rand() % 100 - 50;
                        pv.vy = rand() % 100 - 50;
                        pv.vz = rand() % 100 - 50;
                        SetBleed(pp, &pv, rand() % 20 + 20, COLOR_YELLOW);
                        t++;
                    } while (t < 10);
                    RequestDamageFeedback(enemy, RUMBLE_RELEASE_NONE);
                    {
                        s16 r;

                        r = rand() % 360;
                        set_impact_ex_(pp, 0, 2 * FIXED_ONE, 6 * FIXED_ONE, RGB24(220, 220, 220), 0, r, 6, 9, IMPACT_SPRITE_FLASH);
                    }
                    if ((rand() & 1) != 0)
                    {
                        Sound(Me_MOTION_C, CHAR_VOICE_ACTION_B);
                    }
                    if ((enemy->type & PAGE_MASK) != PAGE_BEAST)
                    {
                        Sound(Me_MOTION_C, CHAR_SE_ATTACK_ALT);
                    }
                    return;
                }
            }
        take_damage:
            {
                SnapToWaistConflict();
            }
            dmg = (u16)BattleDB[deg].power;
            if (enemy != StagePlayer)
            {
                goto npc_attack;
            }
            if ((Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN) != 0)
            {
                goto difficulty_bonus;
            }
            Me_MOTION_C->life = 0;
            goto recheck_attacker;
        npc_attack:
            if (Me_MOTION_C != StagePlayer)
            {
                dmg = dmg / 3;
            }
        recheck_attacker:
            if (enemy != StagePlayer)
            {
                goto apply_multipliers;
            }
        difficulty_bonus:
            dmg -= ((u8)gNannido - DIFFICULTY_HARD);
        apply_multipliers:
            if (enemy->type == NINKEN)
            {
                dmg = dmg * 6;
            }
            if (Me_MOTION_C->active_item == ACTIVE_ITEM_PROTECTION)
            {
                dmg = dmg / 3;
            }
            if (enemy->active_item == ACTIVE_ITEM_PROTECTION)
            {
                dmg = (u32)(dmg << 0x10) >> 0xf;
            }
            {
                if ((Me_MOTION_C == StagePlayer) && (ARMOUR_EQUIPPED_ != 0))
                {
                    dmg = (dmg * 7) / 10;
                }
                deg = dmg >> 3;
                if (deg > DAMAGE_MOTION_LAUNCH_TIER)
                {
                    deg = DAMAGE_MOTION_LAUNCH_TIER;
                }
                if (Me_MOTION_C->map.height > 0)
                {
                    deg = DAMAGE_MOTION_LAUNCH_TIER;
                }
                t = dmg * 5 / 2 + 0x50;
                newvy = dtR->vy + did;
                abs_direction = __builtin_abs(did);
                dtR->vy = newvy;
                if (abs_direction < ANGLE_QUADRANT)
                {
                    t = -t;
                }
                else
                {
                    dtR->vy = newvy - ANGLE_HALF;
                }
                if (deg == DAMAGE_MOTION_LAUNCH_TIER)
                {
                    MoveHumanoid(Me_MOTION_C,
                                 (__builtin_abs(did) > ANGLE_QUADRANT)
                                     ? DAMAGE_LAUNCH_SPEED
                                     : -DAMAGE_LAUNCH_SPEED,
                                 0);
                }
                else
                {
                    MoveHumanoid(Me_MOTION_C, t, 0);
                }
            }
            {
                s16 hp;

                hp = (u16)Me_MOTION_C->life - dmg;
                Me_MOTION_C->life = hp;
                if (hp <= 0)
                {
                    Me_MOTION_C->life = 0;
                    DeadHumanoid = Me_MOTION_C;
                    if (deg != DAMAGE_MOTION_LAUNCH_TIER)
                    {
                        SET_MOTION(MOT_DEAD, MOTION_MOVE_APPLY);
                        SET_NOW_MOTION_UNLESS_CVA(goto death_motion_set);
                    death_motion_set:
                        if ((rand() & 1) != 0)
                        {
                            SET_MOTION(MOT_DEAD_ALT, MOTION_MOVE_APPLY);
                        }
                    }
                    else
                    {
                        int abs_direction;

                        abs_direction = did;
                        if (abs_direction < 0)
                        {
                            abs_direction = -abs_direction;
                        }
                        if (abs_direction > ANGLE_QUADRANT)
                        {
                            deg += DAMAGE_MOTION_FROM_BEHIND_OFFSET;
                        }
                        dtM->mid = MOTION_ID_NONE;
                        motID = damagemotion[deg];
                    }
                    if (enemy == StagePlayer)
                    {
                        RecordPlayerKill(Me_MOTION_C);
                    }
                    if ((Me_MOTION_C->attribute & (ATTR_WEAPON_DRAWN | PHASE_ALERT)) != 0)
                    {
                        Sound(Me_MOTION_C, CHAR_VOICE_HURT_HEAVY);
                        reset_alert_duration();
                    }
                }
                else
                {
                    int abs_direction;

                    abs_direction = did;
                    if (abs_direction < 0)
                    {
                        abs_direction = -abs_direction;
                    }
                    if (abs_direction > ANGLE_QUADRANT)
                    {
                        deg += DAMAGE_MOTION_FROM_BEHIND_OFFSET;
                    }
                    dtM->mid = MOTION_ID_NONE;
                    SET_MOTION(damagemotion[deg], MOTION_MOVE_NONE);
                    reset_alert_duration();
                }
            }
            RecoilAttacker(enemy, dmg, RUMBLE_POWER_MAX,
                           RUMBLE_RELEASE_SHORT);
            DeleteConflict(ConflictObject[(short)id].model);
            p.vx = dtL->vx;
            p.vy = dtL->vy - Me_MOTION_C->height / 2;
            p.vz = dtL->vz;
            SetBlood(&p, 5, 120);
            SetImpact(&p, 6 * FIXED_ONE, IMPACT_SPRITE_HIT);
            RequestDamageFeedback(enemy, RUMBLE_RELEASE_LONG);
            {
                int r;
                short sound_id;

                PlayRandomHurtVoice();
                r = rand();
                sound_id = CHAR_SE_IMPACT;
                if ((r & 1) == 0 && Me_MOTION_C->life == 0)
                {
                    sound_id = CHAR_SE_SPECIAL;
                }
                Sound(enemy, sound_id);
            }
        }
    }
    if ((Me_MOTION_C->life == 0) && (Me_MOTION_C->item[ITEM_KAWARIMI] != 0))
    {
        ReqItemDefault(Me_MOTION_C, ITEM_KAWARIMI);
        Me_MOTION_C->life = Me_MOTION_C->lifemax;
        if (motID != MOT_DAMAGE_LAUNCH_BACK && motID != MOT_DAMAGE_LAUNCH_FORE)
        {
            SET_MOTION(MOT_DAMAGE_FRONT_HEAVY, MOTION_MOVE_APPLY);
        }
    }
    Me_MOTION_C->pad.time = 0;
    if ((dtM->mid == MOT_SWIM) || (dtM->mid == MOT_SWIM_STROKE))
    {
        SVECTOR *v;

        v = dtV;
        v->vz = 0;
        v->vx = 0;
        if (Me_MOTION_C->life != 0)
        {
            motMODE = MOTION_MOVE_UNSET;
            return;
        }
        SET_MOTION(MOT_DEAD_DROWN, MOTION_MOVE_APPLY);
    }
    SET_NOW_MOTION_UNLESS_CVA(return);
    return;
}
