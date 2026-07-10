#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ArrangeLocalMatrix(struct ModelType *model, struct MATRIX *t);
 *     ITEM.C:3328, 38 src lines, frame 72 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct ModelType * model
 *     param $s3       struct MATRIX * t
 *     stack sp+16     struct MATRIX m
 *     reg   $a1       int i
 *     reg   $t4       int j
 *     reg   $t5       int k
 *     reg   $s1       long det
 *     reg   $a3       long t
 *     reg   $t1       long u
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ArrangeLocalMatrix", ArrangeLocalMatrix);

// triage: MEDIUM — 159 insns, mul/div, 2 loop, 2 callees, ~0.08 to AdtFntOpen
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Removing unreachable block (ram,0x80047a1c) */
//
// void ArrangeLocalMatrix(ModelType *model,MATRIX *t)
//
// {
//   int iVar1;
//   int iVar2;
//   int iVar3;
//   short *psVar4;
//   short *psVar5;
//   int iVar6;
//   int iVar7;
//   int iVar8;
//   int iVar9;
//   int iVar10;
//   int iVar11;
//   int iVar12;
//   int iVar13;
//   int iVar14;
//   int iVar15;
//   int iVar16;
//   MATRIX *pMVar17;
//   MATRIX local_50;
//   int local_30;
//
//   GsGetLw(&model->locate,&local_50);
//   iVar15 = 0x1000;
//   iVar13 = 0;
//   iVar14 = 0;
//   pMVar17 = &local_50;
//   for (iVar16 = 0; iVar16 < 3; iVar16 = iVar16 + 1) {
//     iVar10 = (int)pMVar17->m[0][0];
//     iVar1 = iVar15 * iVar10;
//     if (iVar10 == 0) {
//       iVar10 = 0x1000;
//       iVar1 = iVar15 * 0x1000;
//     }
//     if (iVar1 < 0) {
//       iVar1 = iVar1 + 0xfff;
//     }
//     iVar15 = iVar1 >> 0xc;
//     iVar7 = 0;
//     iVar1 = iVar13;
//     do {
//       psVar4 = (short *)((int)local_50.m[0] + iVar1);
//       iVar2 = (int)*psVar4 << 0xc;
//       if (iVar10 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar10 == -1) && (iVar2 == -0x80000000)) {
//         trap(0x1800);
//       }
//       iVar7 = iVar7 + 1;
//       *psVar4 = (short)(iVar2 / iVar10);
//       iVar1 = iVar1 + 2;
//     } while (iVar7 < 3);
//     if (iVar10 == 0) {
//       trap(0x1c00);
//     }
//     iVar2 = 0;
//     local_30 = iVar13;
//     pMVar17->m[0][0] = (short)(0x1000000 / iVar10);
//     iVar1 = iVar14;
//     for (iVar7 = 0; iVar7 < 3; iVar7 = iVar7 + 1) {
//       psVar4 = (short *)((int)local_50.m[0] + iVar1);
//       if (iVar7 != iVar16) {
//         iVar8 = 0;
//         iVar12 = (int)*psVar4;
//         iVar11 = iVar12 * -0x1000;
//         iVar6 = iVar2;
//         iVar9 = local_30;
//         do {
//           if (iVar8 == iVar16) {
//             if (iVar10 == 0) {
//               trap(0x1c00);
//             }
//             if ((iVar10 == -1) && (iVar11 == -0x80000000)) {
//               trap(0x1800);
//             }
//             *psVar4 = (short)(iVar11 / iVar10);
//           }
//           else {
//             iVar3 = *(short *)((int)local_50.m[0] + iVar9) * iVar12;
//             psVar5 = (short *)((int)local_50.m[0] + iVar6);
//             if (iVar3 < 0) {
//               iVar3 = iVar3 + 0xfff;
//             }
//             *psVar5 = *psVar5 - (short)(iVar3 >> 0xc);
//           }
//           iVar9 = iVar9 + 2;
//           iVar8 = iVar8 + 1;
//           iVar6 = iVar6 + 2;
//         } while (iVar8 < 3);
//       }
//       iVar1 = iVar1 + 6;
//       iVar2 = iVar2 + 6;
//     }
//     iVar13 = iVar13 + 6;
//     iVar14 = iVar14 + 2;
//     pMVar17 = (MATRIX *)(pMVar17->m[1] + 1);
//   }
//   if (iVar15 - 0x800U < 0x801) {
//     MulMatrix(&local_50,t);
//     *(undefined4 *)t->m[0] = local_50.m[0]._0_4_;
//     *(undefined4 *)(t->m[0] + 2) = local_50.m._4_4_;
//     *(undefined4 *)(t->m[1] + 1) = local_50.m[1]._2_4_;
//     *(undefined4 *)t->m[2] = local_50.m[2]._0_4_;
//     *(undefined4 *)(t->m[2] + 2) = local_50._16_4_;
//     t->t[0] = local_50.t[0];
//     t->t[1] = local_50.t[1];
//     t->t[2] = local_50.t[2];
//   }
//   return;
// }
