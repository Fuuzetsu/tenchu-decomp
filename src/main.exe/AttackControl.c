#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackControl(void);
 *     MOTION.C:648, 74 src lines, frame 48 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct SVECTOR vect
 *     reg   $a1       short emid
 *     reg   $a2       short myid
 *     stack sp+26     short deg
 *     stack sp+24     short mydeg
 *     reg   $s0       struct Humanoid * enemy
 *     reg   $s0       struct Humanoid * human
 *     reg   $a1       short mid
 *     reg   $v0       struct Humanoid * enemy
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *StagePlayer;
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern short motID;
 *     extern short motMODE;
 *     extern short Criticals;
 *     extern short dtPAD;
 *     extern struct MotionManager *dtM;
 * END PSX.SYM */

extern Humanoid *Me_MOTION_C;

extern s16 UpdateMotion(MotionManager *mmp, motion_id mid);

void AttackControl(void)
{
    s16 mydeg;
    s16 deg;

    {
        Humanoid *enemy;

        if ((u16)Me_MOTION_C->type < N_PLAYABLE_CHARACTERS)
        {
            enemy = GetNearestHumanoid(Me_MOTION_C, Me_MOTION_C->width + 1000);
            if (enemy != NULL)
            {
                u16 type;
                s32 group;

                /* Bosses, civilians, and beasts do not trade blows; on story page 0,
                 * only armed kerai types 7-9 do. */
                type = enemy->type;
                group = type & PAGE_MASK;
                if (group == PAGE_BOSS)
                    goto reject_enemy;
                if (group > PAGE_BOSS)
                    goto check_high_group;
                if (group == PAGE_PALACE)
                    goto check_low_group;
                goto enemy_type_ok;

            check_high_group:
                if (group == PAGE_CIVILIAN)
                    goto reject_enemy;
                if (group == PAGE_BEAST)
                    goto reject_enemy;
                goto enemy_type_ok;

            check_low_group:
                if ((u16)(type - 7) < 3)
                    goto enemy_type_ok;

            reject_enemy:
                enemy = NULL;

            enemy_type_ok:

                if (enemy != NULL &&
                    (enemy->attribute & (ATTR_WEAPON_DRAWN | ATTR_PHASE)) == 0 &&
                    enemy->status != STAT_ITEM && enemy->status != STAT_ACTION)
                {
                    GsCOORDINATE2 *target;

                    Me_MOTION_C->target = &enemy->model->locate;
                    GetTargetDistance(Me_MOTION_C, &mydeg);
                    target = enemy->target;
                    enemy->target = &StagePlayer->model->locate;
                    GetTargetDistance(enemy, &deg);
                    enemy->target = target;

                    if (dtL->vy == enemy->locate->vy &&
                        Me_MOTION_C->map.vector == 0 &&
                        Me_MOTION_C->map.angleL == 0 &&
                        Me_MOTION_C->map.angleH == 0)
                    {
                        s16 myid;
                        motion_id emid;

                        if (__builtin_abs(deg) > 1000 &&
                            __builtin_abs(mydeg) < 1000)
                        {
                            myid = MOT_ATTACK_STEALTH_BACK;
                            emid = MOT_DEAD_STEALTH_BACK;
                        }
                        else if (__builtin_abs(deg) < 1000 &&
                                 __builtin_abs(mydeg) < 1000)
                        {
                            myid = MOT_ATTACK_STEALTH_FRONT;
                            emid = MOT_DEAD_STEALTH_FRONT;
                        }
                        else
                        {
                            myid = MOT_ATTACK_STEALTH_SIDE;
                            emid = MOT_DEAD_STEALTH_SIDE;
                        }
                        if (Me_MOTION_C->type == AYAME_0)
                        {
                            myid += 3;
                            emid += 3;
                        }

                        enemy->rotate->vy = dtR->vy;
                        enemy->locate->vx = dtL->vx;
                        SET_MOTION(myid, MOTION_MOVE_APPLY);
                        enemy->locate->vz = dtL->vz;
                        enemy->life = 0;
                        if ((enemy->status != STAT_DEAD ||
                             enemy->motion->loop != MOTION_LOOP_DISABLED) &&
                            UpdateMotion(enemy->motion, emid) != 0)
                        {
                            enemy->status = (s8)MOTION_STATUS(emid);
                            MoveHumanoid(enemy, enemy->motion->motion->orderspd,
                                         enemy->motion->motion->sidespd);
                        }
                        DeleteConflict(enemy->model->object[MODEL_PART_WAIST]);
                        Criticals++;
                        return;
                    }
                }
            }
        }
    }

    if (dtPAD & PADLdown)
    {
        if (GetMotionID(dtM, MOT_ATTACK_BACK) < 0)
        {
            return;
        }
        SET_MOTION(MOT_ATTACK_BACK, MOTION_MOVE_APPLY);
    }
    else if (motID == MOT_SQUAT)
    {
        if (GetMotionID(dtM, MOT_ATTACK_CROUCH) < 0)
        {
            return;
        }
        SET_MOTION(MOT_ATTACK_CROUCH, MOTION_MOVE_APPLY);
    }
    else if (motID == MOT_CHASE_DASH_FWD)
    {
        if (dtM->count > 10)
        {
            return;
        }
        SET_MOTION(MOT_ATTACK, MOTION_MOVE_APPLY);
        if (GetMotionID(dtM, MOT_ATTACK_LUNGE) >= 0)
        {
            SET_MOTION(MOT_ATTACK_LUNGE, MOTION_MOVE_APPLY);
        }
    }
    else
    {
        if (dtPAD & PADLup)
        {
            motID = MOT_ATTACK;
        }
        else if (dtPAD & PADLright)
        {
            motID = MOT_ATTACK_RIGHT1;
        }
        else if (dtPAD & PADLleft)
        {
            motID = MOT_ATTACK_LEFT1;
        }
        else
        {
            motID = MOT_ATTACK;
        }
        motMODE = MOTION_MOVE_APPLY;
    }

    if (Me_MOTION_C == StagePlayer)
    {
        Humanoid *enemy;
        Humanoid *human;

        enemy = GetNearestHumanoid(Me_MOTION_C, 3000);
        human = Me_MOTION_C;
        if (enemy != NULL)
        {
            human->target = &enemy->model->locate;
        }
        else
        {
            human->target = NULL;
        }
    }
}
