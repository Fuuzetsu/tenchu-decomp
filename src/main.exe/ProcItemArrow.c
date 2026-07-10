#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemArrow(struct tag_TItem *item);
 *     ITEM.C:3367, 118 src lines, frame 168 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s1       struct tag_TItem * item
 *     reg   $s4       struct ModelType * model
 *     reg   $s3       struct param_arrow * param
 *     stack sp+24     struct VECTOR v1
 *     stack sp+40     struct VECTOR v2
 *     stack sp+136    int rx
 *     stack sp+140    int ry
 *     reg   $a0       int cid
 *     reg   $s1       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       struct ModelType * model
 *     reg   $s2       struct Humanoid * m
 *     reg   $s2       struct Humanoid * human
 *     stack sp+56     struct PARAM_ITEM_LAUNCH param
 *     reg   $s1       struct tag_TItem * item
 *     reg   $s0       struct ModelType * model
 *     reg   $s1       struct tag_TItem * item
 *     reg   $s1       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern long GameClock;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcItemArrow", ProcItemArrow);

// triage: HARD — 332 insns, mul/div, 1 loop, indirect-call, 15 callees, ~0.28 to ProcItemHappou
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcItemArrow(tag_TItem *item)
//
// {
//   char cVar1;
//   uchar uVar2;
//   short sVar3;
//   int iVar4;
//   undefined **ppuVar5;
//   VECTOR *pVVar6;
//   byte bVar7;
//   SVECTOR *pSVar8;
//   int iVar9;
//   ModelType *pMVar10;
//   undefined4 uVar11;
//   undefined4 uVar12;
//   undefined4 uVar13;
//   undefined4 *puVar14;
//   ModelType *pMVar15;
//   undefined *puVar16;
//   ModelType *objp;
//   VECTOR local_40;
//   VECTOR local_30;
//   short local_20 [2];
//   short local_1c [2];
//
//   objp = item->model;
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//     return;
//   }
//   bVar7 = item->mode;
//   if (bVar7 == 1) {
//     uVar2 = (item->param).arrow.count + 0xff;
//     (item->param).arrow.count = uVar2;
//     if (uVar2 == '\0') {
//       (item->param).arrow.count = '\x0f';
//       item->mode = item->mode + '\x01';
//     }
//   }
//   else if (bVar7 < 2) {
//     if (bVar7 == 0) {
//       local_40.vx = (item->locate->locate).coord.t[0];
//       local_40.vy = (item->locate->locate).coord.t[1];
//       local_40.vz = (item->locate->locate).coord.t[2];
//       MoveFly((int *)item,(int *)&(item->param).henshin);
//       uVar2 = (item->param).arrow.count + 0xff;
//       (item->param).arrow.count = uVar2;
//       if (uVar2 == '\0') {
//         DeleteConflict(item->locate);
//         sVar3 = InsertConflict(item->locate);
//         ConflictObject[sVar3].offset.vx = 0;
//         ConflictObject[sVar3].offset.vz = 0;
//         ConflictObject[sVar3].offset.vy = 0;
//         ConflictObject[sVar3].size.vz = 300;
//         ConflictObject[sVar3].size.vy = 300;
//         ConflictObject[sVar3].size.vx = 300;
//         ConflictObject[sVar3].common = (undefined *)0x1;
//         ConflictObject[sVar3].size.pad = 1;
//         (item->collision).size = 300;
//         (item->collision).ofsY = 0;
//         (item->collision).mode = 1;
//         (item->collision).pause = 0;
//       }
//       if (((int)item->locate->attribute & 0x8000U) == 0) {
//         iVar9 = -1;
//       }
//       else {
//         sVar3 = GetConflictResult(item->locate,-1);
//         iVar9 = (int)sVar3;
//       }
//       if (iVar9 == -1) {
//         if ((*(char *)((int)&item->param + 0x28) != '\0') &&
//            (cVar1 = *(char *)((int)&item->param + 10), cVar1 != '\0')) {
//           if (cVar1 != '\x01') {
//             SoundEx((VECTOR *)(item->locate->locate).coord.t,0x31);
//             SetBleeds((short)item->locate + 0x18,0,0x19);
//             (item->param).arrow.count = '\x1e';
//             item->mode = item->mode + '\x01';
//             DeleteConflict(item->locate);
//             return;
//           }
//           ppuVar5 = item->proc;
//           if (ppuVar5 == (undefined **)0x0) {
//             return;
//           }
//           item->mode = 0xff;
// LAB_80048018:
//           (*(code *)ppuVar5)(item);
//           DeleteConflict(item->locate);
//           if (item->mode != 0) {
//             AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//           }
//           item->owner = (Humanoid *)0x0;
//           item->proc = (undefined **)0x0;
//           return;
//         }
//       }
//       else {
//         puVar16 = ConflictObject[iVar9].common;
//         iVar4 = FUN_800294dc(puVar16);
//         if (iVar4 != 0) {
//           if ((ConflictObject[iVar9].size.pad & 1U) != 0) {
//             ppuVar5 = item->proc;
//             if (ppuVar5 == (undefined **)0x0) {
//               return;
//             }
//             item->mode = 0xff;
//             goto LAB_80048018;
//           }
//           puVar14 = *(undefined4 **)(*(int *)(puVar16 + 0x58) + 0x68);
//           if (0 < *(short *)(*(int *)(puVar16 + 0x58) + 100)) {
//             iVar9 = rand();
//             iVar4 = (int)*(short *)(*(int *)(puVar16 + 0x58) + 100);
//             if (iVar4 == 0) {
//               trap(0x1c00);
//             }
//             if ((iVar4 == -1) && (iVar9 == -0x80000000)) {
//               trap(0x1800);
//             }
//             puVar14 = puVar14 + iVar9 % iVar4;
//           }
//           pMVar15 = (ModelType *)*puVar14;
//           pVVar6 = GetAbsolutePosition(pMVar15,0,0,0);
//           SetImpact(pVVar6,0x6000,2);
//           pVVar6 = GetAbsolutePosition(pMVar15,0,0,0);
//           SoundEx(pVVar6,0x30);
//           ArrangeLocalMatrix(pMVar15,&(item->locate->locate).coord);
//           (item->locate->locate).flg = 0;
//           (item->locate->locate).super = (_GsCOORDINATE2 *)pMVar15;
//           (item->locate->locate).coord.t[0] = 0;
//           (item->locate->locate).coord.t[1] = 0;
//           (item->locate->locate).coord.t[2] = 0;
//           (item->param).arrow.count = 'x';
//           item->mode = item->mode + '\x01';
//           DeleteConflict(item->locate);
//           goto LAB_80048064;
//         }
//       }
//       local_30.vx = (item->locate->locate).coord.t[0];
//       local_30.vy = (item->locate->locate).coord.t[1];
//       local_30.vz = (item->locate->locate).coord.t[2];
//       GetVectorRotation(&local_40,&local_30,(int *)local_20,(int *)local_1c);
//       (item->locate->rotate).vx = local_20[0];
//       (item->locate->rotate).vy = local_1c[0];
//       (item->locate->rotate).vz = (short)(GameClock << 8);
//       UpdateCoordinate(item->locate);
//     }
//   }
//   else if (bVar7 == 2) {
//     uVar2 = (item->param).arrow.count;
//     bVar7 = uVar2 - 1;
//     (item->param).arrow.count = bVar7;
//     if (uVar2 == '\x01') {
//       ppuVar5 = item->proc;
//       if (ppuVar5 == (undefined **)0x0) {
//         return;
//       }
//       item->mode = 0xff;
//       goto LAB_80048018;
//     }
//     if ((bVar7 & 1) != 0) {
//       return;
//     }
//   }
// LAB_80048064:
//   pMVar10 = item->locate;
//   pSVar8 = &pMVar10->rotate;
//   pMVar15 = objp;
//   do {
//     uVar11 = *(undefined4 *)(pMVar10->locate).coord.m[0];
//     uVar12 = *(undefined4 *)((pMVar10->locate).coord.m[0] + 2);
//     uVar13 = *(undefined4 *)((pMVar10->locate).coord.m[1] + 1);
//     (pMVar15->locate).flg = (pMVar10->locate).flg;
//     *(undefined4 *)(pMVar15->locate).coord.m[0] = uVar11;
//     *(undefined4 *)((pMVar15->locate).coord.m[0] + 2) = uVar12;
//     *(undefined4 *)((pMVar15->locate).coord.m[1] + 1) = uVar13;
//     pMVar10 = (ModelType *)((pMVar10->locate).coord.m + 2);
//     pMVar15 = (ModelType *)((pMVar15->locate).coord.m + 2);
//   } while (pMVar10 != (ModelType *)pSVar8);
//   DrawModel(objp);
//   return;
// }
