#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActCHASE(void);
 *     MOTION.C:1241, 61 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *     reg   $s1       short turn
 *     reg   $v1       short i
 *     reg   $s0       long y
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern short MotionUpdateMode;
 *     extern short motID;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern short dtCMD;
 * END PSX.SYM */

/*
 * ActCHASE (0x800217dc) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=ActCHASE
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActCHASE", ActCHASE);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActCHASE", switchD_8002183c__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActCHASE", switchD_8002183c__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActCHASE", switchD_8002183c__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActCHASE", switchD_8002183c__caseD_7);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActCHASE", switchD_8002183c__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActCHASE", switchD_8002183c__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActCHASE", switchD_80021cc0__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActCHASE", switchD_80021cc0__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActCHASE", switchD_80021cc0__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActCHASE", switchD_80021cc0__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActCHASE", switchD_80021cc0__caseD_6);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActCHASE", switchD_80021cc0__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActCHASE", switchD_80021cc0__caseD_7);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActCHASE", switchD_80021cc0__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActCHASE", switchD_80021cc0__caseD_4);

/* jump-table pool @ 0x80011458 (20 words; tables at 0x80011458, 0x80011478) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 ActCHASE_jtbl[20] = {
    0x80021844, 0x80021C24, 0x80021A5C, 0x80021C24,
    0x80021B9C, 0x80021B9C, 0x80021B9C, 0x80021B60,
    0x80021D08, 0x80021CD0, 0x80021CC8, 0x80021CD8,
    0x80021D24, 0x80021CE8, 0x80021CE0, 0x80021CF0,
    0x80021D24, 0x80021D24, 0x80021D24, 0x80021D08,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// void ActCHASE(void)
// 
// {
//   ushort uVar1;
//   Humanoid *pHVar2;
//   MotionManager *pMVar3;
//   short sVar4;
//   int iVar5;
//   int iVar6;
//   MotionDataType *pMVar7;
//   uint uVar8;
//   long lVar9;
//   
//   iVar5 = (uint)(ushort)Me_MOTION_C->turn << 0x10;
//   uVar8 = (uint)((iVar5 >> 0x10) - (iVar5 >> 0x1f)) >> 1;
//   switch((int)(((ushort)dtM->mid - 0x600) * 0x10000) >> 0x10) {
//   case 0:
//     if ((dtM->count == 0) ||
//        (iVar5 = (uint)(ushort)dtM->motion->time << 0x10,
//        (int)dtM->count == (iVar5 >> 0x10) - (iVar5 >> 0x1f) >> 1)) {
//       sVar4 = 0x12;
//       if (((Me_MOTION_C->map).attrib & 8U) != 0) {
//         sVar4 = 0x14;
//       }
//       Sound(Me_MOTION_C,sVar4);
//     }
//     pHVar2 = Me_MOTION_C;
//     sVar4 = 0x501;
//     if ((dtPAD & 0x1000) != 0) {
//       if ((Me_MOTION_C->attribute & 0x1000U) != 0) {
//         motID = 0x801;
//         DAT_80097f0e = 0;
//         iVar5 = 0;
//         if (MotionUpdateMode != 0) {
//           iVar6 = 0;
//           do {
//             iVar5 = iVar5 + 1;
//             if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar6 >> 0xd)) == Me_MOTION_C)
//             goto LAB_80021950;
//             iVar6 = iVar5 * 0x10000;
//           } while (iVar5 * 0x10000 >> 0x10 < 5);
//         }
//         SetNowMotion(Me_MOTION_C,0x801,0);
//         DAT_80097f0e = 0xffff;
// LAB_80021950:
//         MoveHumanoid(Me_MOTION_C,0x23,0);
//         pMVar3 = dtM;
//         uVar1 = dtM->mode;
//         if ((uVar1 & 1) == 0) {
//           dtM->mode = uVar1 | 1;
//         }
//         else {
//           dtM->mode = uVar1 & 0xfffe;
//           pMVar3->count = 0xd;
//         }
//         goto switchD_8002183c_caseD_1;
//       }
//       if ((Me_MOTION_C->attribute & 0x400U) != 0) {
//         iVar5 = dtL->vy;
//         lVar9 = (Me_MOTION_C->map).height;
//         dtL->vy = iVar5 + -400;
//         (pHVar2->map).height = 1;
//         sVar4 = HangCheck();
//         pHVar2 = Me_MOTION_C;
//         if (sVar4 == 0) {
//           dtL->vy = iVar5;
//           (pHVar2->map).height = lVar9;
//         }
//         goto switchD_8002183c_caseD_1;
//       }
//       if ((dtPAD & 0xa000) != 0) {
//         sVar4 = (short)uVar8;
//         if ((dtPAD & 0x2000) == 0) {
//           sVar4 = -sVar4;
//         }
//         dtR->vy = dtR->vy + sVar4;
//         pMVar7 = pHVar2->motion->motion;
//         MoveHumanoid(pHVar2,(ushort)pMVar7->orderspd,(ushort)pMVar7->sidespd);
//         goto switchD_8002183c_caseD_1;
//       }
//       sVar4 = 0xb00;
//       if ((dtPAD & 0x20) == 0) goto switchD_8002183c_caseD_1;
//     }
//     break;
//   default:
//     goto switchD_8002183c_caseD_1;
//   case 2:
//     if (dtM->count == 1) {
//       Sound(Me_MOTION_C,0x11);
//     }
//     pHVar2 = Me_MOTION_C;
//     if ((dtPAD & 0x4000) == 0) {
//       motID = 0x501;
// LAB_80021ab0:
//       DAT_80097f0e = 1;
//     }
//     else {
//       if (dtCMD == 0x22) {
//         motID = 0x712;
//         goto LAB_80021ab0;
//       }
//       if ((dtPAD & 0xa000) != 0) {
//         sVar4 = (short)((int)(uVar8 << 0x10) >> 0xe);
//         if ((dtPAD & 0x2000) == 0) {
//           sVar4 = -sVar4;
//         }
//         dtR->vy = dtR->vy + sVar4;
//         pMVar7 = pHVar2->motion->motion;
//         MoveHumanoid(pHVar2,(ushort)pMVar7->orderspd,(ushort)pMVar7->sidespd);
//       }
//     }
//     if ((dtPAD & 0x20) == 0) goto switchD_8002183c_caseD_1;
//     if (((Me_MOTION_C->pad).trig & 0x80) != 0) {
//       motID = 0x70c;
//       DAT_80097f0e = 1;
//       return;
//     }
//     sVar4 = 0xb00;
//     break;
//   case 7:
//     uVar1 = (Me_MOTION_C->pad).trig;
//     if ((uVar1 & 0x80) == 0) {
//       if ((uVar1 & 0x40) != 0) {
//         JumpControl();
//       }
//     }
//     else {
//       AttackControl();
//     }
//   case 4:
//   case 5:
//   case 6:
//     if (dtM->count == 1) {
//       Sound(Me_MOTION_C,0x13);
//     }
//     if (dtM->count < 7) {
//       FUN_80033bc0(dtL,0x96,0xc,1);
//     }
//     if (dtM->count == 0) {
//       if (dtM->loop != 0) {
//         motID = 0x501;
//         DAT_80097f0e = 1;
//         return;
//       }
//       return;
//     }
//     return;
//   }
//   DAT_80097f0e = 1;
//   motID = sVar4;
// switchD_8002183c_caseD_1:
//   if (dtCMD == 0x31) {
//     motID = 0x907;
//     DAT_80097f0e = 0;
//     MoveHumanoid(Me_MOTION_C,0x78,0);
//     return;
//   }
//   uVar1 = (Me_MOTION_C->pad).trig;
//   if ((uVar1 & 0x40) != 0) {
//     JumpControl();
//     return;
//   }
//   if ((uVar1 & 0x10) != 0) {
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
//   if ((uVar1 & 0x80) != 0) {
//     AttackControl();
//     return;
//   }
//   return;
// }

#endif /* NON_MATCHING */
