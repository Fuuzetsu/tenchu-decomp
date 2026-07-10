#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSTATE(void);
 *     MOTION.C:1507, 79 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern struct Humanoid *StagePlayer;
 *     extern short motID;
 *     extern struct SVECTOR *dtV;
 *     extern short dtPAD;
 *     extern struct SVECTOR *dtR;
 *     extern struct VECTOR *dtL;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 * END PSX.SYM */

/*
 * ActSTATE (0x8002375c) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=ActSTATE
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSTATE", ActSTATE);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSTATE", switchD_8002379c__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSTATE", switchD_8002379c__caseD_d);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSTATE", switchD_8002379c__caseD_e);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSTATE", switchD_8002379c__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSTATE", switchD_8002379c__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSTATE", switchD_8002379c__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSTATE", switchD_8002379c__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSTATE", switchD_8002379c__caseD_f);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSTATE", switchD_8002379c__caseD_1);

/* jump-table pool @ 0x80011510 (16 words; tables at 0x80011510) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 ActSTATE_jtbl[16] = {
    0x8002406C, 0x800241C4, 0x80023C70, 0x80023E6C,
    0x80023E6C, 0x80023F1C, 0x800241C4, 0x800241C4,
    0x800241C4, 0x800241C4, 0x800241C4, 0x800241C4,
    0x800241C4, 0x800237A4, 0x80023A6C, 0x80024148,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// void ActSTATE(void)
// 
// {
//   short *psVar1;
//   Humanoid *pHVar2;
//   Humanoid *pHVar3;
//   SVECTOR *pSVar4;
//   short sVar5;
//   ushort uVar6;
//   long lVar7;
//   int iVar8;
//   uint uVar9;
//   int iVar10;
//   ModelType *pMVar11;
//   
//   pHVar3 = Me_MOTION_C;
//   pHVar2 = StagePlayer;
//   switch((int)(((ushort)dtM->mid - 0x801) * 0x10000) >> 0x10) {
//   case 0:
//     iVar10 = (uint)(ushort)dtM->motion->time << 0x10;
//     if ((int)dtM->count != (iVar10 >> 0x10) - (iVar10 >> 0x1f) >> 1) {
//       if (dtM->count != 0) {
//         return;
//       }
//       if (dtM->loop == 0) {
//         return;
//       }
//     }
//     motID = 0x600;
//     DAT_80097f0e = 1;
//     iVar10 = 0;
//     if (MotionUpdateMode != 0) {
//       iVar8 = 0;
//       do {
//         iVar10 = iVar10 + 1;
//         if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar8 >> 0xd)) == Me_MOTION_C)
//         goto LAB_80024134;
//         iVar8 = iVar10 * 0x10000;
//       } while (iVar10 * 0x10000 >> 0x10 < 5);
//     }
//     SetNowMotion(Me_MOTION_C,0x600,1);
//     DAT_80097f0e = 0xffff;
// LAB_80024134:
//     Sound(Me_MOTION_C,0x13);
//     return;
//   default:
//     goto switchD_8002379c_caseD_1;
//   case 2:
//     if ((dtM->count < -0x36) && (200 < dtV->vy)) {
//       dtM->count = -0x1e;
//     }
//     pSVar4 = dtV;
//     if ((0 < dtV->vy) && (((Me_MOTION_C->pad).trig & 0x80) != 0)) {
//       motID = 0x70f;
//       DAT_80097f0e = 0;
//     }
//     if (((Me_MOTION_C->attribute & 0x800U) == 0) && (0 < (Me_MOTION_C->map).height)) {
//       if (-0x2e < dtM->count) {
//         return;
//       }
//       psVar1 = &dtV->vz;
//       dtV->vx = dtV->vx >> 1;
//       pSVar4->vz = *psVar1 >> 1;
//       return;
//     }
//     if (dtM->count < -0x28) {
//       if (Me_MOTION_C == StagePlayer) {
//         SetCameraMode(CMODE_NORMAL);
//       }
//       if ((Me_MOTION_C->attribute & 0x40U) == 0) {
//         motID = 0;
//       }
//       else {
//         motID = 0x501;
//       }
//       DAT_80097f0e = 1;
//       Sound(Me_MOTION_C,0x19);
//       return;
//     }
//     sVar5 = 0x804;
//     if ((-0x15 < dtM->count) && (sVar5 = 0x805, (Me_MOTION_C->type & 0xf0U) == 0x10)) {
//       DAT_80097f0e = 0;
//       uVar9 = rand();
//       pHVar2 = Me_MOTION_C;
//       motID = 0x1008;
//       if ((uVar9 & 1) != 0) {
//         motID = 0x1007;
//       }
//       uVar6 = Me_MOTION_C->life - 10;
//       Me_MOTION_C->life = uVar6;
//       if ((int)((uint)uVar6 << 0x10) < 0) {
//         pHVar2->life = 0;
//       }
//       Sound(Me_MOTION_C,8);
//       ReqLifeBar(Me_MOTION_C);
//       return;
//     }
//     motID = sVar5;
//     DAT_80097f0e = 0;
//     return;
//   case 3:
//   case 4:
//     if ((dtM->count == 1) && (Me_MOTION_C == StagePlayer)) {
//       PadShockAR(0,0xff,10,0);
//       SetCameraMode(CMODE_NORMAL);
//     }
//     if (((dtM->count < 5) && ((dtPAD & 0x20) != 0)) && (((Me_MOTION_C->pad).trig & 0x40) != 0)) {
//       motID = 0xb09;
//       DAT_80097f0e = 1;
//       dtR->vy = dtR->vy + 0x800;
//     }
//   case 5:
//     if (dtM->count == 1) {
//       sVar5 = 0x1a;
//       if (motID == 0x804) {
//         sVar5 = 0x19;
//       }
//       Sound(Me_MOTION_C,sVar5);
//       FUN_80033bc0(dtL,300,0xc,10);
//       if (StagePlayer == Me_MOTION_C) {
//         if (motID == 0x805) {
//           iVar10 = 0;
//           iVar8 = 0x1e;
//         }
//         else {
//           iVar10 = 5;
//           iVar8 = 0;
//         }
//         PadShockAR(0,0xff,iVar10,iVar8);
//       }
//     }
//     pSVar4 = dtV;
//     if ((dtM->count != 0) || (dtM->loop == 0)) {
//       psVar1 = &dtV->vz;
//       dtV->vx = dtV->vx - (dtV->vx >> 2);
//       pSVar4->vz = *psVar1 - (*psVar1 >> 2);
//       return;
//     }
//     if (motID == 0x806) {
//       CamState.OldMode._1_1_ = 1;
//     }
//     if (Me_MOTION_C == StagePlayer) {
//       SetCameraMode(CMODE_NORMAL);
//     }
//     if ((Me_MOTION_C->attribute & 0x40U) == 0) {
// LAB_800241bc:
//       motID = 0;
//     }
//     else {
//       motID = 0x501;
//     }
//     break;
//   case 0xd:
//     iVar10 = (int)dtM->count;
//     if (iVar10 != 1) {
//       iVar8 = (uint)(ushort)dtM->motion->time << 0x10;
//       if (iVar10 == (iVar8 >> 0x10) - (iVar8 >> 0x1f) >> 1) {
//         Sound(Me_MOTION_C,0);
//         EquipWeapon(Me_MOTION_C,1);
//         return;
//       }
//       if (iVar10 != 0) {
//         return;
//       }
//       if (dtM->loop == 0) {
//         return;
//       }
//       if ((Me_MOTION_C->attribute & 3U) == 0) {
//         Me_MOTION_C->attribute = Me_MOTION_C->attribute | 0x12;
//         pHVar3->chase[0] = pHVar2->locate->vx;
//         lVar7 = pHVar2->locate->vz;
//         pHVar3->actscnt = '\x01';
//         pHVar3->chase[1] = lVar7;
//       }
//       motID = 0x501;
//       DAT_80097f0e = 1;
//       return;
//     }
//     sVar5 = Me_MOTION_C->wpatk;
//     if (sVar5 == 2) {
//       DeleteConflict(Me_MOTION_C->model->object[8]);
//       pMVar11 = Me_MOTION_C->model->object[0xb];
// LAB_80023898:
//       DeleteConflict(pMVar11);
//     }
//     else {
//       if (2 < sVar5) {
//         if (sVar5 != 3) goto LAB_80023858;
//         pMVar11 = Me_MOTION_C->model->object[2];
//         goto LAB_80023898;
//       }
//       if (sVar5 != 0) {
// LAB_80023858:
//         DeleteConflict(Me_MOTION_C->model->object[0xd]);
//         pMVar11 = Me_MOTION_C->model->object[0xe];
//         goto LAB_80023898;
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
//     pHVar2 = Me_MOTION_C;
//     dtM->mask = 0x7fff;
//     sVar5 = pHVar2->type;
//     if ((sVar5 < 7) && (3 < sVar5)) {
//       if (pHVar2 == StagePlayer) {
//         SetCameraMode(CMODE_NORMAL);
//       }
//       if ((Me_MOTION_C->attribute & 0x40U) == 0) goto LAB_800241bc;
//       motID = 0x501;
//     }
//     else {
//       if ((Me_MOTION_C->attribute & 0x40U) == 0) {
//         return;
//       }
//       motID = 0x501;
//     }
//     break;
//   case 0xe:
//     iVar10 = (int)dtM->count;
//     if (iVar10 != 1) {
//       iVar8 = (uint)(ushort)dtM->motion->time << 0x10;
//       if (iVar10 == (iVar8 >> 0x10) - (iVar8 >> 0x1f) >> 1) {
//         Sound(Me_MOTION_C,1);
//         EquipWeapon(Me_MOTION_C,0);
//         return;
//       }
//       if (iVar10 != 0) {
//         return;
//       }
//       if (dtM->loop == 0) {
//         return;
//       }
//       motID = 0;
//       DAT_80097f0e = 1;
//       return;
//     }
//     sVar5 = Me_MOTION_C->wpatk;
//     if (sVar5 == 2) {
//       DeleteConflict(Me_MOTION_C->model->object[8]);
//       pMVar11 = Me_MOTION_C->model->object[0xb];
// LAB_80023b60:
//       DeleteConflict(pMVar11);
//     }
//     else {
//       if (2 < sVar5) {
//         if (sVar5 != 3) goto LAB_80023b20;
//         pMVar11 = Me_MOTION_C->model->object[2];
//         goto LAB_80023b60;
//       }
//       if (sVar5 != 0) {
// LAB_80023b20:
//         DeleteConflict(Me_MOTION_C->model->object[0xd]);
//         pMVar11 = Me_MOTION_C->model->object[0xe];
//         goto LAB_80023b60;
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
//     pHVar2 = Me_MOTION_C;
//     dtM->mask = 0x7fff;
//     if ((pHVar2->attribute & 0x40U) != 0) {
//       return;
//     }
//     goto LAB_800241bc;
//   case 0xf:
//     if (dtM->count != 0) {
//       return;
//     }
//     if (dtM->loop == 0) {
//       return;
//     }
//     if (Me_MOTION_C == StagePlayer) {
//       SetCameraMode(CMODE_NORMAL);
//     }
//     if ((Me_MOTION_C->attribute & 0x40U) == 0) goto LAB_800241bc;
//     motID = 0x501;
//   }
//   DAT_80097f0e = 1;
// switchD_8002379c_caseD_1:
//   return;
// }

#endif /* NON_MATCHING */
