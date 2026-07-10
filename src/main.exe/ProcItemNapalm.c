#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemNapalm(struct tag_TItem *item);
 *     ITEM.C:3014, 71 src lines, frame 80 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
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
 *     param $s2       struct tag_TItem * item
 *     reg   $s5       struct Sprite3D * model
 *     reg   $s3       struct param_napalm * param
 *     reg   $s0       int ex
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s0       struct Humanoid * human
 *     reg   $s0       struct Humanoid * human
 *     reg   $s1       struct ModelType * model
 *     stack sp+16     struct VECTOR pos
 *     reg   $s2       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcItemNapalm", ProcItemNapalm);

// triage: HARD — 418 insns, mul/div, 2 loop, indirect-call, 11 callees, ~0.25 to ProcItemMakibishi
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Type propagation algorithm not settling */
//
// void ProcItemNapalm(tag_TItem *item)
//
// {
//   uchar uVar1;
//   byte bVar2;
//   short sVar3;
//   int iVar4;
//   long lVar5;
//   undefined **ppuVar6;
//   ModelType *pMVar7;
//   Sprite3D *pSVar8;
//   int iVar9;
//   SVECTOR *pSVar10;
//   int iVar11;
//   undefined4 uVar12;
//   undefined4 uVar13;
//   undefined4 uVar14;
//   uint uVar15;
//   undefined *puVar16;
//   undefined4 *puVar17;
//   _GsCOORDINATE2 *super;
//   Sprite3D *sprt;
//   VECTOR local_40;
//   int local_30;
//   int local_2c;
//   int local_28;
//   long local_24;
//
//   sprt = (Sprite3D *)item->model;
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//     return;
//   }
//   bVar2 = item->mode;
//   if (bVar2 == 1) {
//     uVar15 = (uint)(item->param).napalm.count;
//     iVar9 = uVar15 * uVar15;
//     (item->locate->locate).coord.t[0] =
//          (item->locate->locate).coord.t[0] + ((item->param).napalm.vec.vx * iVar9) / 100;
//     (item->locate->locate).coord.t[1] =
//          (item->locate->locate).coord.t[1] + ((item->param).napalm.vec.vy * iVar9) / 100;
//     (item->locate->locate).coord.t[2] =
//          (item->locate->locate).coord.t[2] + ((item->param).napalm.vec.vz * iVar9) / 100;
//     iVar4 = rand();
//     uVar1 = ((char)iVar4 + (char)(iVar4 / 0x19) * -0x19 + -0x1a) -
//             (char)(((uint)(item->param).napalm.count * 0xe6) / 0x14);
//     (sprt->sprite).r = uVar1;
//     (sprt->sprite).g = uVar1;
//     (sprt->sprite).b = uVar1;
//     iVar4 = rand();
//     iVar11 = 0xff - (uint)(sprt->sprite).r;
//     (sprt->sprite).rotate = (iVar4 % 0x168) * 0x1000;
//     sprt->scale = (uint)(iVar9 * 0x1000) / 0x32 + 0x1000;
//     (DAT_80097f60->sprite).r =
//          (char)((ulonglong)((longlong)iVar11 * 0x55555556) >> 0x20) - (char)(iVar11 >> 0x1f);
//     (DAT_80097f60->sprite).g = (DAT_80097f60->sprite).r;
//     (DAT_80097f60->sprite).b = (DAT_80097f60->sprite).r;
//     pSVar8 = DAT_80097f60;
//     (DAT_80097f60->sprite).rotate = (sprt->sprite).rotate;
//     pSVar8->scale = sprt->scale;
//     if ((item->param).napalm.count == '\n') {
//       DeleteConflict(item->locate);
//       sVar3 = InsertConflict(item->locate);
//       ConflictObject[sVar3].offset.vx = 0;
//       ConflictObject[sVar3].offset.vz = 0;
//       ConflictObject[sVar3].offset.vy = 0;
//       ConflictObject[sVar3].size.vz = 500;
//       ConflictObject[sVar3].size.vy = 500;
//       ConflictObject[sVar3].size.vx = 500;
//       ConflictObject[sVar3].common = (undefined *)0x1;
//       ConflictObject[sVar3].size.pad = 1;
//       (item->collision).size = 500;
//       (item->collision).ofsY = 0;
//       (item->collision).mode = 1;
//       (item->collision).pause = 0;
//     }
//     bVar2 = (item->param).napalm.count + 1;
//     (item->param).napalm.count = bVar2;
//     if (0x14 < bVar2) {
//       item->mode = item->mode + '\x01';
//     }
//     if (((int)item->locate->attribute & 0x8000U) == 0) {
//       iVar9 = -1;
//     }
//     else {
//       sVar3 = GetConflictResult(item->locate,-1);
//       iVar9 = (int)sVar3;
//     }
//     if (iVar9 != -1) {
//       puVar16 = ConflictObject[iVar9].common;
//       iVar9 = FUN_800294dc(puVar16);
//       if (iVar9 != 0) {
//         puVar17 = *(undefined4 **)(*(int *)(puVar16 + 0x58) + 0x68);
//         if (0 < *(short *)(*(int *)(puVar16 + 0x58) + 100)) {
//           iVar9 = rand();
//           iVar4 = (int)*(short *)(*(int *)(puVar16 + 0x58) + 100);
//           if (iVar4 == 0) {
//             trap(0x1c00);
//           }
//           if ((iVar4 == -1) && (iVar9 == -0x80000000)) {
//             trap(0x1800);
//           }
//           puVar17 = puVar17 + iVar9 % iVar4;
//         }
//         super = (_GsCOORDINATE2 *)*puVar17;
//         memset((uchar *)&local_30,'\0',0x10);
//         iVar9 = rand();
//         local_30 = iVar9 % 200 + -100;
//         iVar9 = rand();
//         local_2c = iVar9 % 200 + -100;
//         iVar9 = rand();
//         local_40.vz = iVar9 % 200 + -100;
//         local_40.vx = local_30;
//         local_40.vy = local_2c;
//         local_40.pad = local_24;
//         local_28 = local_40.vz;
//         SetFrame(&local_40,0x3000,0x3c,super);
//       }
//     }
//     lVar5 = GetAreaMapLevel(GlobalAreaMap,(item->locate->locate).coord.t[0],
//                             (item->locate->locate).coord.t[1]);
//     if (lVar5 == -0x80000000) {
//       ppuVar6 = item->proc;
//       if (ppuVar6 == (undefined **)0x0) {
//         return;
//       }
//       item->mode = 0xff;
// LAB_80046f50:
//       (*(code *)ppuVar6)(item);
//       DeleteConflict(item->locate);
//       if (item->mode != 0) {
//         AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//       }
//       item->owner = (Humanoid *)0x0;
//       item->proc = (undefined **)0x0;
//       return;
//     }
//   }
//   else if (bVar2 < 2) {
//     if (bVar2 == 0) {
//       (item->param).napalm.count = '\0';
//       item->mode = item->mode + '\x01';
//       return;
//     }
//   }
//   else if (bVar2 == 2) {
//     ppuVar6 = item->proc;
//     if (ppuVar6 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//     goto LAB_80046f50;
//   }
//   UpdateCoordinate(item->locate);
//   pMVar7 = item->locate;
//   pSVar10 = &pMVar7->rotate;
//   pSVar8 = DAT_80097f60;
//   do {
//     uVar13 = *(undefined4 *)(pMVar7->locate).coord.m[0];
//     uVar14 = *(undefined4 *)((pMVar7->locate).coord.m[0] + 2);
//     uVar12 = *(undefined4 *)((pMVar7->locate).coord.m[1] + 1);
//     (pSVar8->locate).flg = (pMVar7->locate).flg;
//     *(undefined4 *)(pSVar8->locate).coord.m[0] = uVar13;
//     *(undefined4 *)((pSVar8->locate).coord.m[0] + 2) = uVar14;
//     *(undefined4 *)((pSVar8->locate).coord.m[1] + 1) = uVar12;
//     pMVar7 = (ModelType *)((pMVar7->locate).coord.m + 2);
//     pSVar8 = (Sprite3D *)((pSVar8->locate).coord.m + 2);
//   } while (pMVar7 != (ModelType *)pSVar10);
//   DrawSprite(DAT_80097f60);
//   pMVar7 = item->locate;
//   pSVar10 = &pMVar7->rotate;
//   pSVar8 = sprt;
//   do {
//     uVar13 = *(undefined4 *)(pMVar7->locate).coord.m[0];
//     uVar14 = *(undefined4 *)((pMVar7->locate).coord.m[0] + 2);
//     uVar12 = *(undefined4 *)((pMVar7->locate).coord.m[1] + 1);
//     (pSVar8->locate).flg = (pMVar7->locate).flg;
//     *(undefined4 *)(pSVar8->locate).coord.m[0] = uVar13;
//     *(undefined4 *)((pSVar8->locate).coord.m[0] + 2) = uVar14;
//     *(undefined4 *)((pSVar8->locate).coord.m[1] + 1) = uVar12;
//     pMVar7 = (ModelType *)((pMVar7->locate).coord.m + 2);
//     pSVar8 = (Sprite3D *)((pSVar8->locate).coord.m + 2);
//   } while (pMVar7 != (ModelType *)pSVar10);
//   DrawSprite(sprt);
//   return;
// }
