#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void CameraDirection(struct Humanoid *pl, struct GsRVIEW2 *vDif);
 *     CAMERA.C:898, 114 src lines, frame 160 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct Humanoid * pl
 *     param $s3       struct GsRVIEW2 * vDif
 *     stack sp+24     struct GsRVIEW2 target
 *     reg   $s2       struct ModelArchiveType * mad
 *     stack sp+56     struct SVECTOR r
 *     stack sp+64     struct SVECTOR CamLoc
 *     stack sp+72     struct VECTOR vc
 *     stack sp+88     struct MATRIX mat
 *     stack sp+120    int rx
 *     stack sp+124    int ry
 *     stack sp+128    short x
 *     stack sp+130    short y
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/CameraDirection", CameraDirection);

// triage: MEDIUM — 215 insns, mul/div, 6 callees, ~0.05 to ProcItemTeleport
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void CameraDirection(Humanoid *pl,GsRVIEW2 *vDif)
//
// {
//   short sVar1;
//   int iVar2;
//   int iVar3;
//   uint uVar4;
//   int iVar5;
//   int iVar6;
//   ModelArchiveType *pMVar7;
//   SVECTOR local_38;
//   int local_30;
//   int local_2c;
//   int local_28;
//   ushort local_20;
//   ushort local_1e [3];
//
//   pMVar7 = pl->model;
//   GetPadXY(0,(short *)&local_20,(short *)local_1e);
//   if ((undefined1)CamState.OldMode == CMODE_SIGHT) {
//     local_20 = (ushort)(((int)((uint)local_20 << 0x10) >> 0x10) -
//                         ((int)((uint)local_20 << 0x10) >> 0x1f) >> 1);
//     local_1e[0] = (ushort)(((int)((uint)local_1e[0] << 0x10) >> 0x10) -
//                            ((int)((uint)local_1e[0] << 0x10) >> 0x1f) >> 1);
//   }
//   else if ((((CamState.Owner)->pad).data & 4) == 0) {
//     SetCameraMode(CMODE_NORMAL);
//   }
//   uVar4 = (uint)(ushort)CamState.DirectionRX;
//   CamState.DirectionRY = CamState.DirectionRY + local_20;
//   CamState.DirectionRX = (short)(uVar4 - local_1e[0]);
//   iVar5 = (int)((uVar4 - local_1e[0]) * 0x10000) >> 0x10;
//   if (iVar5 < 0x38f) {
//     sVar1 = -0x38e;
//     if (iVar5 < -0x38e) goto LAB_80031d68;
//   }
//   else {
//     sVar1 = 0x38e;
// LAB_80031d68:
//     CamState.DirectionRX = sVar1;
//   }
//   if (CamState.DirectionRY < 0x401) {
//     sVar1 = -0x400;
//     if (-0x401 < CamState.DirectionRY) goto LAB_80031d9c;
//   }
//   else {
//     sVar1 = 0x400;
//   }
//   CamState.DirectionRY = sVar1;
// LAB_80031d9c:
//   iVar5 = rsin((int)(pMVar7->rotate).vy);
//   iVar2 = rcos((int)(pMVar7->rotate).vy);
//   local_38.vz = 0x4b0;
//   local_38.vx = 0;
//   local_38.vy = 0;
//   RotateVectorS(&local_38,(int)(pMVar7->rotate).vx + (int)CamState.DirectionRX,
//                 (int)(pMVar7->rotate).vy + (int)CamState.DirectionRY,(int)(pMVar7->rotate).vz);
//   local_30 = (pMVar7->locate).coord.t[0];
//   local_2c = (pMVar7->locate).coord.t[1] + -0x60e;
//   local_28 = (pMVar7->locate).coord.t[2];
//   FUN_80030644(&local_30,1000);
//   iVar3 = local_30 - iVar5 / 6;
//   iVar5 = (int)local_38.vy;
//   iVar2 = local_28 - iVar2 / 6;
//   if (iVar5 < 1) {
//     if (iVar5 < 0) {
//       iVar5 = iVar5 + 3;
//     }
//     iVar5 = iVar5 >> 2;
//   }
//   else {
//     iVar5 = -iVar5;
//   }
//   iVar6 = (iVar3 - local_38.vx) - ViewInfo.vrx;
//   if (iVar6 < 0) {
//     iVar6 = iVar6 + 3;
//   }
//   vDif->vrx = iVar6 >> 2;
//   iVar6 = ((local_2c + iVar5) - (int)local_38.vy) - ViewInfo.vry;
//   if (iVar6 < 0) {
//     iVar6 = iVar6 + 3;
//   }
//   vDif->vry = iVar6 >> 2;
//   iVar6 = (iVar2 - local_38.vz) - ViewInfo.vrz;
//   if (iVar6 < 0) {
//     iVar6 = iVar6 + 3;
//   }
//   vDif->vrz = iVar6 >> 2;
//   iVar3 = (iVar3 + local_38.vx) - ViewInfo.vpx;
//   if (iVar3 < 0) {
//     iVar3 = iVar3 + 7;
//   }
//   vDif->vpx = iVar3 >> 3;
//   iVar5 = (local_2c + iVar5 + (int)local_38.vy) - ViewInfo.vpy;
//   if (iVar5 < 0) {
//     iVar5 = iVar5 + 7;
//   }
//   vDif->vpy = iVar5 >> 3;
//   iVar5 = (iVar2 + local_38.vz) - ViewInfo.vpz;
//   if (iVar5 < 0) {
//     iVar5 = iVar5 + 7;
//   }
//   vDif->vpz = iVar5 >> 3;
//   return;
// }
