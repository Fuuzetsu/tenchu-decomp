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
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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

/*
 * AttackControl (0x8001ed70) -- select an attack motion and handle the
 * close-range critical-hit takeover of a nearby humanoid.
 *
 * STATUS: MATCHED -- exact 1040 bytes / 260 instructions.
 *
 * Matching notes:
 *  - The target-ordered labels in the enemy-kind filter preserve three
 *    physical branch islands that jump2 otherwise collapses.
 *  - The two `enemy` declarations deliberately have disjoint block scopes.
 *    PSX.SYM records the first in $s1 and the final retargeting value in $v0;
 *    one function-wide local instead survives into the tail and adds reloads.
 *  - The final `human` copy gives both target stores one shared base pseudo,
 *    while keeping the GetNearestHumanoid result directly in $v0.
 */

extern Humanoid *Me_MOTION_C;

extern s16 UpdateMotion(MotionManager *mmp, s16 mid);

void AttackControl(void)
{
    s16 mydeg;
    s16 deg;

    {
        Humanoid *enemy;

        if ((u16)Me_MOTION_C->type < 2)
        {
            enemy = GetNearestHumanoid(Me_MOTION_C, Me_MOTION_C->width + 1000);
            if (enemy != NULL)
            {
                u16 type;
                s32 group;

                /* Target filter: bosses (0x80), civilians (0x90) and
                 * beasts (0xa0) never trade blows; on the story page
                 * (group 0) only the armed kerai retainers (types 7-9)
                 * do. The goto ladder is byte-required: both a switch
                 * (branch polarity flips) and the structured chain
                 * (length change) were measured off. */
                type = enemy->type;
                group = type & 0xf0;
                if (group == 0x80)
                    goto reject_enemy;
                if (group >= 0x81)
                    goto check_high_group;
                if (group == 0)
                    goto check_low_group;
                goto enemy_type_ok;

            check_high_group:
                if (group == 0x90)
                    goto reject_enemy;
                if (group == 0xa0)
                    goto reject_enemy;
                goto enemy_type_ok;

            check_low_group:
                if ((u16)(type - 7) < 3)
                    goto enemy_type_ok;

            reject_enemy:
                enemy = NULL;

            enemy_type_ok:

                if (enemy != NULL &&
                    (enemy->attribute & (ATTR_ALERT | ATTR_PHASE)) == 0 &&
                    enemy->status != STAT_ITEM && enemy->status != STAT_ACTION)
                {
                    ModelType *target;

                    Me_MOTION_C->target = (ModelType *)enemy->model;
                    GetTargetDistance(Me_MOTION_C, &mydeg);
                    target = enemy->target;
                    enemy->target = (ModelType *)StagePlayer->model;
                    GetTargetDistance(enemy, &deg);
                    enemy->target = target;

                    if (dtL->vy == enemy->locate->vy &&
                        (*(u32 *)&Me_MOTION_C->map.vector & 0xffff00ff) == 0)
                    {
                        s16 myid;
                        s16 emid;

                        if (__builtin_abs(deg) > 1000 &&
                            __builtin_abs(mydeg) < 1000)
                        {
                            myid = 0x714;
                            emid = 0x1109;
                        }
                        else if (__builtin_abs(deg) < 1000 &&
                                 __builtin_abs(mydeg) < 1000)
                        {
                            myid = 0x715;
                            emid = 0x110a;
                        }
                        else
                        {
                            myid = 0x716;
                            emid = 0x110b;
                        }
                        if (Me_MOTION_C->type == 1)
                        {
                            myid += 3;
                            emid += 3;
                        }

                        enemy->rotate->vy = dtR->vy;
                        enemy->locate->vx = dtL->vx;
                        motID = myid;
                        motMODE = 1;
                        enemy->locate->vz = dtL->vz;
                        enemy->life = 0;
                        if ((enemy->status != STAT_DEAD || enemy->motion->loop != -1) &&
                            UpdateMotion(enemy->motion, emid) != 0)
                        {
                            enemy->status = (s8)(emid >> 8);
                            MoveHumanoid(enemy, enemy->motion->motion->orderspd,
                                         enemy->motion->motion->sidespd);
                        }
                        DeleteConflict(enemy->model->object[0]);
                        Criticals++;
                        return;
                    }
                }
            }
        }
    }

    if (MOTION_PAD_BITS & PADLdown)
    {
        if (GetMotionID(dtM, 0x711) < 0)
        {
            return;
        }
        motID = 0x711;
    }
    else if (motID == MOT_SQUAT)
    {
        if (GetMotionID(dtM, 0x70c) < 0)
        {
            return;
        }
        motID = 0x70c;
    }
    else if (motID == 0x607)
    {
        if (dtM->count > 10)
        {
            return;
        }
        motID = MOT_ATTACK;
        motMODE = 1;
        if (GetMotionID(dtM, 0x70d) >= 0)
        {
            motID = 0x70d;
            motMODE = 1;
        }
        goto update_target;
    }
    else if (MOTION_PAD_BITS & PADLup)
    {
        motID = MOT_ATTACK;
    }
    else if (MOTION_PAD_BITS & PADLright)
    {
        motID = 0x706;
    }
    else if (MOTION_PAD_BITS & PADLleft)
    {
        motID = 0x709;
    }
    else
    {
        motID = MOT_ATTACK;
    }
    motMODE = 1;

update_target:
    if (Me_MOTION_C == StagePlayer)
    {
        Humanoid *enemy;
        Humanoid *human;

        enemy = GetNearestHumanoid(Me_MOTION_C, 3000);
        human = Me_MOTION_C;
        if (enemy != NULL)
        {
            human->target = (ModelType *)enemy->model;
        }
        else
        {
            human->target = NULL;
        }
    }
}
