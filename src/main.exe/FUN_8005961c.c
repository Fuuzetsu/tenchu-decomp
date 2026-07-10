#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8005961c", FUN_8005961c);

// triage: VERY-HARD — 315 insns, 7 GTE CMD — UN-SPLITTABLE (as can't assemble), 2 loop, 0 callees, ~0.04 to DeleteConflict
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Toolchain gotchas: GTE command opcodes — `as` rejects them, so the per-function split does not even assemble; needs the GTE->.word pass first

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// uint * FUN_8005961c(int param_1,int param_2,uint *param_3,int param_4,uint *param_5)
//
// {
//   uint uVar1;
//   int iVar2;
//   int iVar3;
//   uint uVar4;
//   uint uVar5;
//   uint *puVar6;
//   int iVar7;
//   int iVar8;
//   uint uVar9;
//   CVECTOR *pCVar10;
//   uint *puVar11;
//   uint *puVar12;
//   uint *r0;
//   uint *puVar13;
//   uint *r0_00;
//   uint *r0_01;
//   uint *puVar14;
//   undefined4 uVar15;
//
//   if (param_4 != 0) {
//     r0_00 = param_5 + 0x1d;
//     puVar14 = param_5 + 1;
//     uVar15 = 0x3c;
//     r0_01 = param_5 + 0x17;
//     puVar13 = (uint *)(param_1 + 0x20);
//     r0 = param_5;
//     do {
//       gte_ldv3((SVECTOR *)((uint)(ushort)puVar13[1] * 8 + param_2),
//                (SVECTOR *)((uint)*(ushort *)((int)puVar13 + 6) * 8 + param_2),
//                (SVECTOR *)((uint)(ushort)puVar13[2] * 8 + param_2));
//       gte_rtpt();
//       r0[3] = puVar13[-7];
//       r0[6] = puVar13[-6];
//       r0[9] = puVar13[-5];
//       gte_stflg((long *)r0_00);
//       if (-1 < (int)param_5[0x1d]) {
//         gte_nclip();
//         r0[1] = puVar13[-3];
//         *(char *)((int)puVar14 + 3) = (char)uVar15;
//         gte_stopz((long *)(param_5 + 0x20));
//         if (0 < (int)param_5[0x20]) {
//           gte_stsxy3_gt3(r0);
//           gte_ldv0((SVECTOR *)((uint)*(ushort *)((int)puVar13 + 10) * 8 + param_2));
//           gte_rtps();
//           r0[0xc] = puVar13[-4];
//           r0[4] = puVar13[-2];
//           r0[7] = puVar13[-1];
//           r0[10] = *puVar13;
//           gte_stflg((long *)r0_00);
//           if (-1 < (int)param_5[0x1d]) {
//             gte_stsxy((long *)(r0 + 0xb));
//             iVar7 = (int)(short)r0[2];
//             iVar2 = (int)(short)r0[5];
//             iVar8 = iVar7;
//             if (iVar2 < iVar7) {
//               iVar8 = iVar2;
//               iVar2 = iVar7;
//             }
//             iVar3 = (int)(short)r0[8];
//             iVar7 = iVar3;
//             if ((iVar8 <= iVar3) && (iVar7 = iVar8, iVar2 < iVar3)) {
//               iVar2 = iVar3;
//             }
//             iVar3 = (int)(short)r0[0xb];
//             iVar8 = iVar3;
//             if ((iVar7 <= iVar3) && (iVar8 = iVar7, iVar2 < iVar3)) {
//               iVar2 = iVar3;
//             }
//             if (((int)param_5[0x25] <= iVar2) && (iVar8 <= (int)param_5[0x26])) {
//               iVar7 = (int)*(short *)((int)r0 + 10);
//               iVar2 = (int)*(short *)((int)r0 + 0x16);
//               iVar8 = iVar7;
//               if (iVar2 < iVar7) {
//                 iVar8 = iVar2;
//                 iVar2 = iVar7;
//               }
//               iVar3 = (int)*(short *)((int)r0 + 0x22);
//               iVar7 = iVar3;
//               if ((iVar8 <= iVar3) && (iVar7 = iVar8, iVar2 < iVar3)) {
//                 iVar2 = iVar3;
//               }
//               iVar3 = (int)*(short *)((int)r0 + 0x2e);
//               iVar8 = iVar3;
//               if ((iVar7 <= iVar3) && (iVar8 = iVar7, iVar2 < iVar3)) {
//                 iVar2 = iVar3;
//               }
//               if (((int)param_5[0x27] <= iVar2) && (iVar8 <= (int)param_5[0x28])) {
//                 gte_stsz4((long *)r0_01,(long *)(param_5 + 0x18),(long *)(param_5 + 0x19),
//                           (long *)(param_5 + 0x1a));
//                 uVar9 = param_5[0x17];
//                 uVar4 = param_5[0x18];
//                 uVar1 = uVar9;
//                 if ((int)uVar4 < (int)uVar9) {
//                   uVar1 = uVar4;
//                   uVar4 = uVar9;
//                 }
//                 uVar5 = param_5[0x19];
//                 uVar9 = uVar5;
//                 if (((int)uVar1 <= (int)uVar5) && (uVar9 = uVar1, (int)uVar4 < (int)uVar5)) {
//                   uVar4 = uVar5;
//                 }
//                 uVar5 = param_5[0x1a];
//                 uVar1 = uVar5;
//                 if (((int)uVar9 <= (int)uVar5) && (uVar1 = uVar9, (int)uVar4 < (int)uVar5)) {
//                   uVar4 = uVar5;
//                 }
//                 if ((int)uVar1 <= (int)param_5[0x21]) {
//                   uVar1 = uVar4;
//                   if ((int)uVar4 < 0) {
//                     uVar1 = uVar4 + 3;
//                   }
//                   uVar9 = param_5[0x23];
//                   param_5[0x1e] = (int)uVar1 >> 2;
//                   pCVar10 = (CVECTOR *)(r0 + 1);
//                   if ((int)uVar9 < (int)uVar4) {
//                     uVar1 = param_5[0x17];
//                     gte_ldrgb(pCVar10);
//                     gte_ldIR0(uVar1 - uVar9);
//                     gte_dpcs();
//                     if ((int)uVar9 < (int)uVar1) {
//                       gte_strgb(pCVar10);
//                     }
//                     pCVar10 = (CVECTOR *)(r0 + 4);
//                     uVar1 = param_5[0x18];
//                     gte_ldrgb(pCVar10);
//                     gte_ldIR0(uVar1 - param_5[0x23]);
//                     gte_dpcs();
//                     if ((int)param_5[0x23] < (int)uVar1) {
//                       gte_strgb(pCVar10);
//                     }
//                     pCVar10 = (CVECTOR *)(r0 + 7);
//                     uVar1 = param_5[0x19];
//                     gte_ldrgb(pCVar10);
//                     gte_ldIR0(uVar1 - param_5[0x23]);
//                     gte_dpcs();
//                     if ((int)param_5[0x23] < (int)uVar1) {
//                       gte_strgb(pCVar10);
//                     }
//                     pCVar10 = (CVECTOR *)(r0 + 10);
//                     uVar1 = param_5[0x1a];
//                     gte_ldrgb(pCVar10);
//                     gte_ldIR0(uVar1 - param_5[0x23]);
//                     gte_dpcs();
//                     if ((int)param_5[0x23] < (int)uVar1) {
//                       gte_strgb(pCVar10);
//                     }
//                   }
//                   puVar6 = (uint *)(*(int *)(param_5[0x24] + 4) +
//                                    ((int)param_5[0x1e] >> (param_5[0x22] & 0x1f)) * 4);
//                   *r0 = *puVar6;
//                   *(undefined1 *)((int)r0 + 3) = 0xc;
//                   puVar11 = r0;
//                   puVar12 = param_3;
//                   do {
//                     uVar1 = puVar11[1];
//                     uVar4 = puVar11[2];
//                     uVar9 = puVar11[3];
//                     *puVar12 = *puVar11;
//                     puVar12[1] = uVar1;
//                     puVar12[2] = uVar4;
//                     puVar12[3] = uVar9;
//                     puVar11 = puVar11 + 4;
//                     puVar12 = puVar12 + 4;
//                   } while (puVar11 != r0 + 0xc);
//                   uVar1 = (uint)param_3 & 0xffffff;
//                   param_3 = param_3 + 0xd;
//                   *puVar12 = *puVar11;
//                   *puVar6 = uVar1;
//                 }
//               }
//             }
//           }
//         }
//       }
//       param_4 = param_4 + -1;
//       puVar13 = puVar13 + 0xb;
//     } while (param_4 != 0);
//   }
//   return param_3;
// }
