#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemFire(struct tag_TItem *item);
 *     ITEM.C:2586, 116 src lines, frame 176 bytes, saved-reg mask 0x803f0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $s2       struct tag_TItem * item
 *     reg   $s4       struct Sprite3D * model
 *     reg   $s5       struct param_smoke * param
 *     reg   $s2       struct tag_TItem * item
 *     stack sp+24     struct VECTOR pos
 *     stack sp+40     struct SVECTOR vec
 *     stack sp+56     struct PARAM_ITEM_STAY rparam
 *     reg   $s2       struct tag_TItem * item
 *     stack sp+104    struct PARAM_ITEM_LAUNCH param
 *     reg   $a0       int cid
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       struct ModelType * model
 *     reg   $v0       struct Humanoid * human
 *     stack sp+48     struct SVECTOR vec
 *     stack sp+80     struct VECTOR pos
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s2       struct tag_TItem * item
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s0       struct Humanoid * human
 *     reg   $s0       struct Humanoid * human
 *     reg   $s1       struct ModelType * model
 *     stack sp+96     struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcItemFire", ProcItemFire);

// triage: HARD — 583 insns, mul/div, 1 loop, indirect-call, 20 callees, ~0.22 to ProcItemHappou
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
// void ProcItemFire(tag_TItem *item)
//
// {
//   byte bVar1;
//   short sVar2;
//   ModelType *pMVar3;
//   int iVar4;
//   int iVar5;
//   uchar uVar6;
//   Sprite3D *pSVar7;
//   SVECTOR *pSVar8;
//   undefined4 uVar9;
//   undefined4 uVar10;
//   undefined4 uVar11;
//   VECTOR *pos;
//   undefined *puVar12;
//   undefined4 *puVar13;
//   _GsCOORDINATE2 *super;
//   Sprite3D *sprt;
//   undefined1 local_80 [12];
//   long local_74;
//   SVECTOR local_70;
//   int local_68;
//   long local_64;
//   TItemType local_60;
//   long local_5c;
//   long local_58;
//   long local_54;
//   SVECTOR local_50;
//   PARAM_ITEM_USE local_48;
//
//   sprt = (Sprite3D *)item->model;
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//   }
//   else {
//     MoveKorogari(item,(param_korogari *)&(item->param).launch);
//     if (*(char *)((int)&item->param + 10) == '\x01') {
//       if (item->proc != (undefined **)0x0) {
//         item->mode = 0xff;
//         (*(code *)item->proc)(item);
//         DeleteConflict(item->locate);
//         if (item->mode != 0) {
//           AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//         }
//         item->owner = (Humanoid *)0x0;
//         item->proc = (undefined **)0x0;
//       }
//     }
//     else {
//       UpdateCoordinate(item->locate);
//       pMVar3 = item->locate;
//       pSVar8 = &pMVar3->rotate;
//       pSVar7 = sprt;
//       do {
//         uVar9 = *(undefined4 *)(pMVar3->locate).coord.m[0];
//         uVar10 = *(undefined4 *)((pMVar3->locate).coord.m[0] + 2);
//         uVar11 = *(undefined4 *)((pMVar3->locate).coord.m[1] + 1);
//         (pSVar7->locate).flg = (pMVar3->locate).flg;
//         *(undefined4 *)(pSVar7->locate).coord.m[0] = uVar9;
//         *(undefined4 *)((pSVar7->locate).coord.m[0] + 2) = uVar10;
//         *(undefined4 *)((pSVar7->locate).coord.m[1] + 1) = uVar11;
//         pMVar3 = (ModelType *)((pMVar3->locate).coord.m + 2);
//         pSVar7 = (Sprite3D *)((pSVar7->locate).coord.m + 2);
//       } while (pMVar3 != (ModelType *)pSVar8);
//       DrawSprite(sprt);
//       memset((uchar *)&local_70,'\0',0x10);
//       iVar4 = rand();
//       local_70._0_4_ = (item->locate->locate).coord.t[0] + -0x19 + iVar4 % 0x32;
//       iVar4 = rand();
//       local_70._4_4_ = (item->locate->locate).coord.t[1] + -0x19 + iVar4 % 0x32;
//       iVar4 = rand();
//       local_80._8_4_ = (item->locate->locate).coord.t[2] + -0x19 + iVar4 % 0x32;
//       local_80._0_2_ = local_70.vx;
//       local_80._2_2_ = local_70.vy;
//       local_80._4_2_ = local_70.vz;
//       local_80._6_2_ = local_70.pad;
//       local_74 = local_64;
//       local_70.vx = (short)DAT_80097afc;
//       local_70.vy = DAT_80097afc._2_2_;
//       local_70.vz = (short)DAT_80097b00;
//       local_70.pad = DAT_80097b00._2_2_;
//       local_68 = local_80._8_4_;
//       iVar4 = rand();
//       SetBleed((VECTOR *)local_80,&local_70,iVar4 % 0x14,0xffff00);
//       uVar6 = (item->param).smoke.count + 0xff;
//       (item->param).smoke.count = uVar6;
//       bVar1 = item->mode;
//       if (bVar1 == 1) {
//         local_80._0_4_ = DAT_80097ae4;
//         local_80._4_4_ = DAT_80097ae8;
//         memset((uchar *)&local_68,'\0',0x10);
//         local_80._8_4_ = (item->locate->locate).coord.t[0];
//         local_74 = (item->locate->locate).coord.t[1];
//         pos = (VECTOR *)(local_80 + 8);
//         local_70._0_4_ = (item->locate->locate).coord.t[2];
//         local_70.vz = (undefined2)local_5c;
//         local_70.pad = local_5c._2_2_;
//         local_68 = local_80._8_4_;
//         local_64 = local_74;
//         local_60 = local_70._0_4_;
//         SetExplosion(pos,(SVECTOR *)local_80);
//         local_80._0_4_ = 0x78004b;
//         local_80._4_2_ = 0x4b;
//         SetHinoko(pos,(SVECTOR *)local_80,8);
//         local_80._0_4_ = -0xc80000;
//         local_80._4_4_ = (uint)(ushort)local_80._6_2_ << 0x10;
//         SetSmoke(pos,(SVECTOR *)local_80,0x14,6);
//         SoundEx(pos,0x25);
//         DeleteConflict(item->locate);
//         sVar2 = InsertConflict(item->locate);
//         ConflictObject[sVar2].offset.vx = 0;
//         ConflictObject[sVar2].offset.vz = 0;
//         ConflictObject[sVar2].offset.vy = 0;
//         ConflictObject[sVar2].size.vz = 0x5dc;
//         ConflictObject[sVar2].size.vy = 0x5dc;
//         ConflictObject[sVar2].size.vx = 0x5dc;
//         ConflictObject[sVar2].common = (undefined *)0x1;
//         ConflictObject[sVar2].size.pad = 1;
//         uVar6 = item->mode;
//         (item->collision).size = 0x5dc;
//         (item->collision).ofsY = 0;
//         (item->collision).mode = 1;
//         (item->collision).pause = 0;
//         item->mode = uVar6 + '\x01';
//         (item->param).smoke.count = '\x03';
//         FUN_8002f7f4();
//       }
//       else if (bVar1 < 2) {
//         if (bVar1 == 0) {
//           if (uVar6 == '\0') {
//             iVar4 = rand();
//             if (iVar4 % 10 < 2) {
//               memset((uchar *)&local_60,'\0',0x14);
//               local_80._0_4_ = item->type;
//               local_80._4_4_ = (sprt->locate).coord.t[0];
//               local_80._8_4_ = (sprt->locate).coord.t[1];
//               local_74 = (sprt->locate).coord.t[2];
//               local_70.vx = local_50.vx;
//               local_70.vy = local_50.vy;
//               local_60 = local_80._0_4_;
//               local_5c = local_80._4_4_;
//               local_58 = local_80._8_4_;
//               local_54 = local_74;
//               if (item->proc != (undefined **)0x0) {
//                 item->mode = 0xff;
//                 (*(code *)item->proc)(item);
//                 DeleteConflict(item->locate);
//                 if (item->mode != 0) {
//                   AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//                 }
//                 item->owner = (Humanoid *)0x0;
//                 item->proc = (undefined **)0x0;
//               }
//               local_48.type = local_80._0_4_;
//               local_48.user = (Humanoid *)&DAT_00000001;
//               local_48.start.vx = local_80._4_4_;
//               local_48.start.vy = local_80._8_4_;
//               local_48.end.vx = 0;
//               local_48.end.vy = 0;
//               local_48.end.vz = 0;
//               local_48.start.vz = local_74;
//               local_48.start.vy = GetAreaMapLevel(GlobalAreaMap,local_80._4_4_,local_80._8_4_);
//               ReqItemDrop(&local_48);
//               SetSmokeS(local_80 + 4,0,0xffffff9c,0,10);
//               return;
//             }
//           }
//           else {
//             if (uVar6 == 0x8c) {
//               DeleteConflict(item->locate);
//               sVar2 = InsertConflict(item->locate);
//               ConflictObject[sVar2].offset.vx = 0;
//               ConflictObject[sVar2].offset.vz = 0;
//               ConflictObject[sVar2].offset.vy = 0;
//               ConflictObject[sVar2].size.vz = 500;
//               ConflictObject[sVar2].size.vy = 500;
//               ConflictObject[sVar2].size.vx = 500;
//               ConflictObject[sVar2].common = (undefined *)0x1;
//               ConflictObject[sVar2].size.pad = 8;
//               (item->collision).size = 500;
//               (item->collision).ofsY = 0;
//               (item->collision).mode = 8;
//               (item->collision).pause = 0;
//             }
//             if (((int)item->locate->attribute & 0x8000U) == 0) {
//               iVar4 = -1;
//             }
//             else {
//               sVar2 = GetConflictResult(item->locate,-1);
//               iVar4 = (int)sVar2;
//             }
//             if (iVar4 == -1) {
//               return;
//             }
//             iVar5 = FUN_800294dc(ConflictObject[iVar4].common);
//             if ((iVar5 == 0) && (ConflictObject[iVar4].size.pad != 1)) {
//               return;
//             }
//           }
//           item->mode = item->mode + '\x01';
//         }
//       }
//       else if (bVar1 == 2) {
//         if ((uVar6 == '\0') && (item->proc != (undefined **)0x0)) {
//           item->mode = 0xff;
//           (*(code *)item->proc)(item);
//           DeleteConflict(item->locate);
//           if (item->mode != 0) {
//             AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//           }
//           item->owner = (Humanoid *)0x0;
//           item->proc = (undefined **)0x0;
//         }
//         if (((int)item->locate->attribute & 0x8000U) == 0) {
//           iVar4 = -1;
//         }
//         else {
//           sVar2 = GetConflictResult(item->locate,-1);
//           iVar4 = (int)sVar2;
//         }
//         if (iVar4 != -1) {
//           puVar12 = ConflictObject[iVar4].common;
//           iVar4 = FUN_800294dc(puVar12);
//           if (iVar4 != 0) {
//             puVar13 = *(undefined4 **)(*(int *)(puVar12 + 0x58) + 0x68);
//             if (0 < *(short *)(*(int *)(puVar12 + 0x58) + 100)) {
//               iVar4 = rand();
//               iVar5 = (int)*(short *)(*(int *)(puVar12 + 0x58) + 100);
//               if (iVar5 == 0) {
//                 trap(0x1c00);
//               }
//               if ((iVar5 == -1) && (iVar4 == -0x80000000)) {
//                 trap(0x1800);
//               }
//               puVar13 = puVar13 + iVar4 % iVar5;
//             }
//             super = (_GsCOORDINATE2 *)*puVar13;
//             memset((uchar *)&local_70,'\0',0x10);
//             iVar4 = rand();
//             local_70._0_4_ = iVar4 % 200 + -100;
//             iVar4 = rand();
//             local_70._4_4_ = iVar4 % 200 + -100;
//             iVar4 = rand();
//             local_80._8_4_ = iVar4 % 200 + -100;
//             local_80._0_2_ = local_70.vx;
//             local_80._2_2_ = local_70.vy;
//             local_80._4_2_ = local_70.vz;
//             local_80._6_2_ = local_70.pad;
//             local_74 = local_64;
//             local_68 = local_80._8_4_;
//             SetFrame((VECTOR *)local_80,0x3000,0x78,super);
//           }
//         }
//       }
//     }
//   }
//   return;
// }
