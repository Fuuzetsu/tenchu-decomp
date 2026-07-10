#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActMOVE(void);
 *     MOTION.C:994, 32 src lines, frame 24 bytes, saved-reg mask 0x80010000
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern short motID;
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 * END PSX.SYM */

/*
 * ActMOVE (0x80020108) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=ActMOVE
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActMOVE", ActMOVE);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActMOVE", switchD_800203b4__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActMOVE", switchD_800203b4__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActMOVE", switchD_800203b4__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActMOVE", switchD_800203b4__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActMOVE", switchD_800203b4__caseD_6);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActMOVE", switchD_800203b4__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActMOVE", switchD_800203b4__caseD_7);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActMOVE", switchD_800203b4__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActMOVE", switchD_800203b4__caseD_4);

/* jump-table pool @ 0x80011328 (12 words; tables at 0x80011328) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 ActMOVE_jtbl[12] = {
    0x800203EC, 0x800203C4, 0x800203BC, 0x800203CC,
    0x80020408, 0x800203DC, 0x800203D4, 0x800203E4,
    0x80020408, 0x80020408, 0x80020408, 0x800203EC,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// void ActMOVE(void)
// 
// {
//   ushort uVar1;
//   Humanoid *pHVar2;
//   short sVar3;
//   int iVar4;
//   MotionDataType *pMVar5;
//   long lVar6;
//   
//   sVar3 = dtM->mid;
//   if (sVar3 == 0x201) {
//     if (dtM->count == 1) {
//       Sound(Me_MOTION_C,0x11);
//     }
//     if ((dtPAD & 0x4000) == 0) {
//       motID = 0;
//       DAT_80097f0e = 1;
//     }
//     if ((dtPAD & 0x2000) == 0) {
//       sVar3 = dtR->vy - Me_MOTION_C->turn;
//     }
//     else {
//       sVar3 = dtR->vy + Me_MOTION_C->turn;
//     }
//   }
//   else {
//     if (0x201 < sVar3) {
//       if (sVar3 < 0x206) {
//         if (dtM->count == 1) {
//           Sound(Me_MOTION_C,0x13);
//         }
//         else if ((dtM->count == 0) && (dtM->loop != 0)) {
//           motID = 0;
//           DAT_80097f0e = 1;
//         }
//       }
//       goto LAB_80020348;
//     }
//     if (sVar3 != 0x200) goto LAB_80020348;
//     if ((dtM->count == 1) ||
//        (iVar4 = (uint)(ushort)dtM->motion->time << 0x10,
//        (int)dtM->count == (iVar4 >> 0x10) - (iVar4 >> 0x1f) >> 1)) {
//       Sound(Me_MOTION_C,0x11);
//     }
//     pHVar2 = Me_MOTION_C;
//     if ((dtPAD & 0x1000) == 0) {
//       motID = 0;
//       DAT_80097f0e = 1;
//       goto LAB_80020348;
//     }
//     if ((Me_MOTION_C->attribute & 0x400U) != 0) {
//       iVar4 = dtL->vy;
//       lVar6 = (Me_MOTION_C->map).height;
//       dtL->vy = iVar4 + -400;
//       (pHVar2->map).height = 1;
//       sVar3 = HangCheck();
//       pHVar2 = Me_MOTION_C;
//       if (sVar3 == 0) {
//         dtL->vy = iVar4;
//         (pHVar2->map).height = lVar6;
//       }
//       goto LAB_80020348;
//     }
//     if ((dtPAD & 0xa000) == 0) goto LAB_80020348;
//     if ((dtPAD & 0x2000) == 0) {
//       sVar3 = dtR->vy - Me_MOTION_C->turn;
//     }
//     else {
//       sVar3 = dtR->vy + Me_MOTION_C->turn;
//     }
//   }
//   pHVar2 = Me_MOTION_C;
//   dtR->vy = sVar3;
//   pMVar5 = pHVar2->motion->motion;
//   MoveHumanoid(pHVar2,(ushort)pMVar5->orderspd,(ushort)pMVar5->sidespd);
// LAB_80020348:
//   uVar1 = (Me_MOTION_C->pad).trig;
//   if ((uVar1 & 0x40) != 0) {
//     JumpControl();
//     return;
//   }
//   if ((uVar1 & 0x10) == 0) {
//     sVar3 = 0xb00;
//     if (((dtPAD & 0x20) == 0) && (sVar3 = 0x80e, (uVar1 & 0x80) == 0)) {
//       return;
//     }
//   }
//   else {
//     switch((int)((DAT_80097b1e + 1) * 0x10000) >> 0x10) {
//     case 0:
//     case 0xb:
//       SoundEx(Me_MOTION_C->locate,0xc);
//       return;
//     case 1:
//       sVar3 = 0x400;
//       break;
//     case 2:
//       sVar3 = 0xe00;
//       break;
//     case 3:
//       sVar3 = 0xf00;
//       break;
//     default:
//       ReqItemDefault(Me_MOTION_C,(int)(short)DAT_80097b1e);
//       return;
//     case 5:
//       sVar3 = 0xf02;
//       break;
//     case 6:
//       sVar3 = 0xf02;
//       break;
//     case 7:
//       sVar3 = 0xf03;
//     }
//   }
//   DAT_80097f0e = 1;
//   motID = sVar3;
//   return;
// }

#endif /* NON_MATCHING */
