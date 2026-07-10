#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActENGAGE(void);
 *     MOTION.C:1181, 56 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern short motID;
 *     extern short dtCMD;
 *     extern short ActionHalt;
 *     extern struct SVECTOR *dtR;
 *     extern struct SVECTOR *dtV;
 *     extern struct Humanoid *StagePlayer;
 *     extern long GameClock;
 *     extern struct VECTOR *dtL;
 * END PSX.SYM */

/*
 * ActENGAGE (0x80021270) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=ActENGAGE
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", ActENGAGE);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_800212b4__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_800212b4__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_800212b4__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_800212b4__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_800212b4__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_800212b4__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_800212b4__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_800212b4__caseD_6);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_8002166c__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_8002166c__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_8002166c__caseD_20);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_8002166c__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_8002166c__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_8002166c__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_80021700__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_80021700__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_80021700__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_80021700__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_80021700__caseD_6);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_80021700__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_80021700__caseD_7);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_80021700__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_80021700__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActENGAGE", switchD_8002166c__caseD_4);

/* jump-table pool @ 0x80011388 (52 words; tables at 0x80011388, 0x800113a0, 0x80011428) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 ActENGAGE_jtbl[52] = {
    0x80021478, 0x800212BC, 0x800215DC, 0x800215AC,
    0x800213E0, 0x80021420, 0x80021674, 0x80021684,
    0x80021694, 0x8002168C, 0x800217CC, 0x800217CC,
    0x800217CC, 0x800217CC, 0x800217CC, 0x800217CC,
    0x800217CC, 0x800217CC, 0x800217CC, 0x800217CC,
    0x800217CC, 0x800217CC, 0x800217CC, 0x800217CC,
    0x800217CC, 0x800217CC, 0x800217CC, 0x800217CC,
    0x800217CC, 0x800217CC, 0x800217CC, 0x800217CC,
    0x800217CC, 0x800217CC, 0x800217CC, 0x800217CC,
    0x800217CC, 0x800217CC, 0x8002167C, 0x00000000,
    0x80021738, 0x80021710, 0x80021708, 0x80021718,
    0x80021754, 0x80021728, 0x80021720, 0x80021730,
    0x80021754, 0x80021754, 0x80021754, 0x80021738,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// void ActENGAGE(void)
// 
// {
//   short sVar1;
//   Humanoid *human;
//   MotionManager *pMVar2;
//   short sVar3;
//   ushort uVar4;
//   int iVar5;
//   
//   pMVar2 = dtM;
//   human = Me_MOTION_C;
//   switch((int)(((ushort)dtM->mid - 0x500) * 0x10000) >> 0x10) {
//   case 0:
//     sVar3 = dtV->vx;
//     if (sVar3 != 0) {
//       if (sVar3 < 1) {
//         sVar1 = 4;
//       }
//       else {
//         sVar1 = -4;
//       }
//       dtV->vx = sVar3 + sVar1;
//     }
//     sVar3 = dtV->vz;
//     if (sVar3 != 0) {
//       if (sVar3 < 1) {
//         sVar1 = 4;
//       }
//       else {
//         sVar1 = -4;
//       }
//       dtV->vz = sVar3 + sVar1;
//     }
//     pMVar2 = dtM;
//     sVar3 = dtM->count + -1;
//     dtM->count = sVar3;
//     if (sVar3 < pMVar2->loop) {
//       if ((dtPAD & 0x4000) == 0) {
//         if (Me_MOTION_C == StagePlayer) {
//           SetCameraMode(CMODE_NORMAL);
//         }
//         if ((Me_MOTION_C->attribute & 0x40U) == 0) {
//           motID = 0;
//         }
//         else {
//           motID = 0x501;
//         }
//       }
//       else {
//         motID = 0x602;
//       }
//       DAT_80097f0e = 1;
//     }
//     if ((GameClock & 3U) != 0) {
//       return;
//     }
//     FUN_80033bc0(dtL,300,10,5);
//     return;
//   case 1:
//     sVar3 = 0x504;
//     if (((dtPAD & 0x2000) == 0) && (sVar3 = 0x505, (dtPAD & 0x8000) == 0)) {
//       if (dtCMD == 0x22) {
//         sVar3 = 0x712;
// LAB_8002137c:
//         DAT_80097f0e = 1;
//         motID = sVar3;
//       }
//       else if (dtCMD == 0x31) {
//         motID = 0x907;
//         DAT_80097f0e = 0;
//         MoveHumanoid(Me_MOTION_C,0x78,0);
//       }
//       else if (dtM->count == 0) {
//         iVar5 = rand();
//         sVar3 = 0x713;
//         if (iVar5 == (iVar5 / 0x14) * 0x14) goto LAB_8002137c;
//       }
//     }
//     else {
//       DAT_80097f0e = 0;
//       motID = sVar3;
//     }
//     if ((ActionHalt == -1) && (dtM->count == 0)) {
//       uVar4 = GetMotionID(dtM,0x503);
//       motID = 0x503;
//       if ((int)((uint)uVar4 << 0x10) < 0) {
//         motID = 0x80f;
//       }
//       DAT_80097f0e = 1;
//     }
//     break;
//   case 2:
//     if (dtM->count != 0) {
//       return;
//     }
//     uVar4 = dtM->loop;
//     sVar3 = 0x501;
//     goto joined_r0x800215fc;
//   case 3:
//     if (dtM->count != 0) {
//       return;
//     }
//     uVar4 = dtM->loop;
//     sVar3 = 0x80f;
//     goto joined_r0x800215fc;
//   case 4:
//     dtR->vy = dtR->vy + Me_MOTION_C->turn;
//     if (pMVar2->count == 1) {
//       Sound(human,0x10);
//     }
//     uVar4 = dtPAD & 0x2000;
//     goto LAB_80021460;
//   case 5:
//     dtR->vy = dtR->vy - Me_MOTION_C->turn;
//     if (pMVar2->count == 1) {
//       Sound(human,0x10);
//     }
//     uVar4 = dtPAD & 0x8000;
// LAB_80021460:
//     if (uVar4 == 0) {
//       motID = 0x501;
//       DAT_80097f0e = 1;
//     }
//   }
//   if ((Me_MOTION_C->attribute & 0x40U) == 0) {
//     motID = 0;
//     sVar3 = motID;
//   }
//   else if (dtCMD == 0) {
//     uVar4 = (Me_MOTION_C->pad).trig;
//     if ((uVar4 & 0x40) != 0) {
//       JumpControl();
//       return;
//     }
//     if ((uVar4 & 0x10) == 0) {
//       if ((dtPAD & 0x20) == 0) {
//         if ((uVar4 & 0x80) != 0) {
//           AttackControl();
//           return;
//         }
//         sVar3 = 0x600;
//         if ((dtPAD & 0x1000) == 0) {
//           uVar4 = dtPAD & 0x4000;
//           sVar3 = 0x602;
// joined_r0x800215fc:
//           if (uVar4 == 0) {
//             return;
//           }
//         }
//       }
//       else {
//         sVar3 = 0x70c;
//         if ((uVar4 & 0x80) == 0) {
//           sVar3 = 0xb00;
//         }
//       }
//     }
//     else {
//       switch((int)((DAT_80097b1e + 1) * 0x10000) >> 0x10) {
//       case 0:
//       case 0xb:
//         SoundEx(Me_MOTION_C->locate,0xc);
//         return;
//       case 1:
//         sVar3 = 0x400;
//         break;
//       case 2:
//         sVar3 = 0xe00;
//         break;
//       case 3:
//         sVar3 = 0xf00;
//         break;
//       default:
//         ReqItemDefault(Me_MOTION_C,(int)(short)DAT_80097b1e);
//         return;
//       case 5:
//         sVar3 = 0xf02;
//         break;
//       case 6:
//         sVar3 = 0xf02;
//         break;
//       case 7:
//         sVar3 = 0xf03;
//       }
//     }
//   }
//   else {
//     switch((int)((dtCMD - 1) * 0x10000) >> 0x10) {
//     case 0:
//       sVar3 = 0x607;
//       break;
//     case 1:
//       sVar3 = 0x604;
//       break;
//     case 2:
//       sVar3 = 0x606;
//       break;
//     case 3:
//       sVar3 = 0x605;
//       break;
//     default:
//       goto switchD_8002166c_caseD_4;
//     case 0x20:
//       sVar3 = 0x70d;
//     }
//   }
//   motID = sVar3;
//   DAT_80097f0e = 1;
// switchD_8002166c_caseD_4:
//   return;
// }

#endif /* NON_MATCHING */
