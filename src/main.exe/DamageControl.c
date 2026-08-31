#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "humanoid.h"
#include "item.h"



extern Humanoid *Me_MOTION_C;
extern s16 ARMOUR_EQUIPPED_;
extern Humanoid *DeadHumanoid;
/* MOTION.C's original direction-to-damage-animation table. */
extern s16 damagemotion[8];

extern int ReqLifeBar(Humanoid *h);
extern void reset_alert_duration(void);
extern void AttackCancelControl(s16 mode);
extern s16 PlayMotion(MotionManager *mmp, s16 mode);
extern void set_impact_ex_(VECTOR *pos, GsCOORDINATE2 *super,
                           short start_size, short end_size,
                           long start_color, long end_color,
                           /* s16 rotate vs the definition's u16 is retail's own drift -- byte-required. */
                           s16 rotate, u16 rotate_speed, u16 time, u16 type);
extern void SetBlood(VECTOR *pos, s16 n, s16 time);

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

/*
 * Byte-required spellings in this function (each measured; the
 * round-by-round derivation that found them is not repeated here).
 *
 * Structure
 *  - NO cached pointer locals. Every region re-loads Me_MOTION_C /
 *    StagePlayer / dtR / dtV; the per-region CSE temps then land in
 *    $a0/$v1 as retail has them. Function-spanning caches were what made
 *    earlier drafts look like they had unreachable "hard conflicts".
 *  - The 0x602 engage block is a plain nested if with an else arm, not a
 *    guard fence: `if (rand() % (EngageLevel + 1) == 0) { type checks;
 *    if (rand() & 1) motID = 0x602; } else { motID = 0x602; }`. Cross-jump
 *    plus eager delay fill produce the shared `sh motID` tail with a
 *    per-predecessor `li 0x602` in the delay slots. ITEM_NAPALM is likewise
 *    a plain `if ((rand() & 1) == 0) motID = 0x1003; else motID = 0x1001;`
 *    (no staging temp), and ITEM_MAKIBISHI stores motID/motMODE directly.
 *  - Both ReqLifeBar sites are if/else (`who = enemy` in the taken arm,
 *    else `who = Me_MOTION_C`), so `who` coalesces with the Me load in $a0
 *    and the else arm compiles to nothing.
 *  - The passage halving is a real `while` loop. Its loop notes weight the
 *    body's `t <<= 1` refs (p84 14 -> 16 refs, priority 2413 -> 3678),
 *    which is what orders t > deg > enemy = $s0/$s2/$s3.
 *  - The exit block loads dtV INSIDE the mid == 0x300/0x302 arm.
 *
 * Expressions
 *  - `-(x / 3) - 1` must be spelled `x / -3 - 1`: a negative divisor makes
 *    expmed emit the reversed magic-division subtract with a plain addiu -1.
 *  - Under `enemy->itmctl == ITEM_GOSIN`, the doubling is
 *    `(u32)(dmg << 0x10) >> 0xf` (sll 16 / srl 15). Spelling it through the
 *    short lvalue truncates to zero -- a real behaviour bug, not a match.
 *  - The armour block computes deg BEFORE the knockback
 *    (`deg = dmg >> 3;` clamp; clamp; `t = dmg * 5 / 2 + 0x50;`) with no
 *    cached `(u16)dmg << 16` temp, so every read re-extends dmg.
 *  - The knockback absolute value is the assigned form
 *    `ad = __builtin_abs(did);`. cc1's mips abssi2 is ONE insn whose
 *    template hides the branch, so reorg never steals the `move s0,a1`
 *    copy out of the lhu load-delay slot; the explicit `if (ad < 0)`
 *    spelling exposes a real branch that always does steal it.
 *  - The deg == 3 arm keeps the abs INSIDE the call's ternary argument:
 *    `MoveHumanoid(Me, (0x400 < __builtin_abs((int)(short)did)) ? 0x46
 *    : -0x46, 0)`. A move_speed variable costs +4 length.
 *
 * Widths and calls
 *  - `id` is an int loaded via `(u16)vector.pad` (lhu) with `(short)id`
 *    casts at every signed use. An s8 id is wrong (lbu/sll24).
 *  - GetAbsolutePosition's third argument is (short)-converted at the call
 *    site; set_impact_ex_'s rot argument is an s16 parameter in this TU;
 *    its `rand() % 360` is precomputed into a temp so the 0xB60B60B7 magic
 *    pair forms before the 0xDCDCDC pair.
 *
 * Fence-free. Matched: 5812 bytes / 1453 instructions, including the
 * compiled switch's own .rodata jump table.
 */

