#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemGosin(struct tag_TItem *item);
 *     ITEM.C:1804, 52 src lines, frame 88 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+24     struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     stack sp+24     struct VECTOR v
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcItemGosin", ProcItemGosin);

// triage: MEDIUM — 238 insns, mul/div, indirect-call, 11 callees, ~0.21 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcItemGosin(tag_TItem *item)
//
// {
//   byte bVar1;
//   uchar uVar2;
//   VECTOR *pVVar3;
//   int iVar4;
//   ushort uVar5;
//   MotionManager *pMVar6;
//   TItemType TVar7;
//   Humanoid *pHVar8;
//   PARAM_ITEM_USE local_40;
//
//   if (item->mode == 0xff) {
//     item->owner->itmctl = 0;
//     item->mode = '\0';
//     return;
//   }
//   bVar1 = item->mode;
//   if (bVar1 == 1) {
//     pMVar6 = item->owner->motion;
//     if (pMVar6->mid != 0xf04) {
//       pVVar3 = GetAbsolutePosition(item->locate,0,0,0);
//       pHVar8 = item->owner;
//       TVar7 = item->type;
//       memset((uchar *)&local_40,'\0',0x28);
//       local_40.start.vx = pVVar3->vx;
//       local_40.start.vy = pVVar3->vy;
//       local_40.start.vz = pVVar3->vz;
//       local_40.type = TVar7;
//       local_40.user = pHVar8;
//       iVar4 = rand();
//       local_40.end.vx = iVar4 % 200 + -100;
//       iVar4 = rand();
//       local_40.end.vy = iVar4 % 100 + -200;
//       iVar4 = rand();
//       local_40.end.vz = iVar4 % 200 + -100;
//       ReqItemDrop(&local_40);
//       goto LAB_80041e80;
//     }
//     if (pMVar6->count != 0) {
//       return;
//     }
//     if (pMVar6->loop == 0) {
//       return;
//     }
//     NowReturnNormal(item->owner);
//     pVVar3 = GetAbsolutePosition(item->owner->model->object[1],0,0,0);
//     SetBleeds((short)pVVar3,600,100);
//     item->owner->itmctl = (short)item->type;
//     uVar2 = item->mode;
//     (item->param).napalm.vec.vx = 0x1c2;
//   }
//   else {
//     if (1 < bVar1) {
//       if (bVar1 != 2) {
//         return;
//       }
//       uVar5 = (item->param).napalm.vec.vx - 1;
//       (item->param).napalm.vec.vx = uVar5;
//       if (uVar5 != 0) {
//         if ((uVar5 & 0x3f) != 0) {
//           return;
//         }
//         local_40.type = DAT_80012248;
//         local_40.user = DAT_8001224c;
//         local_40.start.vx = DAT_80012250;
//         local_40.start.vy = DAT_80012254;
//         iVar4 = rand();
//         FUN_8003944c(&local_40,item->owner->model,0x1000,0x6000,0x808080,0,
//                      (iVar4 % 0x168) * 0x10000 >> 0x10,2,0x78,4);
//         return;
//       }
// LAB_80041e80:
//       if (item->proc == (undefined **)0x0) {
//         return;
//       }
//       item->mode = 0xff;
//       (*(code *)item->proc)(item);
//       DeleteConflict(item->locate);
//       if (item->mode != 0) {
//         AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//       }
//       item->owner = (Humanoid *)0x0;
//       item->proc = (undefined **)0x0;
//       return;
//     }
//     if (bVar1 != 0) {
//       return;
//     }
//     SetNowMotion(item->owner,0xf04,1);
//     Sound(item->owner,0x4c);
//     uVar2 = item->mode;
//   }
//   item->mode = uVar2 + '\x01';
//   return;
// }
