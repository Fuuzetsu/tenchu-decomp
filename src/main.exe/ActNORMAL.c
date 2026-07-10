#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActNORMAL(void);
 *     MOTION.C:916, 43 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern struct Humanoid *StagePlayer;
 *     extern short motID;
 *     extern struct SVECTOR *dtR;
 *     extern short dtCMD;
 * END PSX.SYM */

/*
 * ActNORMAL (0x8001f7e4) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=ActNORMAL
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActNORMAL", ActNORMAL);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActNORMAL", switchD_8001fad4__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActNORMAL", switchD_8001fad4__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActNORMAL", switchD_8001fad4__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActNORMAL", switchD_8001fad4__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActNORMAL", switchD_8001fad4__caseD_6);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActNORMAL", switchD_8001fad4__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActNORMAL", switchD_8001fad4__caseD_7);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActNORMAL", switchD_8001fad4__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActNORMAL", switchD_8001fad4__caseD_4);

/* jump-table pool @ 0x800112d8 (12 words; tables at 0x800112d8) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 ActNORMAL_jtbl[12] = {
    0x8001FB0C, 0x8001FAE4, 0x8001FADC, 0x8001FAEC,
    0x8001FB28, 0x8001FAFC, 0x8001FAF4, 0x8001FB04,
    0x8001FB28, 0x8001FB28, 0x8001FB28, 0x8001FB0C,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// void ActNORMAL(void)
// 
// {
//   ushort uVar1;
//   short sVar2;
//   int iVar3;
//   uint uVar4;
//   
//   sVar2 = dtM->mid;
//   if (sVar2 == 1) {
//     if (dtM->count == 1) {
//       Sound(Me_MOTION_C,0x10);
//     }
//     if ((dtPAD & 0x2000) != 0) {
//       sVar2 = dtR->vy + Me_MOTION_C->turn;
// LAB_8001f9e4:
//       dtR->vy = sVar2;
//       goto LAB_8001f9e8;
//     }
//   }
//   else {
//     if (sVar2 < 2) {
//       if (sVar2 == 0) {
//         if (((dtPAD & 1) != 0) && (StagePlayer != Me_MOTION_C)) {
//           sVar2 = 0x102;
//           if (((dtPAD & 0x4000) == 0) &&
//              (((sVar2 = 0x101, (dtPAD & 0x2000) == 0 && (sVar2 = 0x106, (dtPAD & 0x8000) == 0)) &&
//               (sVar2 = 0x100, (dtPAD & 0x1000) == 0)))) {
//             return;
//           }
//           motID = sVar2;
//           DAT_80097f0e = 1;
//           return;
//         }
//         sVar2 = 1;
//         if (((dtPAD & 0x2000) == 0) && (sVar2 = 2, (dtPAD & 0x8000) == 0)) {
//           if ((dtM->count == 0) && (iVar3 = rand(), iVar3 == (iVar3 / 100) * 100)) {
//             DAT_80097f0e = 1;
//             uVar4 = rand();
//             motID = 0x105;
//             if ((uVar4 & 1) != 0) {
//               motID = 0x104;
//             }
//           }
//         }
//         else {
//           DAT_80097f0e = 0;
//           motID = sVar2;
//         }
//       }
//       goto LAB_8001f9e8;
//     }
//     if (sVar2 != 2) goto LAB_8001f9e8;
//     if (dtM->count == 1) {
//       Sound(Me_MOTION_C,0x10);
//     }
//     if (((int)(short)dtPAD & 0x8000U) != 0) {
//       sVar2 = dtR->vy - Me_MOTION_C->turn;
//       goto LAB_8001f9e4;
//     }
//   }
//   motID = 0;
//   DAT_80097f0e = 1;
// LAB_8001f9e8:
//   sVar2 = 0x501;
//   if ((Me_MOTION_C->attribute & 0x40U) == 0) {
//     if (dtCMD == 0) {
//       uVar1 = (Me_MOTION_C->pad).trig;
//       if ((uVar1 & 0x40) != 0) {
//         JumpControl();
//         return;
//       }
//       if ((uVar1 & 0x10) == 0) {
//         sVar2 = 0xb00;
//         if (((((dtPAD & 0x20) == 0) && (sVar2 = 0x200, (dtPAD & 0x1000) == 0)) &&
//             (sVar2 = 0x201, (dtPAD & 0x4000) == 0)) && (sVar2 = 0x80e, (uVar1 & 0x80) == 0)) {
//           return;
//         }
//       }
//       else {
//         switch((int)((DAT_80097b1e + 1) * 0x10000) >> 0x10) {
//         case 0:
//         case 0xb:
//           SoundEx(Me_MOTION_C->locate,0xc);
//           return;
//         case 1:
//           sVar2 = 0x400;
//           break;
//         case 2:
//           sVar2 = 0xe00;
//           break;
//         case 3:
//           sVar2 = 0xf00;
//           break;
//         default:
//           ReqItemDefault(Me_MOTION_C,(int)(short)DAT_80097b1e);
//           return;
//         case 5:
//           sVar2 = 0xf02;
//           break;
//         case 6:
//           sVar2 = 0xf02;
//           break;
//         case 7:
//           sVar2 = 0xf03;
//         }
//       }
//     }
//     else if (dtCMD == 2) {
//       sVar2 = 0x203;
//     }
//     else {
//       if (dtCMD < 3) {
//         if (dtCMD != 1) {
//           return;
//         }
//         motID = 0x202;
//         DAT_80097f0e = dtCMD;
//         return;
//       }
//       if (dtCMD == 3) {
//         sVar2 = 0x205;
//       }
//       else {
//         sVar2 = 0x204;
//         if (dtCMD != 4) {
//           return;
//         }
//       }
//     }
//   }
//   DAT_80097f0e = 1;
//   motID = sVar2;
//   return;
// }

#endif /* NON_MATCHING */
