#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SearchTarget(struct Humanoid *human, long *distance, short *degree);
 *     HUMAN.C:436, 46 src lines, frame 64 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s1       struct Humanoid * human
 *     param $s3       long * distance
 *     param $s4       short * degree
 *     reg   $s2       struct VECTOR * head
 *     stack sp+16     struct VECTOR vect
 *     stack sp+32     struct SVECTOR svect
 *     reg   $s0       short mode
 *     reg   $a0       long dx
 *     reg   $a1       long dz
 *     reg   $s0       short roty
 *     reg   $a1       short degree
 *     reg   $a3       short n
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct Humanoid *StagePlayer;
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SearchTarget", SearchTarget);

// triage: HARD — 282 insns, mul/div, 3 loop, 3 callees, ~0.05 to GetDirection
// likely-relevant cookbook sections:
//   - Loops: 3 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Type propagation algorithm not settling */
//
// short SearchTarget(Humanoid *human,long *distance,short *degree)
//
// {
//   ushort uVar1;
//   VECTOR *pVVar2;
//   int iVar3;
//   long lVar4;
//   uint uVar5;
//   int iVar6;
//   int iVar7;
//   short sVar8;
//   short n;
//   int local_40;
//   int local_3c;
//   int local_38;
//   VECTOR local_30;
//   SVECTOR local_20;
//
//   pVVar2 = human->locate;
//   local_30.vx = pVVar2->vx;
//   local_30.vy = pVVar2->vy;
//   local_30.vz = pVVar2->vz;
//   local_30.pad = pVVar2->pad;
//   n = 1;
//   if (human->target == (ModelType *)0x0) {
//     return 0;
//   }
//   local_40 = (human->target->locate).coord.t[0] - local_30.vx;
//   iVar3 = (human->target->locate).coord.t[1] - local_30.vy;
//   local_38 = (human->target->locate).coord.t[2] - local_30.vz;
//   lVar4 = SquareRoot0(local_40 * local_40 + iVar3 * iVar3 + local_38 * local_38);
//   *distance = lVar4;
//   uVar1 = human->rotate->vy;
//   lVar4 = ratan2(-local_40,-local_38);
//   iVar6 = lVar4 - (uint)uVar1;
//   iVar7 = iVar6 * 0x10000 >> 0x10;
//   sVar8 = (short)iVar6;
//   if (iVar7 < 0x801) {
//     if (iVar7 < -0x7ff) {
//       sVar8 = sVar8 + 0x1000;
//     }
//   }
//   else {
//     sVar8 = 0x1000 - sVar8;
//   }
//   *degree = sVar8;
//   if ((GameClock + (*human->model->object)->id & 0x1fU) != 0) {
//     return 0;
//   }
//   uVar5 = (uint)((ushort)StagePlayer->status - 0xb < 2);
//   if (StagePlayer->status == 10) {
//     if (-1 < iVar3) {
//       return -2;
//     }
//     if ((iVar3 < -3000) && (*distance < 4000)) {
//       return -2;
//     }
//   }
//   iVar6 = iVar3;
//   if (iVar3 < 0) {
//     iVar6 = -iVar3;
//   }
//   if (2999 < iVar6) {
//     if (DAT_800979c0 == 0) {
//       return -2;
//     }
//     if (*distance < 4000) {
//       return -2;
//     }
//   }
//   if (*(int *)((int)searchsight + uVar5 * 0xc + 8) <= *distance) {
//     return -2;
//   }
//   iVar6 = (int)*degree;
//   if (iVar6 < 0) {
//     iVar6 = -iVar6;
//   }
//   if (iVar6 < 900) {
//     if ((0x1c1 < iVar6) && (uVar5 != 0)) {
//       return -1;
//     }
//     if (*(int *)((int)searchsight + uVar5 * 0xc) <= *distance) {
//       return -1;
//     }
//     iVar6 = 500;
//     if (uVar5 != 0) {
//       iVar6 = 300;
//     }
//     local_30.vy = (local_30.vy + 300) - (int)human->height;
//     iVar7 = (uint)(ushort)StagePlayer->height << 0x10;
//     local_3c = iVar7 >> 0x10;
//     if (StagePlayer->status == 0xb) {
//       local_3c = local_3c - (iVar7 >> 0x1f) >> 1;
//     }
//     local_3c = (iVar3 + -300 + (int)human->height) - local_3c;
//     iVar3 = local_40;
//     if (local_40 < 0) {
//       iVar3 = -local_40;
//     }
//     if (iVar6 < iVar3) goto LAB_80028ea4;
//     iVar3 = local_3c;
//     if (local_3c < 0) {
//       iVar3 = -local_3c;
//     }
//     if (iVar6 < iVar3) goto LAB_80028ea4;
//     n = 1;
//     iVar3 = local_38;
//     if (local_38 < 0) {
//       iVar3 = -local_38;
//     }
//     while (iVar6 < iVar3) {
// LAB_80028ea4:
//       do {
//         do {
//           n = n << 1;
//           local_40 = local_40 >> 1;
//           local_3c = local_3c >> 1;
//           local_38 = local_38 >> 1;
//           iVar3 = local_40;
//           if (local_40 < 0) {
//             iVar3 = -local_40;
//           }
//         } while (iVar6 < iVar3);
//         iVar3 = local_3c;
//         if (local_3c < 0) {
//           iVar3 = -local_3c;
//         }
//       } while (iVar6 < iVar3);
//       iVar3 = local_38;
//       if (local_38 < 0) {
//         iVar3 = -local_38;
//       }
//     }
//     local_20.vz = (short)local_38;
//     local_20.vx = (short)local_40;
//     local_20.vy = (short)local_3c;
//     pVVar2 = GetAreaMapPassage(GlobalAreaMap,&local_30,&local_20,n);
//     if (pVVar2 != (VECTOR *)0x0) {
//       return -2;
//     }
//     if (*distance < *(int *)((int)searchsight + uVar5 * 0xc + 4)) {
//       return 1;
//     }
//     return 2;
//   }
//   return -1;
// }