void DamageControl(void)
{
    MotionManager *mmp;
    short did;
    short deg;
    short t;
    short newvy;
    int ad;
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
        enemy = (Humanoid *)ConflictObject[(short)id].common;
        if (enemy != (Humanoid *)CONFLICT_OWNER_ITEM)
        {
            Sound(enemy, 4);
            DeleteConflict(ConflictObject[(short)id].model);
            deg = GetAttackDBID(enemy, enemy->motion->mid);
            {
                s16 hp;

                hp = (u16)Me_MOTION_C->life - (u16)BattleDB[deg].power;
                Me_MOTION_C->life = hp;
                if (hp < 0 || (Me_MOTION_C->attribute & ATTR_ALERT) == 0)
                {
                    Me_MOTION_C->life = 0;
                }
            }
            p.vx = dtL->vx;
            p.vy = dtL->vy - Me_MOTION_C->height / 2;
            p.vz = dtL->vz;
            SetImpact(&p, 0x6000, 2);
            if (StagePlayer == enemy)
            {
                PadShockAR(0, 0xff, 10, 10);
            }
        }
        else
        {
            Me_MOTION_C->life = 0;
        }
        ReqLifeBar(Me_MOTION_C);
        if (Me_MOTION_C->life != 0)
        {
            motID = MOT_DAMAGE;
            motMODE = 1;
            Sound(Me_MOTION_C, 6);
            reset_alert_duration();
        }
        else
        {
            motID = MOT_DEAD;
            motMODE = 1;
            if ((Me_MOTION_C->type != NINKEN) &&
                ((StagePlayer == enemy || (enemy == (Humanoid *)CONFLICT_OWNER_ITEM))))
            {
                if ((Me_MOTION_C->attribute & (ATTR_ALERT | PHASE_ALERT)) == 0)
                {
                    Criticals++;
                }
                else
                {
                    Murders++;
                }
            }
            if ((Me_MOTION_C->attribute & (ATTR_ALERT | PHASE_ALERT)) != 0)
            {
                Sound(Me_MOTION_C, 8);
                reset_alert_duration();
            }
        }
        SET_NOW_MOTION_UNLESS_CVA(goto attack_cancel);
    attack_cancel:
        AttackCancelControl(3);
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
    ActionHalt = 0;
    if (Me_MOTION_C == StagePlayer)
    {
        SetCameraMode(CMODE_NORMAL);
    }
    if ((Me_MOTION_C->attribute & ATTR_ALERT) != 0)
    {
        motID = MOT_ENGAGE_STANCE;
        motMODE = 1;
    }
    else
    {
        motID = 0;
        motMODE = 1;
    }
    dtL->vy--;
    return;
resolve_hit:
    dtM->mask = 0x7fff;
    AttackCancelControl(3);
    {
        Humanoid *conflict;

        t = id;
        conflict = (Humanoid *)ConflictObject[t].common;
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
                motID = MOT_DAMAGE_MAKIBISHI;
                motMODE = 1;
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
                motMODE = 1;
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
                motMODE = 1;
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
                int ad;

                did = GetDirection(ConflictDistance.vx, ConflictDistance.vz, dtR->vy);
                ad = did;
                if (ad < 0)
                {
                    ad = -ad;
                }
                if (ad < 0x400)
                {
                    motID = MOT_DAMAGE_LAUNCH_BACK;
                    motMODE = 0;
                    dtR->vy = dtR->vy + did;
                    MoveHumanoid(Me_MOTION_C, -0x46, 0);
                }
                else
                {
                    motID = MOT_DAMAGE_LAUNCH_FORE;
                    motMODE = 0;
                    dtR->vy = (0x800 + did) + dtR->vy;
                    MoveHumanoid(Me_MOTION_C, 0x46, 0);
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
                    if ((u32)(u16)motID - MOT_DAMAGE_LAUNCH_BACK > 1)
                    {
                        motID = MOT_DEAD;
                        motMODE = 1;
                    }
                    Sound(Me_MOTION_C, 8);
                    {
                        TItemType item_type;

                        item_type = GetItemType((s16)id);
                        if ((item_type < ITEM_GUN) ||
                            (item_type > ITEM_ARROW && item_type != ITEM_LIGHTNINGBOLT))
                        {
                            if ((Me_MOTION_C->type & PAGE_MASK) == PAGE_CIVILIAN)
                            {
                                FriendHits++;
                            }
                            else if ((Me_MOTION_C->attribute & (ATTR_ALERT | PHASE_ALERT)) == 0)
                            {
                                Criticals++;
                            }
                            else
                            {
                                Murders++;
                            }
                        }
                    }
                }
                else
                {
                    int r;
                    short sound_id;

                    r = rand();
                    sound_id = 7;
                    if ((r & 1) != 0)
                    {
                        sound_id = 6;
                    }
                    Sound(Me_MOTION_C, sound_id);
                }
            }
            if (StagePlayer == Me_MOTION_C)
            {
                PadShockAR(0, 0xff, 10, 20);
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
            if (((Me_MOTION_C->type & PAGE_MASK) == PAGE_BOSS) && (enemy != StagePlayer))
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
            p.vy = p.vy - 1000;
            if (GetAreaMapPassage(GlobalAreaMap, &p, &dir, t) != 0)
            {
                return;
            }
            {
                int ad;

                did = GetDirection(enemy->locate->vx - dtL->vx,
                                   enemy->locate->vz - dtL->vz, dtR->vy);
                deg = GetAttackDBID(enemy, enemy->motion->mid);
                if (Me_MOTION_C != StagePlayer)
                {
                    if ((((Me_MOTION_C->status != STAT_ATTACK) &&
                          ((Me_MOTION_C->attribute & ATTR_ALERT) != 0)) &&
                         (Me_MOTION_C->map.height == 0)) &&
                        (gNannido != DIFFICULTY_EASY))
                    {
                        if (rand() % (EngageLevel + 1) == 0)
                        {
                            if ((Me_MOTION_C->type == NINJA_0) || (Me_MOTION_C->type == NINJA_1))
                            {
                                if ((rand() & 1) != 0)
                                {
                                    motID = MOT_CHASE_BACK;
                                }
                            }
                        }
                        else
                        {
                            motID = MOT_CHASE_BACK;
                        }
                    }
                }
                ad = did;
                if (ad < 0)
                {
                    ad = -ad;
                }
                if (ad < 700)
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
                    int conflict_id;
                    VECTOR *blood_pos;

                    conflict_id = Me_MOTION_C->model->object[0]->id;
                    if (conflict_id >= 0)
                    {
                        dtL->vx = ConflictObject[conflict_id].position.vx;
                        dtL->vz = ConflictObject[conflict_id].position.vz;
                    }
                    mmp = dtM;
                    dtR->vy = dtR->vy + did;
                    Me_MOTION_C->status = STAT_ENGAGE;
                    mmp->count = 0;
                    PlayMotion(mmp, 1);
                    dmg = (u16)BattleDB[deg].power;
                    dtM->loop = -dmg - 8;
                    MoveHumanoid(Me_MOTION_C, -((short)((dmg * 5) / 2) + 0x50), 0);
                    if (enemy->status == STAT_ATTACK)
                    {
                        enemy->motion->loop = dmg / -3 - 1;
                        enemy->vector.vz = 0;
                        enemy->vector.vx = 0;
                        if (StagePlayer == enemy)
                        {
                            PadShockAR(0, 0x7f, 10, 0);
                        }
                    }
                    DeleteConflict(ConflictObject[(short)id].model);
                    blood_pos = GetAbsolutePosition(Me_MOTION_C->model->object[2], 0, (short)(dmg * 10 + 100), 0);
                    t = 0;
                    do
                    {
                        pv.vx = rand() % 100 - 50;
                        pv.vy = rand() % 100 - 50;
                        pv.vz = rand() % 100 - 50;
                        SetBleed(blood_pos, &pv, rand() % 20 + 20, 0xffff00);
                        t++;
                    } while (t < 10);
                    {
                        Humanoid *who;

                        if (StagePlayer == Me_MOTION_C)
                        {
                            PadShockAR(0, 0x7f, 10, 0);
                            who = enemy;
                        }
                        else
                        {
                            who = Me_MOTION_C;
                        }
                        ReqLifeBar(who);
                    }
                    {
                        s16 r;

                        r = rand() % 360;
                        set_impact_ex_(blood_pos, 0, 0x2000, 0x6000, 0xdcdcdc, 0, r, 6, 9, 1);
                    }
                    if ((rand() & 1) != 0)
                    {
                        Sound(Me_MOTION_C, 10);
                    }
                    if ((enemy->type & PAGE_MASK) != PAGE_BEAST)
                    {
                        Sound(Me_MOTION_C, 3);
                    }
                    return;
                }
            }
        take_damage:
            {
                int conflict_id;

                conflict_id = Me_MOTION_C->model->object[0]->id;
                if (conflict_id >= 0)
                {
                    dtL->vx = ConflictObject[conflict_id].position.vx;
                    dtL->vz = ConflictObject[conflict_id].position.vz;
                }
            }
            dmg = (u16)BattleDB[deg].power;
            if (enemy != StagePlayer)
            {
                goto npc_attack;
            }
            if ((Me_MOTION_C->attribute & ATTR_ALERT) != 0)
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
            dmg = dmg - ((u8)gNannido - DIFFICULTY_HARD);
        apply_multipliers:
            if (enemy->type == NINKEN)
            {
                dmg = dmg * 6;
            }
            if (Me_MOTION_C->itmctl == ITEM_GOSIN)
            {
                dmg = dmg / 3;
            }
            if (enemy->itmctl == ITEM_GOSIN)
            {
                dmg = (u32)(dmg << 0x10) >> 0xf;
            }
            {
                if ((Me_MOTION_C == StagePlayer) && (ARMOUR_EQUIPPED_ != 0))
                {
                    dmg = (dmg * 7) / 10;
                }
                deg = dmg >> 3;
                if (deg > 3)
                {
                    deg = 3;
                }
                if (Me_MOTION_C->map.height > 0)
                {
                    deg = 3;
                }
                t = dmg * 5 / 2 + 0x50;
                newvy = dtR->vy + did;
                ad = __builtin_abs(did);
                dtR->vy = newvy;
                if (ad < 0x400)
                {
                    t = -t;
                }
                else
                {
                    dtR->vy = newvy - 0x800;
                }
                if (deg == 3)
                {
                    MoveHumanoid(Me_MOTION_C,
                                 (__builtin_abs(did) > 0x400) ? 0x46 : -0x46, 0);
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
                    if (deg == 3)
                    {
                        goto directional_death;
                    }
                    motID = MOT_DEAD;
                    motMODE = 1;
                    SET_NOW_MOTION_UNLESS_CVA(goto death_motion_set);
                death_motion_set:
                    if ((rand() & 1) != 0)
                    {
                        motID = MOT_DEAD_ALT;
                        motMODE = 1;
                    }
                    goto score_kill;
                directional_death:
                {
                    int ad;

                    ad = did;
                    if (ad < 0)
                    {
                        ad = -ad;
                    }
                    if (ad > 0x400)
                    {
                        deg = deg + 4;
                    }
                    dtM->mid = -1;
                    motID = damagemotion[deg];
                }
                score_kill:
                    if (enemy == StagePlayer)
                    {
                        if ((Me_MOTION_C->type & PAGE_MASK) == PAGE_CIVILIAN)
                        {
                            FriendHits++;
                        }
                        else if ((Me_MOTION_C->attribute & (ATTR_ALERT | PHASE_ALERT)) == 0)
                        {
                            Criticals++;
                        }
                        else
                        {
                            Murders++;
                        }
                    }
                    if ((Me_MOTION_C->attribute & (ATTR_ALERT | PHASE_ALERT)) != 0)
                    {
                        Sound(Me_MOTION_C, 8);
                        goto alerted;
                    }
                }
                else
                {
                    int ad;

                    ad = did;
                    if (ad < 0)
                    {
                        ad = -ad;
                    }
                    if (ad > 0x400)
                    {
                        deg = deg + 4;
                    }
                    dtM->mid = -1;
                    motID = damagemotion[deg];
                    motMODE = 0;
                alerted:
                    reset_alert_duration();
                }
            }
            if (enemy->status == STAT_ATTACK)
            {
                enemy->motion->loop = dmg / -3 - 1;
                enemy->vector.vz = 0;
                enemy->vector.vx = 0;
                if (StagePlayer == enemy)
                {
                    PadShockAR(0, 0xff, 10, 10);
                }
            }
            DeleteConflict(ConflictObject[(short)id].model);
            p.vx = dtL->vx;
            p.vy = dtL->vy - Me_MOTION_C->height / 2;
            p.vz = dtL->vz;
            SetBlood(&p, 5, 120);
            SetImpact(&p, 0x6000, 2);
            {
                Humanoid *who;

                if (StagePlayer == Me_MOTION_C)
                {
                    PadShockAR(0, 0x7f, 10, 30);
                    who = enemy;
                }
                else
                {
                    who = Me_MOTION_C;
                }
                ReqLifeBar(who);
            }
            {
                int r;
                short sound_id;

                r = rand();
                sound_id = 7;
                if ((r & 1) != 0)
                {
                    sound_id = 6;
                }
                Sound(Me_MOTION_C, sound_id);
                r = rand();
                sound_id = 4;
                if ((r & 1) == 0)
                {
                    if (Me_MOTION_C->life == 0)
                    {
                        sound_id = 5;
                    }
                }
                Sound(enemy, sound_id);
            }
        }
    }
    if ((Me_MOTION_C->life == 0) && (Me_MOTION_C->item[ITEM_KAWARIMI] != 0))
    {
        ReqItemDefault(Me_MOTION_C, ITEM_KAWARIMI);
        Me_MOTION_C->life = Me_MOTION_C->lifemax;
        if ((u32)(u16)motID - MOT_DAMAGE_LAUNCH_BACK > 1)
        {
            motID = MOT_DAMAGE_FRONT_HEAVY;
            motMODE = 1;
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
            motMODE = -1;
            return;
        }
        motID = MOT_DEAD_DROWN;
        motMODE = 1;
    }
    SET_NOW_MOTION_UNLESS_CVA(return);
    return;
}
