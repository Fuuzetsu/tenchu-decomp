#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SnapCameraTargetVector(void);
 *     CAMERA.C:106, 30 src lines, frame 72 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     stack sp+16     struct VECTOR v
 *     stack sp+32     struct SVECTOR sv
 *     stack sp+40     struct SVECTOR sv2
 *     reg   $a0       struct VECTOR * target
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SnapCameraTargetVector", SnapCameraTargetVector);

// triage: EASY — 117 insns, 3 callees, ~0.12 to DebugMenuItemSet

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void SnapCameraTargetVector(void)
//
// {
//   int iVar1;
//   VECTOR *pVVar2;
//   int iVar3;
//   int iVar4;
//   VECTOR local_38;
//   SVECTOR local_28;
//   SVECTOR local_20;
//
//   memset((uchar *)&local_28,'\0',0x10);
//   local_28.vx = (short)ViewInfo.vpx;
//   local_28.vy = ViewInfo.vpx._2_2_;
//   local_28.vz = (short)ViewInfo.vpy;
//   local_28.pad = ViewInfo.vpy._2_2_;
//   local_20.vx = (short)ViewInfo.vpz;
//   local_20.vy = ViewInfo.vpz._2_2_;
//   local_38.vx = ViewInfo.vpx;
//   local_38.vy = ViewInfo.vpy;
//   local_38.vz = ViewInfo.vpz;
//   local_38.pad._0_2_ = local_20.vz;
//   local_38.pad._2_2_ = local_20.pad;
//   memset((uchar *)&local_20,'\0',8);
//   local_20.vx = (short)ViewInfo.vrx - (short)ViewInfo.vpx;
//   local_20.vy = (short)ViewInfo.vry - (short)ViewInfo.vpy;
//   local_28.vz = (short)ViewInfo.vrz - (short)ViewInfo.vpz;
//   local_28.pad = local_20.pad;
//   local_28.vx = local_20.vx;
//   local_28.vy = local_20.vy;
//   local_20.vz = local_28.vz;
//   VectorNormalSS(&local_28,&local_20);
//   iVar1 = (int)local_20.vx;
//   if (iVar1 < 0) {
//     iVar1 = iVar1 + 0x1f;
//   }
//   iVar3 = (int)local_20.vy;
//   if (iVar3 < 0) {
//     iVar3 = iVar3 + 0x1f;
//   }
//   iVar4 = (int)local_20.vz;
//   local_20.vy = (short)(iVar3 >> 5);
//   local_20.vx = (short)(iVar1 >> 5);
//   if (iVar4 < 0) {
//     iVar4 = iVar4 + 0x1f;
//   }
//   local_20.vz = (short)(iVar4 >> 5);
//   pVVar2 = GetAreaMapPassage(GlobalAreaMap,&local_38,&local_20,-1);
//   if (pVVar2 == (VECTOR *)0x0) {
//     CamState.TargetVector.vx = ((CamState.Owner)->model->locate).coord.t[0];
//     CamState.TargetVector.vy = ((CamState.Owner)->model->locate).coord.t[1];
//     CamState.TargetVector.vz = ((CamState.Owner)->model->locate).coord.t[2];
//   }
//   else {
//     CamState.TargetVector.vx = pVVar2->vx;
//     CamState.TargetVector.vy = pVVar2->vy;
//     CamState.TargetVector.vz = pVVar2->vz;
//   }
//   return;
// }
