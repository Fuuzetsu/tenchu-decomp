#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemJirai(struct tag_TItem *item);
 *     ITEM.C:3527, 78 src lines, frame 104 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s2       struct tag_TItem * item
 *     reg   $s6       struct Sprite3D * model
 *     reg   $v1       struct param_smoke * param
 *     reg   $s2       struct tag_TItem * item
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s4       struct Humanoid * human
 *     reg   $s4       struct Humanoid * human
 *     reg   $s3       int i
 *     reg   $s0       struct ModelType * model
 *     stack sp+24     struct VECTOR pos
 *     stack sp+24     struct SVECTOR vec
 *     stack sp+32     struct VECTOR pos
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s2       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct AreaNodeType *FieldArea;
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcItemJirai", ProcItemJirai);

// triage: HARD — 427 insns, mul/div, 1 loop, indirect-call, 16 callees, ~0.27 to ProcItemDrop
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Type propagation algorithm not settling */
//
// void ProcItemJirai(tag_TItem *item)
//
// {
//   byte bVar1;
//   bool bVar2;
//   uchar uVar3;
//   short sVar4;
//   long lVar5;
//   undefined **ppuVar6;
//   Sprite3D *pSVar7;
//   int iVar8;
//   ModelType *pMVar9;
//   int iVar10;
//   SVECTOR *pSVar11;
//   undefined4 uVar12;
//   undefined4 uVar13;
//   undefined4 uVar14;
//   undefined4 *puVar15;
//   _GsCOORDINATE2 *super;
//   VECTOR *pos;
//   Sprite3D *sprt;
//   int iVar16;
//   undefined *puVar17;
//   undefined1 local_48 [12];
//   long local_3c;
//   int local_38;
//   int local_34;
//   int local_30;
//   long local_2c;
//   int local_28;
//   int local_24;
//
//   sprt = (Sprite3D *)item->model;
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//     return;
//   }
//   bVar1 = item->mode;
//   if (bVar1 == 1) {
//     iVar10 = -1;
//     if (((int)item->locate->attribute & 0x8000U) != 0) {
//       sVar4 = GetConflictResult(item->locate,-1);
//       iVar10 = (int)sVar4;
//     }
//     if ((iVar10 != -1) && (iVar10 = FUN_800294dc(ConflictObject[iVar10].common), iVar10 != 0)) {
//       DeleteConflict(item->locate);
//       sVar4 = InsertConflict(item->locate);
//       ConflictObject[sVar4].offset.vx = 0;
//       ConflictObject[sVar4].offset.vz = 0;
//       ConflictObject[sVar4].offset.vy = 0;
//       ConflictObject[sVar4].size.vz = 0x5dc;
//       ConflictObject[sVar4].size.vy = 0x5dc;
//       ConflictObject[sVar4].size.vx = 0x5dc;
//       ConflictObject[sVar4].common = (undefined *)0x1;
//       ConflictObject[sVar4].size.pad = 1;
//       uVar3 = item->mode;
//       (item->collision).size = 0x5dc;
//       (item->collision).ofsY = 0;
//       (item->collision).mode = 1;
//       (item->collision).pause = 0;
//       item->mode = uVar3 + '\x01';
//     }
//   }
//   else if (bVar1 < 2) {
//     if (bVar1 == 0) {
//       lVar5 = GetAreaMapLevel(GlobalAreaMap,(item->locate->locate).coord.t[0],
//                               (item->locate->locate).coord.t[1]);
//       (item->locate->locate).coord.t[1] = lVar5;
//       if (((item->locate->locate).coord.t[1] == -0x80000000) || ((FieldArea->attribute & 4U) != 0))
//       {
//         uVar3 = item->owner->item[item->type];
//         if (uVar3 != 0xff) {
//           item->owner->item[item->type] = uVar3 + '\x01';
//         }
//         ppuVar6 = item->proc;
//         if (ppuVar6 == (undefined **)0x0) {
//           return;
//         }
//         item->mode = 0xff;
// LAB_800488a4:
//         (*(code *)ppuVar6)(item);
//         DeleteConflict(item->locate);
//         if (item->mode != 0) {
//           AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//         }
//         item->owner = (Humanoid *)0x0;
//         item->proc = (undefined **)0x0;
//         return;
//       }
//       DeleteConflict(item->locate);
//       sVar4 = InsertConflict(item->locate);
//       ConflictObject[sVar4].offset.vx = 0;
//       ConflictObject[sVar4].offset.vz = 0;
//       ConflictObject[sVar4].offset.vy = 0;
//       ConflictObject[sVar4].size.vz = 500;
//       ConflictObject[sVar4].size.vy = 500;
//       ConflictObject[sVar4].size.vx = 500;
//       ConflictObject[sVar4].common = (undefined *)0x1;
//       ConflictObject[sVar4].size.pad = 8;
//       uVar3 = item->mode;
//       (item->collision).size = 500;
//       (item->collision).ofsY = 0;
//       (item->collision).mode = 8;
//       (item->collision).pause = 0;
//       item->mode = uVar3 + '\x01';
//     }
//   }
//   else if (bVar1 == 2) {
//     local_48._0_4_ = DAT_80097ae4;
//     local_48._4_4_ = DAT_80097ae8;
//     memset((uchar *)&local_30,'\0',0x10);
//     local_48._8_4_ = (item->locate->locate).coord.t[0];
//     pos = (VECTOR *)(local_48 + 8);
//     local_3c = (item->locate->locate).coord.t[1];
//     local_38 = (item->locate->locate).coord.t[2];
//     local_34 = local_24;
//     local_30 = local_48._8_4_;
//     local_2c = local_3c;
//     local_28 = local_38;
//     SetExplosion(pos,(SVECTOR *)local_48);
//     local_48._0_4_ = 0xc8004b;
//     local_48._4_2_ = 0x4b;
//     SetHinoko(pos,(SVECTOR *)local_48,10);
//     local_48._0_4_ = -0x1900000;
//     local_48._4_4_ = (uint)(ushort)local_48._6_2_ << 0x10;
//     SetSmoke(pos,(SVECTOR *)local_48,0x14,6);
//     SoundEx(pos,0x25);
//     item->mode = item->mode + '\x01';
//     (item->param).smoke.count = '\x03';
//     FUN_8002f7f4();
//   }
//   else if (bVar1 == 3) {
//     if (((int)item->locate->attribute & 0x8000U) == 0) {
//       iVar10 = -1;
//     }
//     else {
//       sVar4 = GetConflictResult(item->locate,-1);
//       iVar10 = (int)sVar4;
//     }
//     if (iVar10 != -1) {
//       puVar17 = ConflictObject[iVar10].common;
//       iVar10 = FUN_800294dc(puVar17);
//       iVar16 = 0;
//       if (iVar10 != 0) {
//         bVar2 = true;
//         while (bVar2) {
//           puVar15 = *(undefined4 **)(*(int *)(puVar17 + 0x58) + 0x68);
//           if (0 < *(short *)(*(int *)(puVar17 + 0x58) + 100)) {
//             iVar10 = rand();
//             iVar8 = (int)*(short *)(*(int *)(puVar17 + 0x58) + 100);
//             if (iVar8 == 0) {
//               trap(0x1c00);
//             }
//             if ((iVar8 == -1) && (iVar10 == -0x80000000)) {
//               trap(0x1800);
//             }
//             puVar15 = puVar15 + iVar10 % iVar8;
//           }
//           super = (_GsCOORDINATE2 *)*puVar15;
//           memset((uchar *)&local_38,'\0',0x10);
//           iVar16 = iVar16 + 1;
//           iVar10 = rand();
//           local_38 = iVar10 % 200 + -100;
//           iVar10 = rand();
//           local_34 = iVar10 % 200 + -100;
//           iVar10 = rand();
//           local_48._8_4_ = iVar10 % 200 + -100;
//           local_48._0_4_ = local_38;
//           local_48._4_4_ = local_34;
//           local_3c = local_2c;
//           local_30 = local_48._8_4_;
//           iVar10 = rand();
//           SetFrame((VECTOR *)local_48,0x3000,
//                    (short)((uint)((iVar10 % 0x3c + 0x3c) * 0x10000) >> 0x10),super);
//           bVar2 = iVar16 < 10;
//         }
//       }
//     }
//     uVar3 = (item->param).smoke.count + 0xff;
//     (item->param).smoke.count = uVar3;
//     if (uVar3 != 0xff) {
//       return;
//     }
//     ppuVar6 = item->proc;
//     if (ppuVar6 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//     goto LAB_800488a4;
//   }
//   UpdateCoordinate(item->locate);
//   pMVar9 = item->locate;
//   pSVar11 = &pMVar9->rotate;
//   pSVar7 = sprt;
//   do {
//     uVar13 = *(undefined4 *)(pMVar9->locate).coord.m[0];
//     uVar14 = *(undefined4 *)((pMVar9->locate).coord.m[0] + 2);
//     uVar12 = *(undefined4 *)((pMVar9->locate).coord.m[1] + 1);
//     (pSVar7->locate).flg = (pMVar9->locate).flg;
//     *(undefined4 *)(pSVar7->locate).coord.m[0] = uVar13;
//     *(undefined4 *)((pSVar7->locate).coord.m[0] + 2) = uVar14;
//     *(undefined4 *)((pSVar7->locate).coord.m[1] + 1) = uVar12;
//     pMVar9 = (ModelType *)((pMVar9->locate).coord.m + 2);
//     pSVar7 = (Sprite3D *)((pSVar7->locate).coord.m + 2);
//   } while (pMVar9 != (ModelType *)pSVar11);
//   DrawSprite(sprt);
//   return;
// }
