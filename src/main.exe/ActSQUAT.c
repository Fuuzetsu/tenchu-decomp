#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSQUAT(void);
 *     MOTION.C:1712, 83 src lines, frame 40 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $s0       short turn
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern struct SVECTOR *dtR;
 *     extern short motID;
 *     extern struct SVECTOR *dtV;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct VECTOR *dtL;
 *     extern short dtCMD;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

/*
 * ActSQUAT (0x80024a04) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=ActSQUAT
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", ActSQUAT);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", switchD_80024a64__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", switchD_80024a64__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", switchD_80024a64__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", switchD_80024a64__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", switchD_80024a64__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", switchD_80024a64__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", switchD_80024a64__caseD_9);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", switchD_80024a64__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", switchD_8002500c__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", switchD_8002500c__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", switchD_8002500c__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", switchD_8002500c__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", switchD_8002500c__caseD_6);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", switchD_8002500c__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", switchD_8002500c__caseD_7);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", switchD_8002500c__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSQUAT", switchD_8002500c__caseD_4);

/* jump-table pool @ 0x80011568 (22 words; tables at 0x80011568, 0x80011590) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 ActSQUAT_jtbl[22] = {
    0x80024A6C, 0x80024B0C, 0x80024B90, 0x80024C0C,
    0x80024C64, 0x80024DB0, 0x80024DB0, 0x80024DB0,
    0x80024DB0, 0x80024D34, 0x80025044, 0x8002501C,
    0x80025014, 0x80025024, 0x80025060, 0x80025034,
    0x8002502C, 0x8002503C, 0x80025060, 0x80025060,
    0x80025060, 0x80025044,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// void ActSQUAT(void)
// 
// {
//   ushort uVar1;
//   VECTOR *pVVar2;
//   SVECTOR *pSVar3;
//   int iVar4;
//   MotionDataType *pMVar5;
//   long lVar6;
//   short sVar7;
//   
//   iVar4 = (uint)(ushort)Me_MOTION_C->turn << 0x10;
//   sVar7 = (short)((uint)((iVar4 >> 0x10) - (iVar4 >> 0x1f)) >> 1);
//   switch((int)(((ushort)dtM->mid - 0xb00) * 0x10000) >> 0x10) {
//   case 0:
//     if ((dtM->count == 0) && (dtM->loop != 0)) {
//       dtM->loop = -1;
//     }
//     sVar7 = 0xb01;
//     if (((((dtPAD & 0x1000) == 0) && (sVar7 = 0xb02, (dtPAD & 0x4000) == 0)) &&
//         (sVar7 = 0xb03, (dtPAD & 0x2000) == 0)) && (sVar7 = 0xb04, (dtPAD & 0x8000) == 0)) {
//       if (((Me_MOTION_C->pad).trig & 0x40) != 0) {
//         motID = 0xb09;
//         DAT_80097f0e = 1;
//         dtR->vy = dtR->vy + 0x800;
//       }
//       goto LAB_80024ea8;
//     }
//     goto LAB_80024d9c;
//   case 1:
//     if (dtM->count == 1) {
//       Sound(Me_MOTION_C,0x11);
//     }
//     if ((dtPAD & 0x1000) == 0) {
//       motID = 0xb00;
//       DAT_80097f0e = 1;
//     }
//     else if (((Me_MOTION_C->pad).trig & 0x40) != 0) {
//       motID = 0xb09;
//       DAT_80097f0e = 1;
//       dtR->vy = dtR->vy + 0x800;
//     }
//     goto LAB_80024ea8;
//   case 2:
//     if (dtM->count == 1) {
//       Sound(Me_MOTION_C,0x11);
//     }
//     pSVar3 = dtV;
//     if ((dtPAD & 0x4000) == 0) {
// LAB_80024c9c:
//       motID = 0xb00;
//       DAT_80097f0e = 1;
//       goto LAB_80024ea8;
//     }
//     if ((dtPAD & 0xa000) != 0) {
//       if ((dtPAD & 0x2000) == 0) {
//         sVar7 = -sVar7;
//       }
//       dtR->vy = dtR->vy + sVar7;
//       pSVar3->vz = 0;
//       pSVar3->vx = 0;
//       goto LAB_80024ea8;
//     }
//     break;
//   case 3:
//     if (dtM->count == 1) {
//       Sound(Me_MOTION_C,0x11);
//     }
//     if ((dtPAD & 0x2000) == 0) goto LAB_80024c9c;
//     if ((dtPAD & 0x4000) != 0) {
//       sVar7 = dtR->vy + sVar7;
// LAB_80024ccc:
//       pSVar3 = dtV;
//       dtR->vy = sVar7;
//       pSVar3->vz = 0;
//       pSVar3->vx = 0;
//       goto LAB_80024ea8;
//     }
//     break;
//   case 4:
//     if (dtM->count == 1) {
//       Sound(Me_MOTION_C,0x11);
//     }
//     if (((int)(short)dtPAD & 0x8000U) == 0) goto LAB_80024c9c;
//     if ((dtPAD & 0x4000) != 0) {
//       sVar7 = dtR->vy - sVar7;
//       goto LAB_80024ccc;
//     }
//     break;
//   default:
//     if (dtM->count == 1) {
//       Sound(Me_MOTION_C,0x13);
//       return;
//     }
//     if ((dtM->count == 0) && (dtM->loop != 0)) {
//       motID = 0xb00;
//       DAT_80097f0e = 1;
//       return;
//     }
//     if ((dtV->vx == 0) && (dtV->vz == 0)) {
//       return;
//     }
//     lVar6 = GetAreaMapLevel(GlobalAreaMap,dtL->vx + dtV->vx * 4,dtL->vy);
//     pSVar3 = dtV;
//     pVVar2 = dtL;
//     if (-0x1c3 < lVar6) {
//       return;
//     }
//     dtL->vx = dtL->vx - (int)dtV->vx;
//     pVVar2->vz = pVVar2->vz - (int)pSVar3->vz;
//     pSVar3->vz = 0;
//     pSVar3->vx = 0;
//     return;
//   case 9:
//     if ((int)dtM->count == (int)((uint)(ushort)dtM->motion->time << 0x10) >> 0x11) {
//       Sound(Me_MOTION_C,0x13);
//       CamState.OldMode._1_1_ = 1;
//     }
//     if ((dtM->count != 0) || (sVar7 = 0xb00, dtM->loop == 0)) goto LAB_80024ea8;
// LAB_80024d9c:
//     DAT_80097f0e = 1;
//     motID = sVar7;
//     goto LAB_80024ea8;
//   }
//   if ((dtV->vx == 0) && (dtV->vz == 0)) {
//     pMVar5 = Me_MOTION_C->motion->motion;
//     MoveHumanoid(Me_MOTION_C,(ushort)pMVar5->orderspd,(ushort)pMVar5->sidespd);
//   }
// LAB_80024ea8:
//   if (motID == 0xb09) {
//     return;
//   }
//   if ((motID != 0xb00) && ((dtV->vx != 0 || (dtV->vz != 0)))) {
//     iVar4 = GetAreaMapLevel(GlobalAreaMap,dtL->vx + dtV->vx * 0x10,dtL->vy);
//     pSVar3 = dtV;
//     if (iVar4 < 0) {
//       iVar4 = -iVar4;
//     }
//     if (499 < iVar4) {
//       dtV->vz = 0;
//       pSVar3->vx = 0;
//     }
//   }
//   if (dtCMD == 0) {
//     uVar1 = (Me_MOTION_C->pad).trig;
//     if ((uVar1 & 0x80) != 0) {
//       AttackControl();
//       return;
//     }
//     if ((uVar1 & 0x10) == 0) {
//       if ((dtPAD & 0x20) != 0) {
//         if (DAT_80097f1c == 0) {
//           return;
//         }
//         StickonCheck();
//         return;
//       }
//       if (Me_MOTION_C == StagePlayer) {
//         SetCameraMode(CMODE_NORMAL);
//       }
//       sVar7 = 0x501;
//       if ((Me_MOTION_C->attribute & 0x40U) == 0) {
//         motID = 0;
//         DAT_80097f0e = 1;
//         return;
//       }
//     }
//     else {
//       switch((int)((DAT_80097b1e + 1) * 0x10000) >> 0x10) {
//       case 0:
//       case 0xb:
//         SoundEx(Me_MOTION_C->locate,0xc);
//         return;
//       case 1:
//         sVar7 = 0x400;
//         break;
//       case 2:
//         sVar7 = 0xe00;
//         break;
//       case 3:
//         sVar7 = 0xf00;
//         break;
//       default:
//         ReqItemDefault(Me_MOTION_C,(int)(short)DAT_80097b1e);
//         return;
//       case 5:
//         sVar7 = 0xf02;
//         break;
//       case 6:
//         sVar7 = 0xf02;
//         break;
//       case 7:
//         sVar7 = 0xf03;
//       }
//     }
//   }
//   else if (dtCMD == 0x12) {
//     sVar7 = 0xb06;
//   }
//   else if (dtCMD < 0x13) {
//     sVar7 = 0xb05;
//     if (dtCMD != 0x11) {
//       return;
//     }
//   }
//   else if (dtCMD == 0x13) {
//     sVar7 = 0xb08;
//   }
//   else {
//     sVar7 = 0xb07;
//     if (dtCMD != 0x14) {
//       return;
//     }
//   }
//   DAT_80097f0e = 1;
//   motID = sVar7;
//   return;
// }

#endif /* NON_MATCHING */
