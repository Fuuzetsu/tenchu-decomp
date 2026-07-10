#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemHenshin(struct tag_TItem *item);
 *     ITEM.C:2060, 140 src lines, frame 208 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s3       struct tag_TItem * item
 *     reg   $s4       struct param_henshin * param
 *     reg   $a1       int i
 *     reg   $s1       struct ModelArchiveType * mad
 *     reg   $a2       struct ModelArchiveType * hen
 *     stack sp+16     struct ModelArchiveType *[30] target
 *     reg   $s2       int targets
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+136    struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     stack sp+136    struct SVECTOR sv
 *     stack sp+144    struct SVECTOR sv
 *     reg   $s3       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE TargetSprite[1];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ProcItemHenshin", ProcItemHenshin);

// triage: HARD — 363 insns, mul/div, 2 loop, indirect-call, 10 callees, ~0.27 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcItemHenshin(tag_TItem *item)
//
// {
//   undefined ***pppuVar1;
//   byte bVar2;
//   short sVar3;
//   tag_TItem *ptVar4;
//   tag_TItem *ptVar5;
//   undefined2 *puVar6;
//   VECTOR *pVVar7;
//   undefined **ppuVar8;
//   Humanoid *pHVar9;
//   ModelType **ppMVar10;
//   MotionManager *pMVar11;
//   int iVar12;
//   ModelArchiveType *pMVar13;
//   TItemType TVar14;
//   PARAM_ITEM_USE local_40;
//
//   pHVar9 = item->owner;
//   pMVar13 = pHVar9->model;
//   if (item->mode == 0xff) {
//     if (item == DAT_80097aec) {
//       iVar12 = 0;
//       (pMVar13->rotate).pad = DAT_800c0630;
//       puVar6 = &DAT_800c0630;
//       if (0 < pMVar13->n) {
//         do {
//           (pMVar13->object[iVar12]->object).tmd = *(ulong **)(puVar6 + 2);
//           (pMVar13->object[iVar12]->locate).coord.t[0] = (int)(short)puVar6[4];
//           (pMVar13->object[iVar12]->locate).coord.t[1] = (int)(short)puVar6[5];
//           ppMVar10 = pMVar13->object + iVar12;
//           iVar12 = iVar12 + 1;
//           ((*ppMVar10)->locate).coord.t[2] = (int)(short)puVar6[6];
//           puVar6 = puVar6 + 6;
//         } while (iVar12 < pMVar13->n);
//       }
//       if (item->owner->status == 0xb) {
//         NowReturnNormal(item->owner);
//       }
//       DAT_80097aec = (tag_TItem *)0x0;
//       item->owner->itmctl = 0;
//     }
//     item->mode = '\0';
//     return;
//   }
//   bVar2 = item->mode;
//   if (bVar2 == 1) {
//     pMVar11 = pHVar9->motion;
//     if (pMVar11->mid != 0xf04) {
//       pVVar7 = GetAbsolutePosition(item->locate,0,0,0);
//       pHVar9 = item->owner;
//       TVar14 = item->type;
//       memset((uchar *)&local_40,'\0',0x28);
//       local_40.start.vx = pVVar7->vx;
//       local_40.start.vy = pVVar7->vy;
//       local_40.start.vz = pVVar7->vz;
//       local_40.type = TVar14;
//       local_40.user = pHVar9;
//       iVar12 = rand();
//       local_40.end.vx = iVar12 % 200 + -100;
//       iVar12 = rand();
//       local_40.end.vy = iVar12 % 100 + -200;
//       iVar12 = rand();
//       local_40.end.vz = iVar12 % 200 + -100;
//       ReqItemDrop(&local_40);
//       ppuVar8 = item->proc;
//       if (ppuVar8 == (undefined **)0x0) {
//         return;
//       }
//       item->mode = 0xff;
//       goto LAB_8004316c;
//     }
//     if (pMVar11->count != 0) {
//       return;
//     }
//     if (pMVar11->loop == 0) {
//       return;
//     }
//     NowReturnNormal(pHVar9);
//     local_40.type = DAT_80097af4;
//     local_40.user = DAT_80097af8;
//     SetSmoke((VECTOR *)(pMVar13->locate).coord.t,(SVECTOR *)&local_40,10,6);
//     ptVar4 = DAT_80097aec;
//     ptVar5 = item;
//     if ((DAT_80097aec != (tag_TItem *)0x0) &&
//        (pppuVar1 = &DAT_80097aec->proc, ptVar5 = item, *pppuVar1 != (undefined **)0x0)) {
//       DAT_80097aec->mode = 0xff;
//       (*(code *)*pppuVar1)(ptVar4);
//       DeleteConflict(ptVar4->locate);
//       if (ptVar4->mode != 0) {
//         AdtMessageBox("item dispose fail   id %d  mode %d",ptVar4->type,(uint)ptVar4->mode);
//       }
//       ptVar4->owner = (Humanoid *)0x0;
//       ptVar4->proc = (undefined **)0x0;
//       ptVar5 = item;
//     }
//   }
//   else {
//     if (1 < bVar2) {
//       if (bVar2 == 2) {
//         iVar12 = 0;
//         (pMVar13->rotate).pad = DAT_800c06f0;
//         puVar6 = &DAT_800c06f0;
//         if (0 < pMVar13->n) {
//           do {
//             (pMVar13->object[iVar12]->object).tmd = *(ulong **)(puVar6 + 2);
//             (pMVar13->object[iVar12]->locate).coord.t[0] = (int)(short)puVar6[4];
//             (pMVar13->object[iVar12]->locate).coord.t[1] = (int)(short)puVar6[5];
//             ppMVar10 = pMVar13->object + iVar12;
//             iVar12 = iVar12 + 1;
//             ((*ppMVar10)->locate).coord.t[2] = (int)(short)puVar6[6];
//             puVar6 = puVar6 + 6;
//           } while (iVar12 < pMVar13->n);
//         }
//         item->mode = item->mode + '\x01';
//         DAT_80097af0 = 600;
//         DAT_800979c0 = 0xfffffda8;
//         item->owner->itmctl = (short)item->type;
//         return;
//       }
//       if (bVar2 != 3) {
//         return;
//       }
//       DAT_80097af0 = DAT_80097af0 - 1;
//       if ((((0 < (int)((uint)DAT_80097af0 << 0x10)) &&
//            (pHVar9 = item->owner, (int)pHVar9->itmctl == item->type)) &&
//           (sVar3 = pHVar9->status, sVar3 != 0x10)) && (sVar3 != 0x11)) {
//         if (sVar3 != 7) {
//           return;
//         }
//         if ((-1 < pHVar9->motion->loop) && (pHVar9->motion->mid < 0x714)) {
//           return;
//         }
//       }
//       local_40.type = DAT_80097af4;
//       local_40.user = DAT_80097af8;
//       SetSmoke((VECTOR *)(pMVar13->locate).coord.t,(SVECTOR *)&local_40,10,6);
//       ppuVar8 = item->proc;
//       if (ppuVar8 == (undefined **)0x0) {
//         return;
//       }
//       item->mode = 0xff;
// LAB_8004316c:
//       (*(code *)ppuVar8)(item);
//       DeleteConflict(item->locate);
//       if (item->mode != 0) {
//         AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//       }
//       item->owner = (Humanoid *)0x0;
//       item->proc = (undefined **)0x0;
//       return;
//     }
//     if (bVar2 != 0) {
//       return;
//     }
//     SetNowMotion(pHVar9,0xf04,1);
//     Sound(item->owner,0x4c);
//     ptVar5 = DAT_80097aec;
//   }
//   DAT_80097aec = ptVar5;
//   item->mode = item->mode + '\x01';
//   return;
// }
