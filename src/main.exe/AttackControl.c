#include "common.h"
#include "main.exe.h"
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
extern u16 dtPAD;
extern s16 motID;
extern s16 motMODE;

extern Humanoid *GetNearestHumanoid(Humanoid *human, s16 distance);
extern s16 UpdateMotion(MotionManager *mmp, s16 mid);
extern void MoveHumanoid(Humanoid *human, s16 order, s16 side);

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
                    (enemy->attribute & 0x43U) == 0 &&
                    enemy->status != 0xf && enemy->status != 1)
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
                        if ((enemy->status != 0x11 || enemy->motion->loop != -1) &&
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

    if (dtPAD & 0x4000)
    {
        if (GetMotionID(dtM, 0x711) < 0)
        {
            return;
        }
        motID = 0x711;
    }
    else if (motID == 0xb00)
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
        motID = 0x700;
        motMODE = 1;
        if (GetMotionID(dtM, 0x70d) >= 0)
        {
            motID = 0x70d;
            motMODE = 1;
        }
        goto update_target;
    }
    else if (dtPAD & 0x1000)
    {
        motID = 0x700;
    }
    else if (dtPAD & 0x2000)
    {
        motID = 0x706;
    }
    else if (dtPAD & 0x8000)
    {
        motID = 0x709;
    }
    else
    {
        motID = 0x700;
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

// triage: MEDIUM — 260 insns, 6 callees, ~0.06 to ProcItemDrop
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void AttackControl(void)
//
// {
//   VECTOR *pVVar1;
//   VECTOR *pVVar2;
//   short sVar3;
//   ushort uVar4;
//   Humanoid *human;
//   int iVar5;
//   MotionDataType *pMVar6;
//   Humanoid *pHVar7;
//   uint uVar8;
//   short sVar9;
//   ModelType *pMVar10;
//   short local_18;
//   short local_16 [3];
//
//   if (((ushort)Me_MOTION_C->type < 2) &&
//      (human = GetNearestHumanoid(Me_MOTION_C,Me_MOTION_C->width + 1000), pHVar7 = Me_MOTION_C,
//      human != (Humanoid *)0x0)) {
//     uVar4 = human->type;
//     uVar8 = uVar4 & 0xf0;
//     if (uVar8 == 0x80) {
// LAB_8001ee10:
//       human = (Humanoid *)0x0;
//     }
//     else if (uVar8 < 0x81) {
//       if (((uVar4 & 0xf0) == 0) && (2 < uVar4 - 7)) goto LAB_8001ee10;
//     }
//     else if ((uVar8 == 0x90) || (uVar8 == 0xa0)) goto LAB_8001ee10;
//     if ((((human != (Humanoid *)0x0) && ((human->attribute & 0x43U) == 0)) && (human->status != 0xf)
//         ) && (human->status != 1)) {
//       Me_MOTION_C->target = (ModelType *)human->model;
//       GetTargetDistance(pHVar7,&local_18);
//       pMVar10 = human->target;
//       human->target = (ModelType *)StagePlayer->model;
//       GetTargetDistance(human,local_16);
//       pVVar1 = dtL;
//       human->target = pMVar10;
//       pVVar2 = dtL;
//       if ((pVVar1->vy == human->locate->vy) &&
//          (uVar8._0_1_ = (Me_MOTION_C->map).vector, uVar8._1_1_ = (Me_MOTION_C->map).direct,
//          uVar8._2_1_ = (Me_MOTION_C->map).angleL, uVar8._3_1_ = (Me_MOTION_C->map).angleH,
//          (uVar8 & 0xffff00ff) == 0)) {
//         iVar5 = (int)local_16[0];
//         if (iVar5 < 0) {
//           iVar5 = -iVar5;
//         }
//         if (1000 < iVar5) {
//           iVar5 = (int)local_18;
//           if (iVar5 < 0) {
//             iVar5 = -iVar5;
//           }
//           motID = 0x714;
//           if (iVar5 < 1000) {
//             sVar9 = 0x1109;
//             goto LAB_8001ef4c;
//           }
//         }
//         iVar5 = (int)local_16[0];
//         if (iVar5 < 0) {
//           iVar5 = -iVar5;
//         }
//         if (iVar5 < 1000) {
//           iVar5 = (int)local_18;
//           if (iVar5 < 0) {
//             iVar5 = -iVar5;
//           }
//           motID = 0x715;
//           if (iVar5 < 1000) {
//             sVar9 = 0x110a;
//             goto LAB_8001ef4c;
//           }
//         }
//         motID = 0x716;
//         sVar9 = 0x110b;
// LAB_8001ef4c:
//         if (Me_MOTION_C->type == 1) {
//           motID = motID + 3;
//           sVar9 = sVar9 + 3;
//         }
//         human->rotate->vy = dtR->vy;
//         human->locate->vx = pVVar2->vx;
//         DAT_80097f0e = 1;
//         human->locate->vz = pVVar2->vz;
//         human->life = 0;
//         if (((human->status != 0x11) || (human->motion->loop != -1)) &&
//            (sVar3 = UpdateMotion(human->motion,sVar9), sVar3 != 0)) {
//           human->status = (short)(char)((ushort)sVar9 >> 8);
//           pMVar6 = human->motion->motion;
//           MoveHumanoid(human,(ushort)pMVar6->orderspd,(ushort)pMVar6->sidespd);
//         }
//         DeleteConflict(*human->model->object);
//         Criticals = Criticals + 1;
//         return;
//       }
//     }
//   }
//   if ((dtPAD & 0x4000) == 0) {
//     if (motID == 0xb00) {
//       uVar4 = GetMotionID(dtM,0x70c);
//       sVar3 = 0x70c;
//       sVar9 = motID;
//       goto joined_r0x8001f09c;
//     }
//     if (motID == 0x607) {
//       if (10 < dtM->count) {
//         return;
//       }
//       motID = 0x700;
//       DAT_80097f0e = 1;
//       uVar4 = GetMotionID(dtM,0x70d);
//       if (-1 < (int)((uint)uVar4 << 0x10)) {
//         motID = 0x70d;
//         DAT_80097f0e = 1;
//       }
//       goto LAB_8001f130;
//     }
//     motID = 0x700;
//     if ((((dtPAD & 0x1000) == 0) && (motID = 0x706, (dtPAD & 0x2000) == 0)) &&
//        (motID = 0x709, (dtPAD & 0x8000) == 0)) {
//       motID = 0x700;
//     }
//   }
//   else {
//     uVar4 = GetMotionID(dtM,0x711);
//     sVar3 = 0x711;
//     sVar9 = motID;
// joined_r0x8001f09c:
//     motID = sVar3;
//     if ((int)((uint)uVar4 << 0x10) < 0) {
//       motID = sVar9;
//       return;
//     }
//   }
//   DAT_80097f0e = 1;
// LAB_8001f130:
//   if (Me_MOTION_C == StagePlayer) {
//     pHVar7 = GetNearestHumanoid(Me_MOTION_C,3000);
//     if (pHVar7 == (Humanoid *)0x0) {
//       Me_MOTION_C->target = (ModelType *)0x0;
//     }
//     else {
//       Me_MOTION_C->target = (ModelType *)pHVar7->model;
//     }
//   }
//   return;
// }
