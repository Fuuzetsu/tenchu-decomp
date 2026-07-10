#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActivateHumans(void);
 *     WORLD.C:752, 41 src lines, frame 48 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     reg   $s1       int i
 *     reg   $s0       struct Humanoid * target
 *     stack sp+16     struct VECTOR vc
 *     reg   $s0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *StagePlayer;
 *     extern long GameClock;
 *     extern short SkipFrame;
 *     extern struct StageCharType StageChar[18];
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern int StageID;
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ActivateHumans", ActivateHumans);

// triage: HARD — 402 insns, mul/div, 2 loop, 3 callees, ~0.05 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void ActivateHumans(void)
//
// {
//   bool bVar1;
//   Humanoid *pHVar2;
//   short sVar3;
//   ushort uVar4;
//   VECTOR *pVVar5;
//   int iVar6;
//   long lVar7;
//   int iVar8;
//   Humanoid *pHVar9;
//   ModelType *pMVar10;
//   int iVar11;
//   Humanoid *pHVar12;
//   short sVar13;
//   int iVar14;
//   VECTOR local_50;
//   VECTOR local_40;
//   long local_30;
//   int local_2c;
//   long local_28;
//   long local_24;
//
//   pHVar2 = CamState.Owner;
//   pVVar5 = (CamState.Owner)->locate;
//   local_50.vx = pVVar5->vx;
//   local_50.vy = pVVar5->vy;
//   local_50.vz = pVVar5->vz;
//   local_50.pad = pVVar5->pad;
//   iVar14 = 26000;
//   if (StagePlayer->motion->mid != 0xf05) {
//     iVar14 = 13000;
//   }
//   if ((GameClock == (GameClock / 0x1e) * 0x1e) && (SkipFrame == 0)) {
//     iVar8 = (0xec78 - DAT_800976b8) / 5000 + -1;
//     DAT_80097f40 = (short)iVar8;
//     sVar3 = DAT_80097f40;
//     if (iVar8 * 0x10000 >> 0x10 < 2) {
//       sVar3 = DAT_80097f44 + -1;
//     }
//     if (sVar3 < 7) {
//       if (sVar3 < 3) {
//         sVar3 = 3;
//       }
//     }
//     else {
//       sVar3 = 6;
//     }
//     sVar13 = 0;
//     DAT_80097f42 = 0;
//     DAT_80097f44 = sVar3;
//     while ((int)sVar13 < (int)Humans) {
//       pHVar12 = HumanGroup[sVar13];
//       if (pHVar12 == pHVar2) {
// LAB_8003be28:
//         sVar13 = sVar13 + 1;
//       }
//       else {
//         iVar8 = GetVectorDistance(pHVar12->locate,&local_50);
//         if (iVar8 < 0x4269) {
//           if ((pHVar12->type & 0xf0U) == 0x80) {
// LAB_8003ba7c:
//             bVar1 = true;
//           }
//           else {
//             bVar1 = true;
//             if ((pHVar12->type != 0xa9) && (-1 < pHVar12->life)) {
//               if ((GameClock == 0x1e) || (StageID == 8)) goto LAB_8003ba7c;
//               if (DAT_80097726 < DAT_80097f44) {
//                 bVar1 = true;
//                 if (DAT_80097f44 <= DAT_80097f42) {
//                   bVar1 = iVar8 < iVar14;
//                 }
//               }
//               else {
//                 if (iVar14 <= iVar8) goto LAB_8003ba4c;
//                 if (((pHVar12->attribute & 0x80U) == 0) && (DAT_80097f42 < DAT_80097f44))
//                 goto LAB_8003ba7c;
//                 iVar6 = 0;
//                 sVar3 = 0;
//                 pHVar9 = _DAT_800be858;
//                 while (pHVar9 != pHVar12) {
//                   sVar3 = (short)iVar6;
//                   iVar6 = iVar6 + 1;
//                   if (DAT_80097726 <= sVar3) break;
//                   sVar3 = (short)iVar6;
//                   pHVar9 = *(Humanoid **)(&DAT_800be858 + (iVar6 * 0x10000 >> 0xe));
//                 }
//                 bVar1 = sVar3 != DAT_80097726;
//               }
//             }
//           }
//         }
//         else {
// LAB_8003ba4c:
//           bVar1 = false;
//         }
//         if (!bVar1) {
//           if (((pHVar12->attribute & 0x80U) == 0) && (sVar3 = pHVar12->type, sVar3 != 0x84)) {
//             if (((sVar3 == 0x87) && (StageID - 6U < 2)) || (sVar3 == 0x83)) {
//               iVar8 = 0;
//               if (StageChar[0].stage != -1) {
//                 iVar11 = StageID + 1;
//                 iVar6 = 0;
//                 do {
//                   iVar6 = iVar6 >> 0x10;
//                   if ((StageChar[iVar6].stage == iVar11) &&
//                      (StageChar[iVar6].chrid == pHVar12->type)) {
//                     (pHVar12->model->locate).coord.t[0] = StageChar[iVar6].position.vx * 1000;
//                     (pHVar12->model->locate).coord.t[1] = StageChar[iVar6].position.vy * 1000;
//                     (pHVar12->model->locate).coord.t[2] = StageChar[iVar6].position.vz * 1000;
//                   }
//                   iVar8 = iVar8 + 1;
//                   iVar6 = iVar8 * 0x10000;
//                 } while (StageChar[iVar8 * 0x10000 >> 0x10].stage != -1);
//               }
//               if ((pHVar12->type == 0x83) && (pHVar12->life == 0)) {
//                 pHVar12->life = 1;
//               }
//             }
//             else if ((pHVar12->status != 0x11) && ((pHVar12->attribute & 0x20U) == 0)) {
//               memset((uchar *)&local_30,'\0',0x10);
//               local_40.vx = pHVar12->point[0];
//               local_40.vy = pHVar12->locate->vy + -0x5dc;
//               local_40.vz = pHVar12->point[1];
//               local_40.pad = local_24;
//               local_30 = local_40.vx;
//               local_2c = local_40.vy;
//               local_28 = local_40.vz;
//               iVar8 = GetVectorDistance(&local_40,&local_50);
//               if ((17000 < iVar8) &&
//                  (lVar7 = GetAreaMapLevel(GlobalAreaMap,local_40.vx,local_40.vy),
//                  lVar7 != -0x80000000)) {
//                 (pHVar12->model->locate).coord.t[0] = local_40.vx;
//                 (pHVar12->model->locate).coord.t[1] = lVar7;
//                 (pHVar12->model->locate).coord.t[2] = local_40.vz;
//               }
//             }
//             pHVar12->attribute = pHVar12->attribute | 0x80;
//             pMVar10 = *pHVar12->model->object;
//             uVar4 = pMVar10->attribute & 0xbfff;
// LAB_8003be24:
//             pMVar10->attribute = uVar4;
//           }
//           goto LAB_8003be28;
//         }
//         if ((pHVar12->attribute & 0x80U) != 0) {
//           if ((((StageID == 8) || (pHVar12->life < 0)) || (GameClock == 0x1e)) ||
//              ((DAT_80097f42 < DAT_80097f44 && (13000 < iVar8)))) {
//             pHVar12->attribute = pHVar12->attribute & 0xff7f;
//             DAT_80097f42 = DAT_80097f42 + 1;
//             pMVar10 = *pHVar12->model->object;
//             uVar4 = pMVar10->attribute | 0x4000;
//             goto LAB_8003be24;
//           }
//           goto LAB_8003be28;
//         }
//         DAT_80097f42 = DAT_80097f42 + 1;
//         sVar13 = sVar13 + 1;
//       }
//     }
//   }
//   return;
// }
