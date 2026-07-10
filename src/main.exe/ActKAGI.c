#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActKAGI(void);
 *     MOTION.C:1091, 86 src lines, frame 88 bytes, saved-reg mask 0x80010000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     stack sp+24     struct VECTOR v
 *     reg   $s0       long dist
 *     stack sp+40     struct PARAM_ITEM_LAUNCH item
 *     reg   $v1       short ry
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern struct VECTOR *dtL;
 *     extern struct TCameraStatus CamState;
 *     extern short motID;
 *     extern struct Humanoid *StagePlayer;
 *     extern struct SVECTOR *dtR;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct SVECTOR *dtV;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ActKAGI", ActKAGI);

// triage: HARD — 524 insns, mul/div, 5 loop, 12 callees, ~0.06 to ProcItemGoshikimai
// likely-relevant cookbook sections:
//   - Loops: 5 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ActKAGI(void)
//
// {
//   short *psVar1;
//   bool bVar2;
//   Humanoid *pHVar3;
//   SVECTOR *pSVar4;
//   SVECTOR *pSVar5;
//   MotionManager *pMVar6;
//   short sVar7;
//   ushort uVar8;
//   int iVar9;
//   long lVar10;
//   VECTOR *pVVar11;
//   ModelType *pMVar12;
//   ushort uVar13;
//   int iVar14;
//   ModelArchiveType *pMVar15;
//   int local_40;
//   int local_3c;
//   int local_38;
//   PARAM_ITEM_USE local_30;
//
//   sVar7 = dtM->mid;
//   if (sVar7 == 0x401) {
//     if ((dtM->count == 0) && (dtM->loop == 1)) {
//       pVVar11 = GetAbsolutePosition(Me_MOTION_C->model->object[0xe],0,0,0);
//       dtM->loop = -1;
//       iVar14 = SetFlyWire(pVVar11,&CamState.TargetVector);
//       dtM->count = (short)iVar14;
//     }
//     pMVar6 = dtM;
//     if (dtM->loop != -1) {
//       return;
//     }
//     uVar8 = dtM->count - 1;
//     dtM->count = uVar8;
//     pHVar3 = Me_MOTION_C;
//     if (-1 < (int)((uint)uVar8 << 0x10)) {
//       return;
//     }
//     motID = 0x402;
//     pMVar6->mask = -2;
//     DAT_80097f0e = 1;
//     if (((pHVar3->map).attrib & 4U) != 0) {
//       Sound(pHVar3,0x15);
//     }
//     Sound(Me_MOTION_C,0x18);
//     return;
//   }
//   if (0x401 < sVar7) {
//     if (sVar7 != 0x402) {
//       return;
//     }
//     SetCameraMode(CMODE_FALL|CMODE_DIRECTION);
//     local_40 = CamState.TargetVector.vx - dtL->vx;
//     local_3c = CamState.TargetVector.vy - dtL->vy;
//     local_38 = CamState.TargetVector.vz - dtL->vz;
//     lVar10 = SquareRoot0(local_40 * local_40 + local_38 * local_38);
//     sVar7 = GetDirection(local_40,local_38,dtR->vy);
//     ((*Me_MOTION_C->model->object)->rotate).vy = sVar7;
//     lVar10 = ratan2(lVar10,-local_3c);
//     pHVar3 = Me_MOTION_C;
//     ((*Me_MOTION_C->model->object)->rotate).vx = (short)lVar10;
//     UpdateCoordinate(*pHVar3->model->object);
//     iVar14 = local_40;
//     if (local_40 < 0) {
//       iVar14 = -local_40;
//     }
//     if (iVar14 < 400) {
//       iVar14 = local_3c;
//       if (local_3c < 0) {
//         iVar14 = -local_3c;
//       }
//       if (iVar14 < 400) {
//         iVar14 = local_38;
//         if (local_38 < 0) {
//           iVar14 = -local_38;
//         }
//         if (iVar14 < 400) {
//           Me_MOTION_C->attribute = Me_MOTION_C->attribute | 0x400;
//         }
//       }
//     }
//     pSVar5 = dtV;
//     pSVar4 = dtR;
//     pHVar3 = Me_MOTION_C;
//     if (((int)Me_MOTION_C->attribute & 0xce00U) == 0) {
//       iVar14 = local_40;
//       if (local_40 < 0) {
//         iVar14 = -local_40;
//       }
//       if (400 < iVar14) goto LAB_8002119c;
//       iVar14 = local_3c;
//       if (local_3c < 0) {
//         iVar14 = -local_3c;
//       }
//       if (400 < iVar14) goto LAB_8002119c;
//       iVar14 = local_38;
//       if (local_38 < 0) {
//         iVar14 = -local_38;
//       }
//       while (400 < iVar14) {
// LAB_8002119c:
//         do {
//           do {
//             local_40 = local_40 >> 1;
//             local_3c = local_3c >> 1;
//             local_38 = local_38 >> 1;
//             iVar14 = local_40;
//             if (local_40 < 0) {
//               iVar14 = -local_40;
//             }
//           } while (400 < iVar14);
//           iVar14 = local_3c;
//           if (local_3c < 0) {
//             iVar14 = -local_3c;
//           }
//         } while (400 < iVar14);
//         iVar14 = local_38;
//         if (local_38 < 0) {
//           iVar14 = -local_38;
//         }
//       }
//       dtV->vx = (short)local_40;
//       pSVar5->vy = (short)local_3c;
//       pSVar5->vz = (short)local_38;
//       pVVar11 = GetAbsolutePosition(pHVar3->model->object[0xe],0,0,0);
//       SetWire(pVVar11,&CamState.TargetVector,(VECTOR *)0x0,0x1000);
//       return;
//     }
//     sVar7 = dtR->vy;
//     uVar8 = sVar7 + ((*Me_MOTION_C->model->object)->rotate).vy;
//     uVar13 = uVar8 & 0xc00;
//     dtR->vy = uVar8;
//     if ((uVar8 & 0x200) != 0) {
//       uVar13 = uVar13 + 0x400;
//     }
//     pSVar4->vy = uVar13;
//     pMVar6 = dtM;
//     pMVar12 = *pHVar3->model->object;
//     motID = 0x803;
//     DAT_80097f0e = 0;
//     (pMVar12->rotate).vy = (pMVar12->rotate).vy + (sVar7 - uVar13);
//     bVar2 = MotionUpdateMode != 0;
//     pMVar6->mask = 0x7fff;
//     if (bVar2) {
//       iVar9 = 0;
//       iVar14 = 0;
//       do {
//         iVar9 = iVar9 + 1;
//         if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar14 >> 0xd)) == pHVar3) goto LAB_800210c8;
//         iVar14 = iVar9 * 0x10000;
//       } while (iVar9 * 0x10000 >> 0x10 < 5);
//     }
//     SetNowMotion(Me_MOTION_C,motID,DAT_80097f0e);
//     DAT_80097f0e = -1;
// LAB_800210c8:
//     pHVar3 = Me_MOTION_C;
//     dtM->count = dtM->count >> 1;
//     pSVar4 = dtV;
//     if ((pHVar3->map).vector == '\x0f') {
//       psVar1 = &dtV->vz;
//       dtV->vx = dtV->vx >> 1;
//       pSVar4->vz = *psVar1 >> 1;
//     }
//     else {
//       dtV->vz = 0;
//       pSVar4->vx = 0;
//     }
//     dtV->vy = 0;
//     return;
//   }
//   if (sVar7 != 0x400) {
//     return;
//   }
//   if ((dtM->count == 0) && (dtM->loop != 0)) {
//     dtM->loop = -1;
//   }
//   if (dtM->count == 1) {
//     local_30.type = ITEM_KAGINAWA;
//     local_30.user = Me_MOTION_C;
//     local_30.start.vx = dtL->vx;
//     local_30.start.vy = (dtL->vy - (int)Me_MOTION_C->height) + 300;
//     local_30.start.vz = dtL->vz;
//     local_30.end.vx = local_30.start.vx;
//     local_30.end.vy = local_30.start.vy;
//     local_30.end.vz = local_30.start.vz;
//     ReqItemUse(&local_30);
//     Sound(Me_MOTION_C,0x1e);
//     goto LAB_80020c90;
//   }
//   iVar14 = FUN_8004a368(1,Me_MOTION_C);
//   if (iVar14 == 0) {
//     DAT_80097f0e = 1;
//     iVar14 = CamState.TargetVector.vx - dtL->vx;
//     motID = 0x401;
//     iVar9 = CamState.TargetVector.vz - dtL->vz;
//     if ((iVar14 != 0) || (iVar9 != 0)) {
//       sVar7 = GetDirection(iVar14,iVar9,dtR->vy);
//       pHVar3 = Me_MOTION_C;
//       dtR->vy = dtR->vy + sVar7;
//       Sound(pHVar3,0x1f);
//       goto LAB_80020c90;
//     }
//     if (Me_MOTION_C == StagePlayer) {
//       SetCameraMode(CMODE_NORMAL);
//     }
//     if ((Me_MOTION_C->attribute & 0x40U) == 0) {
// LAB_80020c88:
//       motID = 0;
//     }
//     else {
//       motID = 0x501;
//     }
//   }
//   else {
//     if (((Me_MOTION_C->pad).trig & 0xe0) == 0) goto LAB_80020c90;
//     FUN_8004a368(0);
//     if (Me_MOTION_C == StagePlayer) {
//       SetCameraMode(CMODE_NORMAL);
//     }
//     if ((Me_MOTION_C->attribute & 0x40U) == 0) goto LAB_80020c88;
//     motID = 0x501;
//   }
//   DAT_80097f0e = 1;
// LAB_80020c90:
//   if ((((Me_MOTION_C->map).attrib & 4U) != 0) && ((int)((uint)(ushort)motID << 0x10) >> 0x18 != 4))
//   {
//     pMVar15 = Me_MOTION_C->model;
//     sVar7 = pMVar15->n + -1;
//     if (0xc < pMVar15->n) {
//       sVar7 = 0xc;
//     }
//     iVar14 = 7;
//     if (6 < sVar7) {
//       do {
//         iVar9 = iVar14 << 0x10;
//         iVar14 = iVar14 + 1;
//         iVar9 = *(int *)((iVar9 >> 0xe) + (int)pMVar15->object);
//         *(ushort *)(iVar9 + 0x5a) = *(ushort *)(iVar9 + 0x5a) | 1;
//       } while (iVar14 * 0x10000 >> 0x10 <= (int)sVar7);
//     }
//     (*pMVar15->object)->attribute = (*pMVar15->object)->attribute | 1;
//     motID = 0x300;
//     DAT_80097f0e = 1;
//     dtM->mask = 0x7fff;
//   }
//   return;
// }
