#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSWIM(void);
 *     MOTION.C:1030, 57 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *     reg   $a2       struct ModelArchiveType * mdl
 *     reg   $a1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short motID;
 *     extern short dtPAD;
 *     extern struct SVECTOR *dtR;
 *     extern struct SVECTOR *dtV;
 *     extern struct VECTOR *dtL;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

/*
 * ActSWIM (0x80020464) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=ActSWIM
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSWIM", ActSWIM);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSWIM", switchD_800209b4__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSWIM", switchD_800209b4__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSWIM", switchD_800209b4__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSWIM", switchD_800209b4__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSWIM", switchD_800209b4__caseD_6);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSWIM", switchD_800209b4__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSWIM", switchD_800209b4__caseD_7);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSWIM", switchD_800209b4__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSWIM", switchD_800209b4__caseD_4);

/* jump-table pool @ 0x80011358 (12 words; tables at 0x80011358) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 ActSWIM_jtbl[12] = {
    0x800209FC, 0x800209C4, 0x800209BC, 0x800209CC,
    0x80020A18, 0x800209DC, 0x800209D4, 0x800209E4,
    0x80020A18, 0x80020A18, 0x80020A18, 0x800209FC,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// void ActSWIM(void)
// 
// {
//   Humanoid *pHVar1;
//   VECTOR *pVVar2;
//   SVECTOR *pSVar3;
//   short sVar4;
//   int iVar5;
//   int iVar6;
//   ModelArchiveType *pMVar7;
//   
//   sVar4 = dtM->mid;
//   if (sVar4 == 0x301) {
//     if (dtM->count == 1) {
//       pMVar7 = Me_MOTION_C->model;
//       sVar4 = pMVar7->n + -1;
//       if (0xc < pMVar7->n) {
//         sVar4 = 0xc;
//       }
//       iVar6 = 7;
//       if (6 < sVar4) {
//         do {
//           iVar5 = iVar6 << 0x10;
//           iVar6 = iVar6 + 1;
//           iVar5 = *(int *)((iVar5 >> 0xe) + (int)pMVar7->object);
//           *(ushort *)(iVar5 + 0x5a) = *(ushort *)(iVar5 + 0x5a) & 0xfffe;
//         } while (iVar6 * 0x10000 >> 0x10 <= (int)sVar4);
//       }
//       (*pMVar7->object)->attribute = (*pMVar7->object)->attribute & 0xfffe;
//       Sound(Me_MOTION_C,0x15);
//       return;
//     }
//     if ((dtM->count == 0) && (dtM->loop != 0)) {
//       if (Me_MOTION_C == StagePlayer) {
//         SetCameraMode(CMODE_NORMAL);
//       }
//       if ((Me_MOTION_C->attribute & 0x40U) != 0) {
//         motID = 0x501;
//         DAT_80097f0e = 1;
//         return;
//       }
//       motID = 0;
//       DAT_80097f0e = 1;
//       return;
//     }
//     if (dtM->count < 0x29) {
//       return;
//     }
//     MoveHumanoid(Me_MOTION_C,100,0);
//     return;
//   }
//   if (sVar4 < 0x302) {
//     if (sVar4 != 0x300) goto code_r0x800208b0;
//     sVar4 = SwimCheck();
//     if (sVar4 == 0) {
//       motID = 0x301;
//       DAT_80097f0e = 1;
//       goto code_r0x800208b0;
//     }
//     if (((int)(short)dtPAD & 0xa000U) != 0) {
//       if (dtM->count == 1) {
//         Sound(Me_MOTION_C,0x15);
//       }
//       if ((dtPAD & 0x2000) == 0) {
//         sVar4 = -Me_MOTION_C->turn;
//       }
//       else {
//         sVar4 = Me_MOTION_C->turn;
//       }
//       dtR->vy = dtR->vy + sVar4;
//       goto code_r0x800208b0;
//     }
//     if ((dtPAD & 0x5000) == 0) goto code_r0x800208b0;
//     motID = 0x302;
//     DAT_80097f0e = 0;
//     sVar4 = 0x3c;
//     if (((dtPAD & 0x1000) == 0) && (sVar4 = -0x3c, (Me_MOTION_C->map).angleH != '\0'))
//     goto code_r0x800208b0;
//   }
//   else {
//     if (sVar4 != 0x302) goto code_r0x800208b0;
//     if (dtM->count == 1) {
//       Sound(Me_MOTION_C,0x15);
//     }
//     if ((dtPAD & 0x1000) == 0) {
//       if ((dtPAD & 0x4000) == 0) {
//         motID = 0x300;
//         DAT_80097f0e = 1;
//         goto code_r0x800208b0;
//       }
//       if (((Me_MOTION_C->map).angleH != '\0') || (sVar4 = SwimCheck(), sVar4 == 0)) {
//         pSVar3 = dtV;
//         pVVar2 = dtL;
//         dtL->vx = dtL->vx - (int)dtV->vx;
//         pVVar2->vz = pVVar2->vz - (int)pSVar3->vz;
//         pSVar3->vz = 0;
//         pSVar3->vx = 0;
//         goto code_r0x800208b0;
//       }
//       if (((int)(short)dtPAD & 0xa000U) != 0) {
//         if ((dtPAD & 0x2000) == 0) {
//           sVar4 = Me_MOTION_C->turn;
//         }
//         else {
//           sVar4 = -Me_MOTION_C->turn;
//         }
//         dtR->vy = dtR->vy + sVar4;
//       }
//       sVar4 = -0x3c;
//     }
//     else {
//       sVar4 = SwimCheck();
//       if (sVar4 == 0) {
//         motID = 0x301;
//         DAT_80097f0e = 1;
//         goto code_r0x800208b0;
//       }
//       if (((int)(short)dtPAD & 0xa000U) != 0) {
//         if ((dtPAD & 0x2000) == 0) {
//           sVar4 = -Me_MOTION_C->turn;
//         }
//         else {
//           sVar4 = Me_MOTION_C->turn;
//         }
//         dtR->vy = dtR->vy + sVar4;
//       }
//       sVar4 = 0x3c;
//     }
//   }
//   MoveHumanoid(Me_MOTION_C,sVar4,0);
// code_r0x800208b0:
//   pHVar1 = Me_MOTION_C;
//   if (((Me_MOTION_C->pad).trig & 0x10) == 0) {
//     return;
//   }
//   if (DAT_80097b1e == 0) {
//     dtM->mask = -2;
//     pMVar7 = pHVar1->model;
//     sVar4 = pMVar7->n + -1;
//     if (0xc < pMVar7->n) {
//       sVar4 = 0xc;
//     }
//     iVar6 = 7;
//     if (6 < sVar4) {
//       do {
//         iVar5 = iVar6 << 0x10;
//         iVar6 = iVar6 + 1;
//         iVar5 = *(int *)((iVar5 >> 0xe) + (int)pMVar7->object);
//         *(ushort *)(iVar5 + 0x5a) = *(ushort *)(iVar5 + 0x5a) & 0xfffe;
//       } while (iVar6 * 0x10000 >> 0x10 <= (int)sVar4);
//     }
//     (*pMVar7->object)->attribute = (*pMVar7->object)->attribute & 0xfffe;
//     switch((int)((DAT_80097b1e + 1) * 0x10000) >> 0x10) {
//     case 0:
//     case 0xb:
//       SoundEx(Me_MOTION_C->locate,0xc);
//       return;
//     case 1:
//       motID = 0x400;
//       break;
//     case 2:
//       motID = 0xe00;
//       break;
//     case 3:
//       motID = 0xf00;
//       break;
//     default:
//       ReqItemDefault(Me_MOTION_C,(int)(short)DAT_80097b1e);
//       return;
//     case 5:
//       motID = 0xf02;
//       break;
//     case 6:
//       motID = 0xf02;
//       break;
//     case 7:
//       motID = 0xf03;
//     }
//     DAT_80097f0e = 1;
//     return;
//   }
//   return;
// }

#endif /* NON_MATCHING */
