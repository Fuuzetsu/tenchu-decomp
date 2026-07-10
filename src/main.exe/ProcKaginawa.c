#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcKaginawa(struct tag_TItem *item);
 *     ITEM.C:1026, 67 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s0       struct tag_TItem * item
 *     stack sp+16     int rx
 *     stack sp+20     int ry
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int dist
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct GsSPRITE TargetSprite[1];
 *     extern struct GsOT *OTablePt;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcKaginawa", ProcKaginawa);

// triage: MEDIUM — 186 insns, indirect-call, 8 callees, ~0.20 to ProcItemTeleport
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcKaginawa(tag_TItem *item)
//
// {
//   undefined **ppuVar1;
//   int iVar2;
//   ModelArchiveType *pMVar3;
//   VECTOR local_40;
//   int local_30;
//   int local_2c;
//   int local_28;
//   int local_20;
//   int local_1c;
//
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//     return;
//   }
//   if (item->owner->field_0xcd == '\0') {
//     SetCameraMode(CMODE_DIRECTION);
//     ppuVar1 = item->proc;
//     if (ppuVar1 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//   }
//   else if (item->owner->motion->mid == 0x400) {
//     GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_20,&local_1c);
//     if (((item->owner->pad).data & 0x10) != 0) {
//       if (-1 < local_20) {
//         return;
//       }
//       GsSortSprite(TargetSprite,OTablePt,0);
//       return;
//     }
//     local_40.vx = DAT_80012238;
//     local_40.vy = DAT_8001223c;
//     local_40.vz = DAT_80012240;
//     local_40.pad = DAT_80012244;
//     RotateVector(&local_40,local_20,local_1c,0);
//     local_30 = local_40.vx + ViewInfo.vpx;
//     local_2c = local_40.vy + ViewInfo.vpy;
//     local_28 = local_40.vz + ViewInfo.vpz;
//     FUN_80039ddc(&ViewInfo,&local_30,&CamState,0);
//     if (local_40.vx < 0) {
//       local_40.vx = local_40.vx + 0xf;
//     }
//     local_40.vx = local_40.vx >> 4;
//     if (local_40.vy < 0) {
//       local_40.vy = local_40.vy + 0xf;
//     }
//     local_40.vy = local_40.vy >> 4;
//     if (local_40.vz < 0) {
//       local_40.vz = local_40.vz + 0xf;
//     }
//     local_40.vz = local_40.vz >> 4;
//     CamState.TargetVector.vx = CamState.TargetVector.vx + local_40.vx;
//     CamState.TargetVector.vy = CamState.TargetVector.vy + local_40.vy;
//     CamState.TargetVector.vz = CamState.TargetVector.vz + local_40.vz;
//     iVar2 = GetVectorDistance((VECTOR *)((CamState.Owner)->model->locate).coord.t,
//                               &CamState.TargetVector);
//     if ((0 < local_20) || (15000 < iVar2)) {
//       pMVar3 = (CamState.Owner)->model;
//       CamState.TargetVector.vx = (pMVar3->locate).coord.t[0];
//       CamState.TargetVector.vy = (pMVar3->locate).coord.t[1];
//       CamState.TargetVector.vz = (pMVar3->locate).coord.t[2];
//       CamState.TargetVector.pad = *(long *)(pMVar3->locate).workm.m[0];
//     }
//     SetCameraMode(CMODE_LOCK);
//     item->owner->field_0xcd = 0;
//     ppuVar1 = item->proc;
//     if (ppuVar1 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//   }
//   else {
//     ppuVar1 = item->proc;
//     if (ppuVar1 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//   }
//   (*(code *)ppuVar1)(item);
//   DeleteConflict(item->locate);
//   if (item->mode != 0) {
//     AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//   }
//   item->owner = (Humanoid *)0x0;
//   item->proc = (undefined **)0x0;
//   return;
// }
