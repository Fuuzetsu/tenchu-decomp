#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActDAMAGE(void);
 *     MOTION.C:1989, 53 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $a2       struct OrnamentType ** weapon
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern struct SVECTOR *dtV;
 *     extern short motID;
 *     extern struct VECTOR *dtL;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

/*
 * ActDAMAGE (0x800262b0) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=ActDAMAGE
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActDAMAGE", ActDAMAGE);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActDAMAGE", switchD_800262f8__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActDAMAGE", switchD_800262f8__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActDAMAGE", switchD_800262f8__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActDAMAGE", switchD_800262f8__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActDAMAGE", switchD_800262f8__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActDAMAGE", switchD_800262f8__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActDAMAGE", switchD_800262f8__caseD_8);

/* jump-table pool @ 0x800115f0 (8 words; tables at 0x800115f0) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 ActDAMAGE_jtbl[8] = {
    0x80026300, 0x80026440, 0x800265B0, 0x800265B0,
    0x8002666C, 0x80026758, 0x80026758, 0x80026758,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// void ActDAMAGE(void)
// 
// {
//   short *psVar1;
//   bool bVar2;
//   bool bVar3;
//   short sVar4;
//   OrnamentType **ppOVar5;
//   Humanoid *pHVar6;
//   Humanoid *pHVar7;
//   SVECTOR *pSVar8;
//   MotionManager *mmp;
//   short sVar9;
//   int iVar10;
//   int iVar11;
//   ModelArchiveType *pMVar12;
//   short in_a3;
//   
//   mmp = dtM;
//   pHVar6 = Me_MOTION_C;
//   bVar3 = false;
//   bVar2 = bVar3;
//   switch((int)(((ushort)dtM->mid - 0x1005) * 0x10000) >> 0x10) {
//   case 0:
//     if (dtM->count == 1) {
//       pMVar12 = Me_MOTION_C->model;
//       sVar9 = pMVar12->n + -1;
//       if (0xc < pMVar12->n) {
//         sVar9 = 0xc;
//       }
//       iVar11 = 7;
//       if (6 < sVar9) {
//         do {
//           iVar10 = iVar11 << 0x10;
//           iVar11 = iVar11 + 1;
//           iVar10 = *(int *)((iVar10 >> 0xe) + (int)pMVar12->object);
//           *(ushort *)(iVar10 + 0x5a) = *(ushort *)(iVar10 + 0x5a) & 0xfffe;
//           in_a3 = sVar9;
//         } while (iVar11 * 0x10000 >> 0x10 <= (int)sVar9);
//       }
//       (*pMVar12->object)->attribute = (*pMVar12->object)->attribute & 0xfffe;
//     }
//     else if ((dtM->count == 0) && (dtM->loop != 0)) {
//       dtM->loop = -1;
//     }
//     else {
//       dtV->vy = (dtM->count - dtM->motion->time) * 6;
//     }
//     sVar9 = 0x1007;
//     if ((Me_MOTION_C->attribute & 0x800U) != 0) goto LAB_80026578;
//     iVar11 = (Me_MOTION_C->map).height;
//     sVar9 = 0x1007;
// joined_r0x80026570:
//     if (iVar11 < 0) goto LAB_80026578;
//     break;
//   case 1:
//     if (dtM->count == 1) {
//       pMVar12 = Me_MOTION_C->model;
//       sVar9 = pMVar12->n + -1;
//       if (0xc < pMVar12->n) {
//         sVar9 = 0xc;
//       }
//       iVar11 = 7;
//       if (6 < sVar9) {
//         do {
//           iVar10 = iVar11 << 0x10;
//           iVar11 = iVar11 + 1;
//           iVar10 = *(int *)((iVar10 >> 0xe) + (int)pMVar12->object);
//           *(ushort *)(iVar10 + 0x5a) = *(ushort *)(iVar10 + 0x5a) & 0xfffe;
//           in_a3 = sVar9;
//         } while (iVar11 * 0x10000 >> 0x10 <= (int)sVar9);
//       }
//       (*pMVar12->object)->attribute = (*pMVar12->object)->attribute & 0xfffe;
//     }
//     else if ((dtM->count == 0) && (dtM->loop != 0)) {
//       dtM->loop = -1;
//     }
//     else {
//       dtV->vy = (dtM->count - dtM->motion->time) * 6;
//     }
//     sVar9 = 0x1008;
//     if ((Me_MOTION_C->attribute & 0x800U) == 0) {
//       iVar11 = (Me_MOTION_C->map).height;
//       sVar9 = 0x1008;
//       goto joined_r0x80026570;
//     }
// LAB_80026578:
//     DAT_80097f0e = 0;
//     motID = sVar9;
//     break;
//   case 2:
//   case 3:
//     if (dtM->count == 1) {
//       Sound(Me_MOTION_C,0x1d);
//       FUN_80033bc0(dtL,500,0x1e,0x1e);
//       if (StagePlayer == Me_MOTION_C) {
//         PadShockAR(0,0xff,0,0x1e);
//       }
//     }
//     pSVar8 = dtV;
//     if ((dtM->count != 0) || (sVar9 = 0x1009, dtM->loop == 0)) {
//       psVar1 = &dtV->vz;
//       dtV->vx = dtV->vx - (dtV->vx >> 2);
//       pSVar8->vz = *psVar1 - (*psVar1 >> 2);
//       goto LAB_80026858;
//     }
//     goto LAB_80026744;
//   case 4:
//     if (Me_MOTION_C->life == 0) {
//       dtM->loop = 0;
//       mmp->count = 0;
//       PlayMotion(mmp,1);
//       pHVar7 = Me_MOTION_C;
//       pHVar6 = StagePlayer;
//       dtM->loop = -2;
//       pHVar7->status = 0x11;
//       pSVar8 = dtV;
//       pHVar7->attribute = pHVar7->attribute & 0xffef;
//       pSVar8->vz = 0;
//       pSVar8->vy = 0;
//       pSVar8->vx = 0;
//       if (pHVar7 == pHVar6) {
//         return;
//       }
//       DeleteConflict(*pHVar7->model->object);
//       TurnAroundAllItems(Me_MOTION_C);
//       return;
//     }
//     sVar9 = dtM->loop + -1;
//     dtM->loop = sVar9;
//     bVar2 = false;
//     if ((int)pHVar6->life - (int)pHVar6->lifemax < (int)sVar9) goto LAB_80026858;
//     sVar9 = 0x100c;
// LAB_80026744:
//     DAT_80097f0e = 1;
//     bVar2 = false;
//     motID = sVar9;
//     goto LAB_80026858;
//   case 5:
//   case 6:
//   case 7:
//     bVar2 = false;
//     if ((dtM->count == 0) && (bVar2 = false, dtM->loop != 0)) {
//       bVar2 = true;
//     }
//     goto LAB_80026858;
//   default:
//     sVar9 = dtV->vx;
//     if (sVar9 != 0) {
//       if (sVar9 < 1) {
//         sVar4 = 4;
//       }
//       else {
//         sVar4 = -4;
//       }
//       dtV->vx = sVar9 + sVar4;
//     }
//     sVar9 = dtV->vz;
//     if (sVar9 != 0) {
//       if (sVar9 < 1) {
//         sVar4 = 4;
//       }
//       else {
//         sVar4 = -4;
//       }
//       dtV->vz = sVar9 + sVar4;
//     }
//     pHVar6 = Me_MOTION_C;
//     bVar2 = false;
//     if ((((dtM->count == 0) && (bVar2 = bVar3, dtM->loop != 0)) &&
//         (bVar2 = true, Me_MOTION_C->wpatk == 0x2a)) &&
//        (Me_MOTION_C->weapon[3] != (OrnamentType *)0x0)) {
//       ppOVar5 = Me_MOTION_C->weapon;
//       Me_MOTION_C->weapon[2] = Me_MOTION_C->weapon[0];
//       pHVar6->weapon[0] = ppOVar5[3];
//       pHVar6->weapon[3] = (OrnamentType *)0x0;
//       Sound(pHVar6,1);
//     }
//     goto LAB_80026858;
//   }
//   if ((dtM->count & 4U) != 0) {
//     SetBlood(dtL,(SVECTOR *)&DAT_00000001,0x3c,in_a3);
//     bVar2 = false;
//   }
// LAB_80026858:
//   if (bVar2) {
//     if ((Me_MOTION_C->attribute & 0x40U) == 0) {
//       motID = 0x80e;
//       DAT_80097f0e = 1;
//     }
//     else {
//       motID = 0x501;
//       DAT_80097f0e = 1;
//       Me_MOTION_C->attribute = Me_MOTION_C->attribute & 0xfffcU | 2;
//     }
//   }
//   return;
// }

#endif /* NON_MATCHING */
