#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemNemuri(struct tag_TItem *item);
 *     ITEM.C:2738, 93 src lines, frame 112 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     reg   $s4       struct Sprite3D * model
 *     reg   $s3       struct param_napalm * param
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       struct VECTOR * pos
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s1       int env
 *     reg   $v0       int bright
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s3       struct Humanoid * human
 *     reg   $s3       struct Humanoid * human
 *     stack sp+16     struct VECTOR pos
 *     stack sp+32     struct SVECTOR vec
 *     stack sp+48     struct VECTOR pos
 *     reg   $s3       struct Humanoid * human
 *     reg   $s2       struct tag_TItem * item
 *     reg   $s2       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcItemNemuri", ProcItemNemuri);

// triage: HARD — 422 insns, mul/div, 1 loop, indirect-call, 18 callees, ~0.26 to ProcItemDrop
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
// void ProcItemNemuri(tag_TItem *item)
//
// {
//   uchar uVar1;
//   byte bVar2;
//   short sVar3;
//   VECTOR *pVVar4;
//   int iVar5;
//   long lVar6;
//   undefined **ppuVar7;
//   ModelType *pMVar8;
//   Sprite3D *pSVar9;
//   MotionManager *pMVar10;
//   SVECTOR *pSVar11;
//   undefined4 uVar12;
//   undefined4 uVar13;
//   undefined4 uVar14;
//   Humanoid *human;
//   Sprite3D *sprt;
//   SVECTOR local_50;
//   int local_48;
//   VECTOR local_40;
//   long local_30;
//   long local_2c;
//   long local_28;
//   long local_24;
//
//   sprt = (Sprite3D *)item->model;
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//     return;
//   }
//   bVar2 = item->mode;
//   if (bVar2 == 1) {
//     pMVar10 = item->owner->motion;
//     if (pMVar10->mid == 0xf02) {
//       if (pMVar10->count != 3) {
//         return;
//       }
//       pVVar4 = GetAbsolutePosition(item->owner->model->object[0xe],0,0,0);
//       (item->param).napalm.count = '\0';
//       item->mode = item->mode + '\x01';
//       (item->locate->locate).coord.t[0] = pVVar4->vx;
//       (item->locate->locate).coord.t[1] = pVVar4->vy;
//       (item->locate->locate).coord.t[2] = pVVar4->vz;
//       DeleteConflict(item->locate);
//       sVar3 = InsertConflict(item->locate);
//       ConflictObject[sVar3].offset.vx = 0;
//       ConflictObject[sVar3].offset.vz = 0;
//       ConflictObject[sVar3].offset.vy = 0;
//       ConflictObject[sVar3].size.vz = 1000;
//       ConflictObject[sVar3].size.vy = 1000;
//       ConflictObject[sVar3].size.vx = 1000;
//       ConflictObject[sVar3].common = (undefined *)0x1;
//       ConflictObject[sVar3].size.pad = 8;
//       (item->collision).size = 1000;
//       (item->collision).ofsY = 0;
//       (item->collision).mode = 8;
//       (item->collision).pause = 0;
//       return;
//     }
// LAB_80045e78:
//     ppuVar7 = item->proc;
//     if (ppuVar7 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
// LAB_80045e90:
//     (*(code *)ppuVar7)(item);
//     DeleteConflict(item->locate);
//     if (item->mode != 0) {
//       AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//     }
//     item->owner = (Humanoid *)0x0;
//     item->proc = (undefined **)0x0;
//   }
//   else {
//     if (bVar2 < 2) {
//       if (bVar2 == 0) {
//         SetNowMotion(item->owner,0xf02,1);
//         SoundEx((VECTOR *)(item->owner->model->locate).coord.t,0x26);
//         item->mode = item->mode + '\x01';
//         return;
//       }
//     }
//     else if (bVar2 == 2) {
//       iVar5 = rsin((uint)(item->param).napalm.count * 0x88);
//       if (iVar5 < 0) {
//         iVar5 = iVar5 + 0x3f;
//       }
//       (item->locate->locate).coord.t[0] =
//            (item->locate->locate).coord.t[0] + (int)(item->param).napalm.vec.vx;
//       (item->locate->locate).coord.t[1] =
//            (item->locate->locate).coord.t[1] + (int)(item->param).napalm.vec.vy;
//       (item->locate->locate).coord.t[2] =
//            (item->locate->locate).coord.t[2] + (int)(item->param).napalm.vec.vz;
//       iVar5 = (iVar5 >> 6) + 0x80;
//       uVar1 = (uchar)iVar5;
//       (sprt->sprite).r = uVar1;
//       (sprt->sprite).g = uVar1;
//       (sprt->sprite).b = uVar1;
//       bVar2 = (item->param).napalm.count;
//       sprt->scale = iVar5 * 2 + 0x4000;
//       (sprt->sprite).rotate = (uint)bVar2 * 0x2d000;
//       SetBleeds((short)item->locate + 0x18,300,10);
//       bVar2 = (item->param).napalm.count + 1;
//       (item->param).napalm.count = bVar2;
//       if (100 < bVar2) {
//         item->mode = item->mode + '\x01';
//       }
//       if (((int)item->locate->attribute & 0x8000U) == 0) {
//         iVar5 = -1;
//       }
//       else {
//         sVar3 = GetConflictResult(item->locate,-1);
//         iVar5 = (int)sVar3;
//       }
//       if (iVar5 != -1) {
//         human = (Humanoid *)ConflictObject[iVar5].common;
//         iVar5 = FUN_800294dc(human);
//         if ((iVar5 != 0) && (human != item->owner)) {
//           if (0 < human->model->n) {
//             rand();
//           }
//           memset((uchar *)&local_50,'\0',0x10);
//           iVar5 = rand();
//           local_50._0_4_ = iVar5 % 200 + -100;
//           iVar5 = rand();
//           local_50._4_4_ = iVar5 % 200 + -100;
//           iVar5 = rand();
//           local_48 = iVar5 % 200 + -100;
//           SoundEx((VECTOR *)(item->locate->locate).coord.t,0x23);
//           local_50.vx = (short)DAT_80097b04;
//           local_50.vy = DAT_80097b04._2_2_;
//           local_50.vz = (short)DAT_80097b08;
//           local_50.pad = DAT_80097b08._2_2_;
//           memset((uchar *)&local_30,'\0',0x10);
//           local_40.vx = (human->model->locate).coord.t[0];
//           local_40.vy = (human->model->locate).coord.t[1];
//           local_40.vz = (human->model->locate).coord.t[2];
//           local_40.pad = local_24;
//           local_30 = local_40.vx;
//           local_2c = local_40.vy;
//           local_28 = local_40.vz;
//           SetSmoke(&local_40,&local_50,10,0x1e);
//           if ((0 < human->life) && (human->motion->mid != 0x100)) {
//             if (((human->type & 0xf0U) != 0x80) && (human->life != -1)) {
//               EquipWeapon(human,0);
//               SetNowMotion(human,0x80f,1);
//               human->think[0] = (undefined **)Think1sleep;
//               human->attribute = human->attribute & 0xfffc;
//             }
//             SetNowMotion(human,0x100,1);
//             Sound(human,6);
//           }
//           ppuVar7 = item->proc;
//           if (ppuVar7 == (undefined **)0x0) {
//             return;
//           }
//           item->mode = 0xff;
//           goto LAB_80045e90;
//         }
//       }
//       lVar6 = GetAreaMapLevel(GlobalAreaMap,(item->locate->locate).coord.t[0],
//                               (item->locate->locate).coord.t[1]);
//       if (lVar6 == -0x80000000) {
//         ppuVar7 = item->proc;
//         if (ppuVar7 == (undefined **)0x0) {
//           return;
//         }
//         item->mode = 0xff;
//         goto LAB_80045e90;
//       }
//     }
//     else if (bVar2 == 3) goto LAB_80045e78;
//     UpdateCoordinate(item->locate);
//     pMVar8 = item->locate;
//     pSVar11 = &pMVar8->rotate;
//     pSVar9 = sprt;
//     do {
//       uVar13 = *(undefined4 *)(pMVar8->locate).coord.m[0];
//       uVar14 = *(undefined4 *)((pMVar8->locate).coord.m[0] + 2);
//       uVar12 = *(undefined4 *)((pMVar8->locate).coord.m[1] + 1);
//       (pSVar9->locate).flg = (pMVar8->locate).flg;
//       *(undefined4 *)(pSVar9->locate).coord.m[0] = uVar13;
//       *(undefined4 *)((pSVar9->locate).coord.m[0] + 2) = uVar14;
//       *(undefined4 *)((pSVar9->locate).coord.m[1] + 1) = uVar12;
//       pMVar8 = (ModelType *)((pMVar8->locate).coord.m + 2);
//       pSVar9 = (Sprite3D *)((pSVar9->locate).coord.m + 2);
//     } while (pMVar8 != (ModelType *)pSVar11);
//     DrawSprite(sprt);
//   }
//   return;
// }
