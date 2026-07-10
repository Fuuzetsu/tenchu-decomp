#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActATTACK(void);
 *     MOTION.C:1306, 197 src lines, frame 128 bytes, saved-reg mask 0x80030000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $s1       struct BattleType * battle
 *     reg   $v1       struct ModelType ** object
 *     stack sp+16     struct ModelType *[2] hand
 *     reg   $a2       struct OrnamentType ** weapon
 *     reg   $v0       struct VECTOR * pos
 *     stack sp+24     struct PARAM_ITEM_LAUNCH item
 *     reg   $v0       struct VECTOR * pos
 *     stack sp+24     struct PARAM_ITEM_LAUNCH item
 *     stack sp+104    struct SVECTOR vect
 *     reg   $v0       struct VECTOR * pos
 *     stack sp+64     struct PARAM_ITEM_LAUNCH item
 *     reg   $v1       short i
 *     reg   $v1       short i
 *     reg   $v1       short i
 *     reg   $a2       struct OrnamentType ** weapon
 *     reg   $v1       short i
 *     reg   $a2       struct OrnamentType ** weapon
 *     reg   $v1       short i
 *     reg   $a3       struct ModelType * waist
 *     reg   $v1       short i
 *     reg   $t0       struct AfterimageType * ilu
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short motID;
 *     extern struct BattleType BattleDB[78];
 *     extern struct SVECTOR *dtR;
 *     extern struct VECTOR *dtL;
 *     extern short dtPAD;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct SVECTOR *dtV;
 *     extern short ActionHalt;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

