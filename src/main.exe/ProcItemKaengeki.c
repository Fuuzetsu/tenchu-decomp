#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemKaengeki(struct tag_TItem *item);
 *     ITEM.C:2287, 54 src lines, frame 80 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s3       struct tag_TItem * item
 *     reg   $s1       struct param_kaengeki * param
 *     reg   $s0       struct Humanoid * human
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+16     struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     stack sp+16     struct PARAM_ITEM_LAUNCH rp
 *
 * Globals it touches, as the original declared them:
 *     extern short ActionHalt;
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcItemKaengeki", ProcItemKaengeki);

// triage: HARD — 319 insns, mul/div, indirect-call, 15 callees, ~0.28 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcItemKaengeki(tag_TItem *item)
//
// {
//   byte bVar1;
//   ushort uVar2;
//   uchar uVar3;
//   short sVar4;
//   MotionDataType *pMVar5;
//   VECTOR *pVVar6;
//   undefined **ppuVar7;
//   MotionManager *pMVar8;
//   ModelArchiveType *pMVar9;
//   int iVar10;
//   Humanoid *pHVar11;
//   TItemType TVar12;
//   PARAM_ITEM_USE local_48;
//   int local_20;
//   int local_1c;
//
//   if (item->mode == 0xff) {
//     if (item->owner->motion->mid == 0xf04) {
//       NowReturnNormal(item->owner);
//     }
//     item->mode = '\0';
//     return;
//   }
//   bVar1 = item->mode;
//   if (bVar1 == 1) {
//     pMVar8 = item->owner->motion;
//     if ((pMVar8->count == 0) && (pMVar8->loop != 0)) {
//       SoundEx((VECTOR *)(item->owner->model->locate).coord.t,0x28);
//       item->mode = item->mode + '\x01';
//       (item->param).kaengeki.count = '(';
//     }
//     if (item->owner->motion->mid == 0xf04) {
//       return;
//     }
//     pVVar6 = GetAbsolutePosition(item->locate,0,0,0);
//     pHVar11 = item->owner;
//     TVar12 = item->type;
//     memset((uchar *)&local_48,'\0',0x28);
//     local_48.start.vx = pVVar6->vx;
//     local_48.start.vy = pVVar6->vy;
//     local_48.start.vz = pVVar6->vz;
//     local_48.type = TVar12;
//     local_48.user = pHVar11;
//     iVar10 = rand();
//     local_48.end.vx = iVar10 % 200 + -100;
//     iVar10 = rand();
//     local_48.end.vy = iVar10 % 100 + -200;
//     iVar10 = rand();
//     local_48.end.vz = iVar10 % 200 + -100;
//     ReqItemDrop(&local_48);
//     ppuVar7 = item->proc;
//     if (ppuVar7 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
// LAB_80043bb4:
//     (*(code *)ppuVar7)(item);
//     DeleteConflict(item->locate);
//     if (item->mode != 0) {
//       AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//     }
//     item->owner = (Humanoid *)0x0;
//     item->proc = (undefined **)0x0;
//     return;
//   }
//   if (bVar1 < 2) {
//     if (bVar1 != 0) {
//       return;
//     }
//     pHVar11 = item->owner;
//     if ((ActionHalt == 0) && (0 < pHVar11->life)) {
//       FUN_800270c8(pHVar11,3);
//       UpdateMotion(pHVar11->motion,0xf04);
//       pHVar11->status = 0xf;
//       pMVar5 = pHVar11->motion->motion;
//       MoveHumanoid(pHVar11,(ushort)pMVar5->orderspd,(ushort)pMVar5->sidespd);
//     }
//     Sound(item->owner,0x4c);
//     item->mode = item->mode + '\x01';
//     return;
//   }
//   if (bVar1 != 2) {
//     return;
//   }
//   if ((item->owner->motion->mid != 0xf04) ||
//      (uVar3 = (item->param).kaengeki.count + 0xff, (item->param).kaengeki.count = uVar3,
//      uVar3 == '\0')) {
//     ppuVar7 = item->proc;
//     if (ppuVar7 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//     goto LAB_80043bb4;
//   }
//   pHVar11 = item->owner;
//   uVar2 = (pHVar11->pad).data;
//   if ((uVar2 & 0x2000) == 0) {
//     if ((uVar2 & 0x8000) == 0) goto LAB_80043a4c;
//     pMVar9 = pHVar11->model;
//     sVar4 = (pMVar9->rotate).vy + -0x20;
//   }
//   else {
//     pMVar9 = pHVar11->model;
//     sVar4 = (pMVar9->rotate).vy + 0x20;
//   }
//   (pMVar9->rotate).vy = sVar4;
// LAB_80043a4c:
//   local_48.user = item->owner;
//   local_48.type = ITEM_NAPALM;
//   local_48.end.vx = (item->param).kaengeki.end.vx;
//   local_48.end.vy = (item->param).kaengeki.end.vy;
//   local_48.end.vz = (item->param).kaengeki.end.vz;
//   pMVar9 = item->owner->model;
//   if (((CamState.Owner)->model == pMVar9) && (CamState.Mode == CMODE_DIRECTION)) {
//     GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_20,&local_1c);
//     iVar10 = 0;
//   }
//   else {
//     local_20 = (int)(pMVar9->rotate).vx;
//     iVar10 = (int)(pMVar9->rotate).vz;
//     local_1c = (int)(pMVar9->rotate).vy;
//   }
//   RotateVector(&local_48.end,local_20,local_1c,iVar10);
//   local_48.start.vx = (long)(&((item->param).smoke.koro.hint)->y + local_48.end.vx * 6);
//   local_48.start.vy = local_48.end.vy * 0xc + *(int *)&(item->param).smoke.koro.vx;
//   local_48.end.vx = local_48.start.vx + local_48.end.vx * 2;
//   local_48.end.vy = local_48.end.vy * 2 + local_48.start.vy;
//   local_48.start.vz = local_48.end.vz * 0xc + *(int *)&(item->param).smoke.koro.vz;
//   local_48.end.vz = local_48.end.vz * 2 + local_48.start.vz;
//   ReqItemUse(&local_48);
//   return;
// }
