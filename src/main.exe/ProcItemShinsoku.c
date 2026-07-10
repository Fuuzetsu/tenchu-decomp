#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemShinsoku(struct tag_TItem *item);
 *     ITEM.C:1324, 55 src lines, frame 104 bytes, saved-reg mask 0x801f0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $s3       struct tag_TItem * item
 *     reg   $s1       struct param_shinsoku * param
 *     stack sp+24     struct VECTOR pos
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+40     struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s0       struct VECTOR * apos
 *     stack sp+56     struct MapVector map
 *     stack sp+40     struct VECTOR pos
 *     reg   $s3       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcItemShinsoku", ProcItemShinsoku);

// triage: HARD — 336 insns, mul/div, indirect-call, 14 callees, ~0.27 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcItemShinsoku(tag_TItem *item)
//
// {
//   byte bVar1;
//   ushort uVar2;
//   bool bVar3;
//   uchar uVar4;
//   VECTOR *pVVar5;
//   undefined **ppuVar6;
//   ModelArchiveType *pMVar7;
//   MotionManager *pMVar8;
//   int iVar9;
//   Humanoid *pHVar10;
//   TItemType TVar11;
//   int local_4c;
//   PARAM_ITEM_USE local_40;
//
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//     return;
//   }
//   bVar1 = item->mode;
//   if (bVar1 == 1) {
//     pMVar8 = item->owner->motion;
//     if (pMVar8->mid == 0xf05) {
//       if (pMVar8->count != 0) {
//         return;
//       }
//       if (pMVar8->loop == 0) {
//         return;
//       }
//       FUN_80033bc0(item->owner->locate,0x96,0xc,8);
//       (item->param).napalm.count = 'K';
//       goto LAB_8003fab0;
//     }
//     pVVar5 = GetAbsolutePosition(item->locate,0,0,0);
//     pHVar10 = item->owner;
//     TVar11 = item->type;
//     memset((uchar *)&local_40,'\0',0x28);
//     local_40.start.vx = pVVar5->vx;
//     local_40.start.vy = pVVar5->vy;
//     local_40.start.vz = pVVar5->vz;
//     local_40.type = TVar11;
//     local_40.user = pHVar10;
//     iVar9 = rand();
//     local_40.end.vx = iVar9 % 200 + -100;
//     iVar9 = rand();
//     local_40.end.vy = iVar9 % 100 + -200;
//     iVar9 = rand();
//     local_40.end.vz = iVar9 % 200 + -100;
//     ReqItemDrop(&local_40);
//     ppuVar6 = item->proc;
//     if (ppuVar6 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//     goto LAB_8003fd84;
//   }
//   if (bVar1 < 2) {
//     if (bVar1 != 0) {
//       return;
//     }
//     SetNowMotion(item->owner,0xf05,1);
//     Sound(item->owner,0x4c);
// LAB_8003fab0:
//     item->mode = item->mode + '\x01';
//     return;
//   }
//   if (bVar1 != 2) {
//     return;
//   }
//   if (item->owner->motion->mid != 0xf05) {
//     ppuVar6 = item->proc;
//     if (ppuVar6 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//     goto LAB_8003fd84;
//   }
//   TVar11 = (item->owner->model->locate).coord.t[0] + (int)(item->param).napalm.vec.vx;
//   local_4c = (item->owner->model->locate).coord.t[1] + (int)(item->param).napalm.vec.vy;
//   iVar9 = (item->owner->model->locate).coord.t[2] + (int)(item->param).napalm.vec.vz;
//   local_40.user = (Humanoid *)(local_4c + -2000);
//   local_40.type = TVar11;
//   local_40.start.vx = iVar9;
//   GetAreaMapVector((MapVector *)GlobalAreaMap,(VECTOR *)&local_40.start.vz,(long)&local_40);
//   bVar3 = false;
//   if ((local_4c + -500 <= local_40.start.vz) && (bVar3 = true, local_40.start.vz < local_4c)) {
//     local_4c = local_40.start.vz;
//   }
//   if (bVar3) {
//     (item->owner->model->locate).coord.t[0] = TVar11;
//     (item->owner->model->locate).coord.t[1] = local_4c;
//     (item->owner->model->locate).coord.t[2] = iVar9;
//   }
//   if (((item->param).napalm.count & 3) == 0) {
//     pMVar7 = item->owner->model;
//     local_40.type = (pMVar7->locate).coord.t[0];
//     local_40.start.vx = (pMVar7->locate).coord.t[2];
//     local_40.start.vy = *(long *)(pMVar7->locate).workm.m[0];
//     local_40.user = (Humanoid *)((pMVar7->locate).coord.t[1] + -300);
//     FUN_8003944c(&local_40,0,0x2000,0x5000,0x808080,0,0,0xffffffe2,0x10,3);
//   }
//   if (CamState.Owner == item->owner) {
//     SetCameraMode(CMODE_CROUCH);
//   }
//   pHVar10 = item->owner;
//   uVar2 = (pHVar10->pad).data;
//   if ((uVar2 & 0x2000) == 0) {
//     if ((uVar2 & 0x8000) != 0) {
//       pMVar7 = pHVar10->model;
//       iVar9 = -0x40;
//       goto LAB_8003fd0c;
//     }
//   }
//   else {
//     pMVar7 = pHVar10->model;
//     iVar9 = 0x40;
// LAB_8003fd0c:
//     (pMVar7->rotate).vy = (pMVar7->rotate).vy + (short)iVar9;
//     RotateVectorS((SVECTOR *)&(item->param).launch,0,iVar9,0);
//   }
//   uVar4 = (item->param).napalm.count + 0xff;
//   (item->param).napalm.count = uVar4;
//   if ((uVar4 != '\0') && (((item->owner->pad).trig & 0xf0) == 0)) {
//     return;
//   }
//   NowReturnNormal(item->owner);
//   SetCameraMode(CMODE_NORMAL);
//   ppuVar6 = item->proc;
//   if (ppuVar6 == (undefined **)0x0) {
//     return;
//   }
//   item->mode = 0xff;
// LAB_8003fd84:
//   (*(code *)ppuVar6)(item);
//   DeleteConflict(item->locate);
//   if (item->mode != 0) {
//     AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//   }
//   item->owner = (Humanoid *)0x0;
//   item->proc = (undefined **)0x0;
//   return;
// }