/*
 * ActATTACK (0x80021d64) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=ActATTACK
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActATTACK", ActATTACK);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActATTACK", switchD_80021f18__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActATTACK", switchD_80021f18__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActATTACK", switchD_80021f18__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActATTACK", switchD_80021f18__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActATTACK", switchD_80021f18__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActATTACK", switchD_80021f18__caseD_6);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActATTACK", switchD_80021f18__caseD_9);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActATTACK", switchD_80021f18__caseD_f);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActATTACK", switchD_80021f18__caseD_10);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActATTACK", switchD_80021f18__caseD_13);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActATTACK", switchD_80021f18__caseD_14);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActATTACK", switchD_80021f18__caseD_2);

/* jump-table pool @ 0x800114a8 (26 words; tables at 0x800114a8) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 ActATTACK_jtbl[26] = {
    0x80021F20, 0x80022274, 0x80022F1C, 0x80022F1C,
    0x800223C8, 0x80022490, 0x800224B8, 0x80022F1C,
    0x80022F1C, 0x80022608, 0x80022F1C, 0x80022F1C,
    0x80022F1C, 0x80022F1C, 0x80022F1C, 0x800227EC,
    0x80022B54, 0x80022F1C, 0x80022F1C, 0x80022D4C,
    0x80022D88, 0x80022D88, 0x80022D88, 0x80022D88,
    0x80022D88, 0x80022D88,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// void ActATTACK(void)
// 
// {
//   short *psVar1;
//   bool bVar2;
//   OrnamentType **ppOVar3;
//   Humanoid *pHVar4;
//   SVECTOR *pSVar5;
//   MotionManager *pMVar6;
//   short sVar7;
//   short sVar8;
//   short sVar9;
//   short sVar10;
//   MotionDataType *pMVar11;
//   VECTOR *pVVar12;
//   int iVar13;
//   AfterimageType *pAVar14;
//   int iVar15;
//   ModelType **ppMVar16;
//   ModelType *pMVar17;
//   short sVar18;
//   BattleType *battle;
//   ModelType *local_70;
//   ModelType *local_6c;
//   SVECTOR local_68 [5];
//   PARAM_ITEM_USE local_40;
//   SVECTOR local_18;
//   
//   if (dtM->count == 1) {
//     if (Me_MOTION_C->life == 0) {
//       motID = 0x1100;
//       DAT_80097f0e = 1;
//       return;
//     }
//     sVar7 = GetAttackDBID(Me_MOTION_C,motID);
//     pHVar4 = Me_MOTION_C;
//     Me_MOTION_C->warid = sVar7;
//     sVar7 = 0xb;
//     if ((motID != 0x713) && (sVar7 = 10, (motID & 1U) != 0)) {
//       sVar7 = 9;
//     }
//     Sound(pHVar4,sVar7);
//   }
//   sVar7 = Me_MOTION_C->warid;
//   pMVar17 = Me_MOTION_C->target;
//   battle = BattleDB + sVar7;
//   if (((pMVar17 != (ModelType *)0x0) && (dtM->count < BattleDB[sVar7].revise)) && (-1 < dtM->count))
//   {
//     sVar8 = GetDirection((pMVar17->locate).coord.t[0] - dtL->vx,
//                          (pMVar17->locate).coord.t[2] - dtL->vz,dtR->vy);
//     pHVar4 = Me_MOTION_C;
//     if ((int)Me_MOTION_C->turn < (int)sVar8) {
//       sVar8 = dtR->vy + 100;
//     }
//     else {
//       if (-(int)Me_MOTION_C->turn <= (int)sVar8) goto LAB_80021edc;
//       sVar8 = dtR->vy + -100;
//     }
//     dtR->vy = sVar8;
//     pMVar11 = pHVar4->motion->motion;
//     MoveHumanoid(pHVar4,(ushort)pMVar11->orderspd,(ushort)pMVar11->sidespd);
//   }
// LAB_80021edc:
//   pVVar12 = dtL;
//   pHVar4 = Me_MOTION_C;
//   switch((int)(((ushort)dtM->mid - 0x700) * 0x10000) >> 0x10) {
//   case 0:
//     sVar8 = GetMotionID(dtM,0x700);
//     pHVar4 = Me_MOTION_C;
//     if (sVar8 == 0xe9) {
//       FUN_8001f6b8(0xd);
//     }
//     else if (sVar8 < 0xea) {
//       if (sVar8 == 0xab) {
//         if (dtM->count == 0x14) {
//           ppMVar16 = Me_MOTION_C->model->object;
//           sVar8 = 100;
// LAB_800220a0:
//           pVVar12 = GetAbsolutePosition(ppMVar16[0xd],0,sVar8,-100);
//           FUN_80027554(0x14,pVVar12);
//           Sound(Me_MOTION_C,2);
//         }
//       }
//       else if (sVar8 < 0xac) {
//         if (sVar8 == 0xaa) {
// LAB_800221b4:
//           AttackBowControl();
//         }
//       }
//       else if ((sVar8 == 0xac) && (dtM->count == 0x16)) {
//         ppMVar16 = Me_MOTION_C->model->object;
//         sVar8 = 700;
//         goto LAB_800220a0;
//       }
//     }
//     else if (sVar8 == 0xf5) {
//       sVar8 = dtM->count;
//       if ((0x23 < sVar8) && (sVar8 < 0x42)) {
//         if (sVar8 == 0x24) {
//           Sound(Me_MOTION_C,0x28);
//         }
//         local_40.type = ITEM_NAPALM;
//         local_40.user = Me_MOTION_C;
//         pVVar12 = GetAbsolutePosition(Me_MOTION_C->model->object[2],0,-100,-300);
//         local_40.start.vx = pVVar12->vx;
//         local_40.start.vy = pVVar12->vy;
//         local_40.start.vz = pVVar12->vz;
//         GetMoveSpeed(&local_18,dtR->vy,100,0);
//         local_40.end.vx = local_40.start.vx + local_18.vx;
//         local_40.end.vz = local_40.start.vz + local_18.vz;
//         local_40.end.vy = local_40.start.vy;
//         ReqItemUse(&local_40);
//       }
//     }
//     else if (sVar8 < 0xf6) {
//       if (sVar8 == 0xf1) {
//         if (dtM->count == 0x2a) {
//           if (Me_MOTION_C->weapon[3] != (OrnamentType *)0x0) {
//             ppOVar3 = Me_MOTION_C->weapon;
//             Me_MOTION_C->weapon[2] = Me_MOTION_C->weapon[0];
//             pHVar4->weapon[0] = ppOVar3[3];
//             pHVar4->weapon[3] = (OrnamentType *)0x0;
//             Sound(pHVar4,1);
//           }
//         }
//         else if ((dtM->count == 6) && (Me_MOTION_C->weapon[2] != (OrnamentType *)0x0)) {
//           ppOVar3 = Me_MOTION_C->weapon;
//           Me_MOTION_C->weapon[3] = Me_MOTION_C->weapon[0];
//           pHVar4->weapon[0] = ppOVar3[2];
//           pHVar4->weapon[2] = (OrnamentType *)0x0;
//           Sound(pHVar4,0);
//         }
//       }
//     }
//     else if (sVar8 == 0x1a4) goto LAB_800221b4;
//     if ((((Me_MOTION_C->pad).trig & 0x80) != 0) &&
//        (sVar8 = AttackContinuousCheck(battle), sVar8 != 0)) {
//       motID = 0x702;
//       if (((dtPAD & 0x2000) == 0) && (motID = 0x703, (dtPAD & 0x8000) == 0)) {
//         motID = 0x701;
//       }
//       iVar13 = 0;
//       if (MotionUpdateMode != 0) {
//         iVar15 = 0;
//         do {
//           iVar13 = iVar13 + 1;
//           if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar15 >> 0xd)) == Me_MOTION_C)
//           goto LAB_800226f8;
//           iVar15 = iVar13 * 0x10000;
//         } while (iVar13 * 0x10000 >> 0x10 < 5);
//       }
// LAB_80022760:
//       DAT_80097f0e = 1;
//       sVar8 = SetNowMotion(Me_MOTION_C,motID,1);
//       DAT_80097f0e = -1;
//       goto LAB_80022780;
//     }
//     break;
//   case 1:
//     sVar8 = Me_MOTION_C->wpatk;
//     if (sVar8 == 0x2a) {
//       if (dtM->count == 0x34) {
//         if (Me_MOTION_C->weapon[3] != (OrnamentType *)0x0) {
//           ppOVar3 = Me_MOTION_C->weapon;
//           Me_MOTION_C->weapon[2] = Me_MOTION_C->weapon[0];
//           pHVar4->weapon[0] = ppOVar3[3];
//           pHVar4->weapon[3] = (OrnamentType *)0x0;
//           Sound(pHVar4,1);
//         }
//       }
//       else if ((dtM->count == 1) && (Me_MOTION_C->weapon[2] != (OrnamentType *)0x0)) {
//         ppOVar3 = Me_MOTION_C->weapon;
//         Me_MOTION_C->weapon[3] = Me_MOTION_C->weapon[0];
//         pHVar4->weapon[0] = ppOVar3[2];
//         pHVar4->weapon[2] = (OrnamentType *)0x0;
//         Sound(pHVar4,0);
//       }
//     }
//     else if (sVar8 == 0x29) {
//       FUN_8001f6b8(0xd);
//     }
//     else if (sVar8 == 0x35) {
//       AttackBowControl();
//     }
//     if ((((Me_MOTION_C->pad).trig & 0x80) != 0) &&
//        (sVar8 = AttackContinuousCheck(battle), sVar8 != 0)) {
//       motID = 0x704;
//       iVar13 = 0;
//       if (MotionUpdateMode != 0) {
//         iVar15 = 0;
//         do {
//           iVar13 = iVar13 + 1;
//           if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar15 >> 0xd)) == Me_MOTION_C)
//           goto LAB_800226f8;
//           iVar15 = iVar13 * 0x10000;
//         } while (iVar13 * 0x10000 >> 0x10 < 5);
//       }
//       goto LAB_80022760;
//     }
//     break;
//   case 4:
//     if (Me_MOTION_C->wpatk == 0x29) {
//       FUN_8001f6b8(0xd);
//     }
//     else if (Me_MOTION_C->wpatk == 0x35) {
//       AttackBowControl();
//     }
//     if ((((Me_MOTION_C->pad).trig & 0x80) != 0) &&
//        (sVar8 = AttackContinuousCheck(battle), sVar8 != 0)) {
//       motID = 0x705;
//       iVar13 = 0;
//       if (MotionUpdateMode != 0) {
//         iVar15 = 0;
//         do {
//           iVar13 = iVar13 + 1;
//           if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar15 >> 0xd)) == Me_MOTION_C)
//           goto LAB_800226f8;
//           iVar15 = iVar13 * 0x10000;
//         } while (iVar13 * 0x10000 >> 0x10 < 5);
//       }
//       goto LAB_80022760;
//     }
//     break;
//   case 5:
//     if (Me_MOTION_C->wpatk == 0x29) {
//       FUN_8001f6b8(0xd);
//     }
//     break;
//   case 6:
//     if (Me_MOTION_C->wpatk == 0x2a) {
//       if (dtM->count == 0x34) {
//         sVar8 = 1;
//         if (Me_MOTION_C->weapon[3] != (OrnamentType *)0x0) {
//           ppOVar3 = Me_MOTION_C->weapon;
//           Me_MOTION_C->weapon[2] = Me_MOTION_C->weapon[0];
//           pHVar4->weapon[0] = ppOVar3[3];
//           pHVar4->weapon[3] = (OrnamentType *)0x0;
// LAB_80022540:
//           Sound(pHVar4,sVar8);
//         }
//       }
//       else if ((dtM->count == 0x10) && (sVar8 = 0, Me_MOTION_C->weapon[2] != (OrnamentType *)0x0)) {
//         ppOVar3 = Me_MOTION_C->weapon;
//         Me_MOTION_C->weapon[3] = Me_MOTION_C->weapon[0];
//         pHVar4->weapon[0] = ppOVar3[2];
//         pHVar4->weapon[2] = (OrnamentType *)0x0;
//         goto LAB_80022540;
//       }
//     }
//     if (((((Me_MOTION_C->pad).trig & 0x80) != 0) && (((int)(short)dtPAD & 0xa000U) != 0)) &&
//        (sVar8 = AttackContinuousCheck(battle), sVar8 != 0)) {
//       motID = 0x708;
//       if (((int)(short)dtPAD & 0x8000U) == 0) {
//         motID = 0x707;
//       }
//       iVar13 = 0;
//       if (MotionUpdateMode != 0) {
//         iVar15 = 0;
//         do {
//           iVar13 = iVar13 + 1;
//           if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar15 >> 0xd)) == Me_MOTION_C)
//           goto LAB_800226f8;
//           iVar15 = iVar13 * 0x10000;
//         } while (iVar13 * 0x10000 >> 0x10 < 5);
//       }
//       goto LAB_80022760;
//     }
//     break;
//   case 9:
//     if (Me_MOTION_C->wpatk == 0x2a) {
//       if (dtM->count == 0x2b) {
//         sVar8 = 1;
//         if (Me_MOTION_C->weapon[3] != (OrnamentType *)0x0) {
//           ppOVar3 = Me_MOTION_C->weapon;
//           Me_MOTION_C->weapon[2] = Me_MOTION_C->weapon[0];
//           pHVar4->weapon[0] = ppOVar3[3];
//           pHVar4->weapon[3] = (OrnamentType *)0x0;
// LAB_80022690:
//           Sound(pHVar4,sVar8);
//         }
//       }
//       else if ((dtM->count == 0xd) && (sVar8 = 0, Me_MOTION_C->weapon[2] != (OrnamentType *)0x0)) {
//         ppOVar3 = Me_MOTION_C->weapon;
//         Me_MOTION_C->weapon[3] = Me_MOTION_C->weapon[0];
//         pHVar4->weapon[0] = ppOVar3[2];
//         pHVar4->weapon[2] = (OrnamentType *)0x0;
//         goto LAB_80022690;
//       }
//     }
//     if (((((Me_MOTION_C->pad).trig & 0x80) != 0) && (((int)(short)dtPAD & 0xa000U) != 0)) &&
//        (sVar8 = AttackContinuousCheck(battle), sVar8 != 0)) {
//       motID = 0x70b;
//       if ((dtPAD & 0x2000) == 0) {
//         motID = 0x70a;
//       }
//       iVar13 = 0;
//       if (MotionUpdateMode != 0) {
//         iVar15 = 0;
//         do {
//           iVar13 = iVar13 + 1;
//           if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar15 >> 0xd)) == Me_MOTION_C)
//           goto LAB_800226f8;
//           iVar15 = iVar13 * 0x10000;
//         } while (iVar13 * 0x10000 >> 0x10 < 5);
//       }
//       goto LAB_80022760;
//     }
//     break;
//   case 0xf:
//     if ((dtM->count == 1) && (3000 < (Me_MOTION_C->map).height)) {
//       SetCameraMode(CMODE_FALL);
//     }
//     if (((int)(short)dtPAD & 0xf000U) != 0) {
//       if ((dtPAD & 0x1000) == 0) {
//         sVar8 = -10;
//         if ((dtPAD & 0x4000) == 0) {
//           if ((dtPAD & 0x2000) == 0) {
//             sVar8 = 0;
//             sVar9 = dtR->vy;
//             sVar18 = 10;
//           }
//           else {
//             sVar8 = 0;
//             sVar9 = dtR->vy;
//             sVar18 = -10;
//           }
//         }
//         else {
//           sVar9 = dtR->vy;
//           sVar18 = 0;
//         }
//       }
//       else {
//         sVar8 = 10;
//         sVar9 = dtR->vy;
//         sVar18 = 0;
//       }
//       GetMoveSpeed(local_68,sVar9,sVar8,sVar18);
//       pSVar5 = dtV;
//       local_68[0].vx = local_68[0].vx + dtV->vx;
//       local_68[0].vz = local_68[0].vz + dtV->vz;
//       iVar13 = (int)local_68[0].vx;
//       if (iVar13 < 0) {
//         iVar13 = -iVar13;
//       }
//       if (iVar13 < 0x65) {
//         iVar13 = (int)local_68[0].vz;
//         if (iVar13 < 0) {
//           iVar13 = -iVar13;
//         }
//         if (iVar13 < 0x65) {
//           dtV->vx = local_68[0].vx;
//           pSVar5->vz = local_68[0].vz;
//         }
//       }
//     }
//     if ((Me_MOTION_C->attribute & 0x800U) != 0) {
//       motID = 0x710;
//       DAT_80097f0e = 0;
//       Sound(Me_MOTION_C,0x1a);
//       FUN_80033bc0(dtL,300,0xc,10);
//     }
//     if ((dtM->count == 0) && (dtM->loop == 1)) {
//       dtM->loop = -1;
//     }
//     sVar8 = dtM->loop + -1;
//     if ((dtM->loop < 0) && (dtM->loop = sVar8, sVar8 < -0x1e)) {
//       motID = 0x803;
//       DAT_80097f0e = 0;
//     }
//     if (motID != 0x70f) {
//       sVar8 = Me_MOTION_C->wpatk;
//       if (sVar8 == 2) {
//         DeleteConflict(Me_MOTION_C->model->object[8]);
//         pMVar17 = Me_MOTION_C->model->object[0xb];
// LAB_80022ab8:
//         DeleteConflict(pMVar17);
//       }
//       else {
//         if (2 < sVar8) {
//           if (sVar8 != 3) goto LAB_80022a78;
//           pMVar17 = Me_MOTION_C->model->object[2];
//           goto LAB_80022ab8;
//         }
//         if (sVar8 != 0) {
// LAB_80022a78:
//           DeleteConflict(Me_MOTION_C->model->object[0xd]);
//           pMVar17 = Me_MOTION_C->model->object[0xe];
//           goto LAB_80022ab8;
//         }
//       }
//       if ((AfterimageType *)Me_MOTION_C->illusion[0] != (AfterimageType *)0x0) {
//         DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[0]);
//         Me_MOTION_C->illusion[0] = (undefined *)0x0;
//       }
//       if ((AfterimageType *)Me_MOTION_C->illusion[1] != (AfterimageType *)0x0) {
//         DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[1]);
//         Me_MOTION_C->illusion[1] = (undefined *)0x0;
//       }
//       dtM->mask = 0x7fff;
//       SetCameraMode(CMODE_NORMAL);
//       if (motID == 0x803) {
//         return;
//       }
//     }
//     break;
//   case 0x10:
//     if (dtM->loop < 0) {
//       dtM->loop = 0;
//     }
//     pSVar5 = dtV;
//     if ((dtM->count != 0) || (dtM->loop == 0)) {
//       if (0 < (Me_MOTION_C->map).height) {
//         return;
//       }
//       psVar1 = &dtV->vz;
//       dtV->vx = dtV->vx - (dtV->vx >> 2);
//       pSVar5->vz = *psVar1 - (*psVar1 >> 2);
//       return;
//     }
//     sVar7 = Me_MOTION_C->wpatk;
//     if (sVar7 == 2) {
//       DeleteConflict(Me_MOTION_C->model->object[8]);
//       pMVar17 = Me_MOTION_C->model->object[0xb];
// LAB_80022c74:
//       DeleteConflict(pMVar17);
//     }
//     else {
//       if (2 < sVar7) {
//         if (sVar7 != 3) goto LAB_80022c34;
//         pMVar17 = Me_MOTION_C->model->object[2];
//         goto LAB_80022c74;
//       }
//       if (sVar7 != 0) {
// LAB_80022c34:
//         DeleteConflict(Me_MOTION_C->model->object[0xd]);
//         pMVar17 = Me_MOTION_C->model->object[0xe];
//         goto LAB_80022c74;
//       }
//     }
//     if ((AfterimageType *)Me_MOTION_C->illusion[0] != (AfterimageType *)0x0) {
//       DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[0]);
//       Me_MOTION_C->illusion[0] = (undefined *)0x0;
//     }
//     if ((AfterimageType *)Me_MOTION_C->illusion[1] != (AfterimageType *)0x0) {
//       DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[1]);
//       Me_MOTION_C->illusion[1] = (undefined *)0x0;
//     }
//     motID = 0x501;
//     DAT_80097f0e = 1;
//     goto LAB_80023740;
//   case 0x13:
//     if (dtM->count != 0) {
//       return;
//     }
//     if (dtM->loop == 0) {
//       return;
//     }
//     motID = 0x501;
//     DAT_80097f0e = 1;
//     return;
//   case 0x14:
//   case 0x15:
//   case 0x16:
//   case 0x17:
//   case 0x18:
//   case 0x19:
//     if (dtM->count == 1) {
//       ActionHalt = dtM->count;
//       SetCameraMode(CMODE_CRITICAL_HIT);
//       CamState.OldMode._1_1_ = 1;
//       return;
//     }
//     if ((dtM->loop == 0) && (dtL->vy == (Me_MOTION_C->target->locate).coord.t[1])) {
//       return;
//     }
//     pMVar17 = *Me_MOTION_C->model->object;
//     ActionHalt = 0;
//     iVar13 = (int)(*Me_MOTION_C->model->object)->id;
//     if (-1 < iVar13) {
//       dtL->vx = ConflictObject[iVar13].position.vx;
//       pVVar12->vz = ConflictObject[iVar13].position.vz;
//     }
//     (pMVar17->locate).coord.t[2] = 0;
//     (pMVar17->locate).coord.t[0] = 0;
//     ReturnNormal();
//     sVar8 = DAT_80097f0e;
//     sVar7 = motID;
//     pHVar4 = Me_MOTION_C;
//     if (((Me_MOTION_C->status != 0x11) || (Me_MOTION_C->motion->loop != -1)) &&
//        ((sVar9 = UpdateMotion(Me_MOTION_C->motion,motID), sVar9 != 0 &&
//         (pHVar4->status = (short)(char)((ushort)sVar7 >> 8), sVar8 != 0)))) {
//       pMVar11 = pHVar4->motion->motion;
//       MoveHumanoid(pHVar4,(ushort)pMVar11->orderspd,(ushort)pMVar11->sidespd);
//     }
//     pMVar6 = dtM;
//     dtM->count = 0;
//     pMVar6->loop = 0;
//     PlayMotion(pMVar6,1);
//     CamState.OldMode._1_1_ = 1;
//     DAT_80097f0e = 0xffff;
//     return;
//   }
// switchD_80021f18_caseD_2:
//   sVar9 = motID;
//   sVar8 = dtM->loop;
//   if (dtM->loop < 0) {
//     dtM->loop = sVar8 + 1;
//     if (sVar8 != -1) {
//       return;
//     }
//     pMVar11 = Me_MOTION_C->motion->motion;
//     MoveHumanoid(Me_MOTION_C,(ushort)pMVar11->orderspd,(ushort)pMVar11->sidespd);
//     return;
//   }
//   if ((dtM->count != 0) || (dtM->loop != 1)) {
//     if (battle->mid == 0) {
//       return;
//     }
//     if (BattleDB[sVar7].atks < 1) {
//       return;
//     }
//     ppMVar16 = Me_MOTION_C->model->object;
//     if (Me_MOTION_C->wpatk == 2) {
//       local_70 = ppMVar16[8];
//       local_6c = ppMVar16[0xb];
//     }
//     else if (Me_MOTION_C->wpatk == 3) {
//       local_70 = ppMVar16[2];
//       local_6c = ppMVar16[1];
//     }
//     else {
//       local_70 = ppMVar16[0xd];
//       local_6c = ppMVar16[0xe];
//     }
//     if (dtM->count == BattleDB[sVar7].atks) {
//       iVar13 = (int)Me_MOTION_C->wepid[0];
//       if (-1 < iVar13) {
//         sVar10 = InsertConflict(local_70);
//         sVar8 = WeaponDB[iVar13].confp.vy;
//         sVar9 = WeaponDB[iVar13].confp.vz;
//         sVar18 = WeaponDB[iVar13].confp.pad;
//         ConflictObject[sVar10].offset.vx = WeaponDB[iVar13].confp.vx;
//         ConflictObject[sVar10].offset.vy = sVar8;
//         ConflictObject[sVar10].offset.vz = sVar9;
//         pHVar4 = Me_MOTION_C;
//         ConflictObject[sVar10].offset.pad = sVar18;
//         sVar8 = WeaponDB[iVar13].confp.pad;
//         ConflictObject[sVar10].size.pad = 1;
//         ConflictObject[sVar10].size.vz = sVar8;
//         ConflictObject[sVar10].size.vy = sVar8;
//         ConflictObject[sVar10].size.vx = sVar8;
//         ConflictObject[sVar10].common = (undefined *)pHVar4;
//       }
//       iVar13 = (int)Me_MOTION_C->wepid[1];
//       if (-1 < iVar13) {
//         sVar10 = InsertConflict(local_6c);
//         sVar8 = WeaponDB[iVar13].confp.vy;
//         sVar9 = WeaponDB[iVar13].confp.vz;
//         sVar18 = WeaponDB[iVar13].confp.pad;
//         ConflictObject[sVar10].offset.vx = WeaponDB[iVar13].confp.vx;
//         ConflictObject[sVar10].offset.vy = sVar8;
//         ConflictObject[sVar10].offset.vz = sVar9;
//         pHVar4 = Me_MOTION_C;
//         ConflictObject[sVar10].offset.pad = sVar18;
//         sVar8 = WeaponDB[iVar13].confp.pad;
//         ConflictObject[sVar10].size.pad = 1;
//         ConflictObject[sVar10].size.vz = sVar8;
//         ConflictObject[sVar10].size.vy = sVar8;
//         ConflictObject[sVar10].size.vx = sVar8;
//         ConflictObject[sVar10].common = (undefined *)pHVar4;
//       }
//       Sound(Me_MOTION_C,2);
//       if (Me_MOTION_C == StagePlayer) {
//         PadShockAR(0,0xff,5,0);
//       }
//     }
//     else if (dtM->count == BattleDB[sVar7].atke) {
//       sVar8 = Me_MOTION_C->wpatk;
//       if (sVar8 == 2) {
//         DeleteConflict(Me_MOTION_C->model->object[8]);
//         DeleteConflict(Me_MOTION_C->model->object[0xb]);
//       }
//       else if (sVar8 < 3) {
//         if (sVar8 != 0) {
// LAB_800234c8:
//           DeleteConflict(Me_MOTION_C->model->object[0xd]);
//           DeleteConflict(Me_MOTION_C->model->object[0xe]);
//         }
//       }
//       else {
//         if (sVar8 != 3) goto LAB_800234c8;
//         DeleteConflict(Me_MOTION_C->model->object[2]);
//       }
//       dtM->mask = 0x7fff;
//     }
//     if ((dtM->count < BattleDB[sVar7].atke) && ((Me_MOTION_C->type & 0xf0U) != 0xa0)) {
//       if (local_70->id != -1) {
//         WeaponHitWeapon(local_70);
//       }
//       if (local_6c->id != -1) {
//         WeaponHitWeapon(local_6c);
//       }
//     }
//     if (BattleDB[sVar7].ilus < 1) {
//       return;
//     }
//     if (dtM->count == BattleDB[sVar7].ilus) {
//       iVar13 = (int)Me_MOTION_C->wepid[0];
//       if (-1 < iVar13) {
//         pAVar14 = SetupAfterimage(local_70,10);
//         pHVar4 = Me_MOTION_C;
//         sVar7 = WeaponDB[iVar13].ilup0.vy;
//         sVar8 = WeaponDB[iVar13].ilup0.vz;
//         sVar9 = WeaponDB[iVar13].ilup0.pad;
//         (pAVar14->vector1).vx = WeaponDB[iVar13].ilup0.vx;
//         (pAVar14->vector1).vy = sVar7;
//         (pAVar14->vector1).vz = sVar8;
//         (pAVar14->vector1).pad = sVar9;
//         sVar7 = WeaponDB[iVar13].ilup1.vy;
//         sVar8 = WeaponDB[iVar13].ilup1.vz;
//         sVar9 = WeaponDB[iVar13].ilup1.pad;
//         (pAVar14->vector2).vx = WeaponDB[iVar13].ilup1.vx;
//         (pAVar14->vector2).vy = sVar7;
//         (pAVar14->vector2).vz = sVar8;
//         (pAVar14->vector2).pad = sVar9;
//         pHVar4->illusion[0] = (undefined *)pAVar14;
//       }
//       iVar13 = (int)Me_MOTION_C->wepid[1];
//       if (iVar13 < 0) {
//         return;
//       }
//       pAVar14 = SetupAfterimage(local_6c,10);
//       pHVar4 = Me_MOTION_C;
//       sVar7 = WeaponDB[iVar13].ilup0.vy;
//       sVar8 = WeaponDB[iVar13].ilup0.vz;
//       sVar9 = WeaponDB[iVar13].ilup0.pad;
//       (pAVar14->vector1).vx = WeaponDB[iVar13].ilup0.vx;
//       (pAVar14->vector1).vy = sVar7;
//       (pAVar14->vector1).vz = sVar8;
//       (pAVar14->vector1).pad = sVar9;
//       sVar7 = WeaponDB[iVar13].ilup1.vy;
//       sVar8 = WeaponDB[iVar13].ilup1.vz;
//       sVar9 = WeaponDB[iVar13].ilup1.pad;
//       (pAVar14->vector2).vx = WeaponDB[iVar13].ilup1.vx;
//       (pAVar14->vector2).vy = sVar7;
//       (pAVar14->vector2).vz = sVar8;
//       (pAVar14->vector2).pad = sVar9;
//       pHVar4->illusion[1] = (undefined *)pAVar14;
//       return;
//     }
//     if (dtM->count != BattleDB[sVar7].ilue) {
//       return;
//     }
//     if ((AfterimageType *)Me_MOTION_C->illusion[0] != (AfterimageType *)0x0) {
//       DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[0]);
//       Me_MOTION_C->illusion[0] = (undefined *)0x0;
//     }
//     if ((AfterimageType *)Me_MOTION_C->illusion[1] != (AfterimageType *)0x0) {
//       DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[1]);
//       Me_MOTION_C->illusion[1] = (undefined *)0x0;
//     }
// LAB_80023740:
//     dtM->mask = 0x7fff;
//     return;
//   }
//   sVar7 = Me_MOTION_C->wpatk;
//   if (sVar7 == 2) {
//     DeleteConflict(Me_MOTION_C->model->object[8]);
//     pMVar17 = Me_MOTION_C->model->object[0xb];
//   }
//   else {
//     if (sVar7 < 3) {
//       if (sVar7 == 0) goto LAB_80023074;
//     }
//     else if (sVar7 == 3) {
//       pMVar17 = Me_MOTION_C->model->object[2];
//       goto LAB_80023068;
//     }
//     DeleteConflict(Me_MOTION_C->model->object[0xd]);
//     pMVar17 = Me_MOTION_C->model->object[0xe];
//   }
// LAB_80023068:
//   DeleteConflict(pMVar17);
// LAB_80023074:
//   if ((AfterimageType *)Me_MOTION_C->illusion[0] != (AfterimageType *)0x0) {
//     DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[0]);
//     Me_MOTION_C->illusion[0] = (undefined *)0x0;
//   }
//   if ((AfterimageType *)Me_MOTION_C->illusion[1] != (AfterimageType *)0x0) {
//     DisposeAfterimage((AfterimageType *)Me_MOTION_C->illusion[1]);
//     Me_MOTION_C->illusion[1] = (undefined *)0x0;
//   }
//   motID = 0x501;
//   DAT_80097f0e = 1;
//   bVar2 = MotionUpdateMode != 0;
//   dtM->mask = 0x7fff;
//   if (bVar2) {
//     iVar15 = 0;
//     iVar13 = 0;
//     do {
//       iVar15 = iVar15 + 1;
//       if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar13 >> 0xd)) == Me_MOTION_C)
//       goto LAB_8002315c;
//       iVar13 = iVar15 * 0x10000;
//     } while (iVar15 * 0x10000 >> 0x10 < 5);
//   }
//   SetNowMotion(Me_MOTION_C,motID,DAT_80097f0e);
//   DAT_80097f0e = -1;
// LAB_8002315c:
//   pMVar6 = dtM;
//   pHVar4 = Me_MOTION_C;
//   dtR->vy = dtR->vy + (((*Me_MOTION_C->model->object)->rotate).vy - dtM->motion->rotate[0]->y);
//   bVar2 = pHVar4 == StagePlayer;
//   ((*pHVar4->model->object)->rotate).vy = pMVar6->motion->rotate[0]->y;
//   if ((bVar2) && (SetCameraMode(CMODE_NORMAL), sVar9 == 0x712)) {
//     CamState.OldMode._1_1_ = 1;
//   }
//   (Me_MOTION_C->pad).time = 0;
//   return;
// LAB_800226f8:
//   DAT_80097f0e = 1;
//   sVar8 = 0;
// LAB_80022780:
//   pVVar12 = dtL;
//   if ((sVar8 != 0) && (iVar13 = (int)(*Me_MOTION_C->model->object)->id, -1 < iVar13)) {
//     dtL->vx = ConflictObject[iVar13].position.vx;
//     pVVar12->vz = ConflictObject[iVar13].position.vz;
//   }
//   goto switchD_80021f18_caseD_2;
// }

#endif /* NON_MATCHING */
