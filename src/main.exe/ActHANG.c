#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActHANG(void);
 *     MOTION.C:1663, 45 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     reg   $s0       long y
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR *dtV;
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern struct VECTOR *dtL;
 *     extern short motID;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

/*
 * ActHANG (0x80024748) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=ActHANG
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActHANG", ActHANG);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActHANG", switchD_80024794__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActHANG", switchD_80024794__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActHANG", switchD_80024794__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActHANG", switchD_80024794__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActHANG", switchD_80024794__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActHANG", switchD_80024794__caseD_5);

/* jump-table pool @ 0x80011550 (5 words; tables at 0x80011550) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 ActHANG_jtbl[5] = {
    0x8002479C, 0x80024990, 0x80024848, 0x80024848,
    0x800248B8,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// void ActHANG(void)
// 
// {
//   MotionManager *pMVar1;
//   MotionManager *pMVar2;
//   short sVar3;
//   long lVar4;
//   int iVar5;
//   
//   pMVar1 = dtM;
//   dtV->vy = 0;
//   pMVar2 = dtM;
//   switch((int)(((ushort)pMVar1->mid - 0xa00) * 0x10000) >> 0x10) {
//   case 0:
//     if ((dtPAD & 0x4000) != 0) {
//       iVar5 = dtL->vy;
//       do {
//         iVar5 = iVar5 + 100;
//         dtL->vy = iVar5;
//         sVar3 = HangCheck();
//       } while (sVar3 != 0);
//       motID = 0x803;
//       DAT_80097f0e = 0;
//       break;
//     }
//     sVar3 = 0xa02;
//     if (((dtPAD & 0x2000) == 0) && (sVar3 = 0xa03, (dtPAD & 0x8000) == 0)) {
//       if (((dtPAD & 0x1000) != 0) &&
//          (lVar4 = GetAreaMapLevel(GlobalAreaMap,dtL->vx,dtL->vy + -2000), lVar4 != -0x80000000)) {
//         motID = 0xa04;
//         DAT_80097f0e = 1;
//       }
//       break;
//     }
//     goto LAB_800249b8;
//   case 1:
//     if ((dtM->count != 0) || (sVar3 = 0xa00, dtM->loop == 0)) break;
// LAB_800249b8:
//     DAT_80097f0e = 1;
//     motID = sVar3;
//     break;
//   case 2:
//   case 3:
//     if (((int)(short)dtPAD & 0xa000U) == 0) {
//       motID = 0xa00;
//       DAT_80097f0e = 1;
//     }
//     else {
//       sVar3 = HangCheck();
//       if (sVar3 == 0) {
//         motID = 0x803;
//         DAT_80097f0e = 0;
//       }
//     }
//     if (dtM->count == 1) {
//       Sound(Me_MOTION_C,0x1b);
//     }
//     break;
//   case 4:
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
//     if ((-1 < dtM->count) && (dtV->vy = -0x23, 0x28 < pMVar2->count)) {
//       MoveHumanoid(Me_MOTION_C,100,0);
//     }
//   }
//   if (((int)Me_MOTION_C->attribute & 0x8000U) != 0) {
//     MoveHumanoid(Me_MOTION_C,-10,0);
//     motID = 0x803;
//     DAT_80097f0e = 0;
//   }
//   return;
// }

#endif /* NON_MATCHING */
