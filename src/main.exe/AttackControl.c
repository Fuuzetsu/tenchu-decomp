#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackControl(void);
 *     MOTION.C:648, 74 src lines, frame 48 bytes, saved-reg mask 0x80030000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     stack sp+16     struct SVECTOR vect
 *     reg   $a1       short emid
 *     reg   $a2       short myid
 *     stack sp+26     short deg
 *     stack sp+24     short mydeg
 *     reg   $s0       struct Humanoid * enemy
 *     reg   $s0       struct Humanoid * human
 *     reg   $a1       short mid
 *     reg   $v0       struct Humanoid * enemy
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *StagePlayer;
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern short motID;
 *     extern short Criticals;
 *     extern short dtPAD;
 *     extern struct MotionManager *dtM;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/AttackControl", AttackControl);

// triage: MEDIUM — 260 insns, 6 callees, ~0.06 to ProcItemDrop
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void AttackControl(void)
//
// {
//   VECTOR *pVVar1;
//   VECTOR *pVVar2;
//   short sVar3;
//   ushort uVar4;
//   Humanoid *human;
//   int iVar5;
//   MotionDataType *pMVar6;
//   Humanoid *pHVar7;
//   uint uVar8;
//   short sVar9;
//   ModelType *pMVar10;
//   short local_18;
//   short local_16 [3];
//
//   if (((ushort)Me_MOTION_C->type < 2) &&
//      (human = GetNearestHumanoid(Me_MOTION_C,Me_MOTION_C->width + 1000), pHVar7 = Me_MOTION_C,
//      human != (Humanoid *)0x0)) {
//     uVar4 = human->type;
//     uVar8 = uVar4 & 0xf0;
//     if (uVar8 == 0x80) {
// LAB_8001ee10:
//       human = (Humanoid *)0x0;
//     }
//     else if (uVar8 < 0x81) {
//       if (((uVar4 & 0xf0) == 0) && (2 < uVar4 - 7)) goto LAB_8001ee10;
//     }
//     else if ((uVar8 == 0x90) || (uVar8 == 0xa0)) goto LAB_8001ee10;
//     if ((((human != (Humanoid *)0x0) && ((human->attribute & 0x43U) == 0)) && (human->status != 0xf)
//         ) && (human->status != 1)) {
//       Me_MOTION_C->target = (ModelType *)human->model;
//       GetTargetDistance(pHVar7,&local_18);
//       pMVar10 = human->target;
//       human->target = (ModelType *)StagePlayer->model;
//       GetTargetDistance(human,local_16);
//       pVVar1 = dtL;
//       human->target = pMVar10;
//       pVVar2 = dtL;
//       if ((pVVar1->vy == human->locate->vy) &&
//          (uVar8._0_1_ = (Me_MOTION_C->map).vector, uVar8._1_1_ = (Me_MOTION_C->map).direct,
//          uVar8._2_1_ = (Me_MOTION_C->map).angleL, uVar8._3_1_ = (Me_MOTION_C->map).angleH,
//          (uVar8 & 0xffff00ff) == 0)) {
//         iVar5 = (int)local_16[0];
//         if (iVar5 < 0) {
//           iVar5 = -iVar5;
//         }
//         if (1000 < iVar5) {
//           iVar5 = (int)local_18;
//           if (iVar5 < 0) {
//             iVar5 = -iVar5;
//           }
//           motID = 0x714;
//           if (iVar5 < 1000) {
//             sVar9 = 0x1109;
//             goto LAB_8001ef4c;
//           }
//         }
//         iVar5 = (int)local_16[0];
//         if (iVar5 < 0) {
//           iVar5 = -iVar5;
//         }
//         if (iVar5 < 1000) {
//           iVar5 = (int)local_18;
//           if (iVar5 < 0) {
//             iVar5 = -iVar5;
//           }
//           motID = 0x715;
//           if (iVar5 < 1000) {
//             sVar9 = 0x110a;
//             goto LAB_8001ef4c;
//           }
//         }
//         motID = 0x716;
//         sVar9 = 0x110b;
// LAB_8001ef4c:
//         if (Me_MOTION_C->type == 1) {
//           motID = motID + 3;
//           sVar9 = sVar9 + 3;
//         }
//         human->rotate->vy = dtR->vy;
//         human->locate->vx = pVVar2->vx;
//         DAT_80097f0e = 1;
//         human->locate->vz = pVVar2->vz;
//         human->life = 0;
//         if (((human->status != 0x11) || (human->motion->loop != -1)) &&
//            (sVar3 = UpdateMotion(human->motion,sVar9), sVar3 != 0)) {
//           human->status = (short)(char)((ushort)sVar9 >> 8);
//           pMVar6 = human->motion->motion;
//           MoveHumanoid(human,(ushort)pMVar6->orderspd,(ushort)pMVar6->sidespd);
//         }
//         DeleteConflict(*human->model->object);
//         Criticals = Criticals + 1;
//         return;
//       }
//     }
//   }
//   if ((dtPAD & 0x4000) == 0) {
//     if (motID == 0xb00) {
//       uVar4 = GetMotionID(dtM,0x70c);
//       sVar3 = 0x70c;
//       sVar9 = motID;
//       goto joined_r0x8001f09c;
//     }
//     if (motID == 0x607) {
//       if (10 < dtM->count) {
//         return;
//       }
//       motID = 0x700;
//       DAT_80097f0e = 1;
//       uVar4 = GetMotionID(dtM,0x70d);
//       if (-1 < (int)((uint)uVar4 << 0x10)) {
//         motID = 0x70d;
//         DAT_80097f0e = 1;
//       }
//       goto LAB_8001f130;
//     }
//     motID = 0x700;
//     if ((((dtPAD & 0x1000) == 0) && (motID = 0x706, (dtPAD & 0x2000) == 0)) &&
//        (motID = 0x709, (dtPAD & 0x8000) == 0)) {
//       motID = 0x700;
//     }
//   }
//   else {
//     uVar4 = GetMotionID(dtM,0x711);
//     sVar3 = 0x711;
//     sVar9 = motID;
// joined_r0x8001f09c:
//     motID = sVar3;
//     if ((int)((uint)uVar4 << 0x10) < 0) {
//       motID = sVar9;
//       return;
//     }
//   }
//   DAT_80097f0e = 1;
// LAB_8001f130:
//   if (Me_MOTION_C == StagePlayer) {
//     pHVar7 = GetNearestHumanoid(Me_MOTION_C,3000);
//     if (pHVar7 == (Humanoid *)0x0) {
//       Me_MOTION_C->target = (ModelType *)0x0;
//     }
//     else {
//       Me_MOTION_C->target = (ModelType *)pHVar7->model;
//     }
//   }
//   return;
// }
