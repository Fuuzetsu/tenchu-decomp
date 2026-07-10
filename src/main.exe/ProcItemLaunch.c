#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemLaunch(struct tag_TItem *item);
 *     ITEM.C:3089, 83 src lines, frame 128 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s0       struct tag_TItem * item
 *     reg   $s2       struct ModelType * model
 *     reg   $s1       struct param_launch * param
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int t
 *     stack sp+24     struct PARAM_ITEM_STAY rparam
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v1       struct PARAM_ITEM_STAY * p
 *     stack sp+72     struct PARAM_ITEM_LAUNCH param
 *     reg   $s0       struct tag_TItem * item
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s1       struct Humanoid * m
 *     reg   $s1       struct Humanoid * human
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern long GameClock;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcItemLaunch", ProcItemLaunch);

// triage: MEDIUM — 253 insns, 2 loop, indirect-call, 16 callees, ~0.56 to ProcItemHappou
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcItemLaunch(tag_TItem *item)
//
// {
//   byte bVar1;
//   long lVar2;
//   uchar uVar3;
//   short sVar4;
//   ModelType *pMVar5;
//   ModelType *pMVar6;
//   PARAM_ITEM_USE *pPVar7;
//   PARAM_ITEM_USE *pPVar8;
//   SVECTOR *pSVar9;
//   int iVar10;
//   undefined4 uVar11;
//   Humanoid *pHVar12;
//   undefined4 uVar13;
//   TItemType TVar14;
//   undefined4 uVar15;
//   TItemType TVar16;
//   TItemType *pTVar17;
//   TItemType *pTVar18;
//   ModelType *objp;
//   PARAM_ITEM_USE local_60;
//   TItemType local_38;
//   Humanoid *local_34;
//   long local_30;
//   long local_2c;
//   long local_28;
//   TItemType aTStack_18 [2];
//
//   objp = item->model;
//   if (item->mode == 0xff) {
//     DisposeAfterimage((item->param).launch.effect);
//     item->mode = '\0';
//     return;
//   }
//   MoveFly((int *)item,(int *)&(item->param).henshin);
//   uVar3 = (item->param).launch.count + 0xff;
//   (item->param).launch.count = uVar3;
//   if (uVar3 == '\0') {
//     DeleteConflict(item->locate);
//     sVar4 = InsertConflict(item->locate);
//     ConflictObject[sVar4].offset.vx = 0;
//     ConflictObject[sVar4].offset.vz = 0;
//     ConflictObject[sVar4].offset.vy = 0;
//     ConflictObject[sVar4].size.vz = 300;
//     ConflictObject[sVar4].size.vy = 300;
//     ConflictObject[sVar4].size.vx = 300;
//     ConflictObject[sVar4].common = (undefined *)0x1;
//     ConflictObject[sVar4].size.pad = 1;
//     (item->collision).size = 300;
//     (item->collision).ofsY = 0;
//     (item->collision).mode = 1;
//     (item->collision).pause = 0;
//   }
//   lVar2 = GameClock;
//   (item->locate->rotate).vx = 0;
//   (item->locate->rotate).vy = (short)lVar2 * 0x2aa;
//   (item->locate->rotate).vz = 0;
//   UpdateCoordinate(item->locate);
//   pMVar5 = item->locate;
//   pSVar9 = &pMVar5->rotate;
//   pMVar6 = objp;
//   do {
//     uVar11 = *(undefined4 *)(pMVar5->locate).coord.m[0];
//     uVar13 = *(undefined4 *)((pMVar5->locate).coord.m[0] + 2);
//     uVar15 = *(undefined4 *)((pMVar5->locate).coord.m[1] + 1);
//     (pMVar6->locate).flg = (pMVar5->locate).flg;
//     *(undefined4 *)(pMVar6->locate).coord.m[0] = uVar11;
//     *(undefined4 *)((pMVar6->locate).coord.m[0] + 2) = uVar13;
//     *(undefined4 *)((pMVar6->locate).coord.m[1] + 1) = uVar15;
//     pMVar5 = (ModelType *)((pMVar5->locate).coord.m + 2);
//     pMVar6 = (ModelType *)((pMVar6->locate).coord.m + 2);
//   } while (pMVar5 != (ModelType *)pSVar9);
//   DrawModel(objp);
//   DrawAfterimage((item->param).launch.effect,1);
//   if (((int)item->locate->attribute & 0x8000U) == 0) {
//     iVar10 = -1;
//   }
//   else {
//     sVar4 = GetConflictResult(item->locate,-1);
//     iVar10 = (int)sVar4;
//   }
//   if ((iVar10 == -1) || (iVar10 = FUN_800294dc(ConflictObject[iVar10].common), iVar10 == 0)) {
//     if (*(char *)((int)&item->param + 0x28) == '\0') {
//       return;
//     }
//     bVar1 = *(byte *)((int)&item->param + 10);
//     if (bVar1 != 2) {
//       if (bVar1 < 3) {
//         if (bVar1 != 1) {
//           return;
//         }
//         goto LAB_800473d4;
//       }
//       if (bVar1 == 3) {
//         SetBleeds((short)item->locate + 0x18,0,0x19);
//         SoundEx((VECTOR *)(item->locate->locate).coord.t,0x31);
//         FUN_8002f7f4();
//         return;
//       }
//       if (bVar1 != 4) {
//         return;
//       }
//     }
//     memset((uchar *)&local_38,'\0',0x28);
//     local_38 = item->type;
//     local_34 = item->owner;
//     local_30 = (objp->locate).coord.t[0];
//     local_2c = (objp->locate).coord.t[1];
//     local_28 = (objp->locate).coord.t[2];
//     pPVar8 = &local_60;
//     pTVar18 = &local_38;
//     do {
//       pTVar17 = pTVar18;
//       pPVar7 = pPVar8;
//       pHVar12 = (Humanoid *)pTVar17[1];
//       TVar14 = pTVar17[2];
//       TVar16 = pTVar17[3];
//       pPVar7->type = *pTVar17;
//       pPVar7->user = pHVar12;
//       (pPVar7->start).vx = TVar14;
//       (pPVar7->start).vy = TVar16;
//       pTVar18 = pTVar17 + 4;
//       pPVar8 = (PARAM_ITEM_USE *)&(pPVar7->start).vz;
//     } while (pTVar18 != aTStack_18);
//     TVar14 = pTVar17[5];
//     *(TItemType *)pPVar8 = *pTVar18;
//     (pPVar7->start).pad = TVar14;
//     if (item->proc != (undefined **)0x0) {
//       item->mode = 0xff;
//       (*(code *)item->proc)(item);
//       DeleteConflict(item->locate);
//       if (item->mode != 0) {
//         AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//       }
//       item->owner = (Humanoid *)0x0;
//       item->proc = (undefined **)0x0;
//     }
//     ReqItemDrop(&local_60);
//   }
//   else {
//     SetImpact((VECTOR *)(item->locate->locate).coord.t,0x4000,2);
//     SoundEx((VECTOR *)(item->locate->locate).coord.t,0x30);
// LAB_800473d4:
//     if (item->proc != (undefined **)0x0) {
//       item->mode = 0xff;
//       (*(code *)item->proc)(item);
//       DeleteConflict(item->locate);
//       if (item->mode != 0) {
//         AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//       }
//       item->owner = (Humanoid *)0x0;
//       item->proc = (undefined **)0x0;
//     }
//   }
//   return;
// }
