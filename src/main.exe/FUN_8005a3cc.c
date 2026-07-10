#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8005a3cc", FUN_8005a3cc);

// triage: VERY-HARD — 246 insns, 5 GTE CMD — UN-SPLITTABLE (as can't assemble), 2 loop, 0 callees, ~0.04 to PlayerOption
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Toolchain gotchas: GTE command opcodes — `as` rejects them, so the per-function split does not even assemble; needs the GTE->.word pass first

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// uint * FUN_8005a3cc(int param_1,int param_2,uint *param_3,int param_4,int param_5)
//
// {
//   uint *puVar1;
//   int iVar2;
//   int iVar3;
//   int iVar4;
//   uint *puVar5;
//   int iVar6;
//   CVECTOR *pCVar7;
//   uint *puVar8;
//   uint *puVar9;
//   uint *puVar10;
//   uint *r0;
//   uint *puVar11;
//   long *r1;
//   long *r0_00;
//   uint uVar12;
//   uint uVar13;
//   uint uVar14;
//   int iVar15;
//   undefined4 uVar16;
//
//   r0 = (uint *)(param_5 + 0x34);
//   if (param_4 != 0) {
//     iVar15 = param_5 + 0x38;
//     uVar16 = 0x34;
//     r0_00 = (long *)(param_5 + 0x5c);
//     r1 = (long *)(param_5 + 0x60);
//     puVar11 = (uint *)(param_1 + 0x18);
//     do {
//       gte_ldv3((SVECTOR *)((uint)(ushort)puVar11[1] * 8 + param_2),
//                (SVECTOR *)((uint)*(ushort *)((int)puVar11 + 6) * 8 + param_2),
//                (SVECTOR *)((uint)(ushort)puVar11[2] * 8 + param_2));
//       gte_rtpt();
//       r0[3] = puVar11[-5];
//       r0[6] = puVar11[-4];
//       r0[9] = puVar11[-3];
//       r0[1] = puVar11[-2];
//       *(char *)(iVar15 + 3) = (char)uVar16;
//       gte_stflg((long *)(param_5 + 0x74));
//       if (-1 < *(int *)(param_5 + 0x74)) {
//         gte_nclip();
//         r0[4] = puVar11[-1];
//         r0[7] = *puVar11;
//         gte_stopz((long *)(param_5 + 0x80));
//         if (0 < *(int *)(param_5 + 0x80)) {
//           gte_stsxy3_gt3(r0);
//           iVar6 = (int)(short)r0[2];
//           iVar3 = (int)(short)r0[5];
//           iVar2 = iVar6;
//           if (iVar3 < iVar6) {
//             iVar2 = iVar3;
//             iVar3 = iVar6;
//           }
//           iVar4 = (int)(short)r0[8];
//           iVar6 = iVar4;
//           if ((iVar2 <= iVar4) && (iVar6 = iVar2, iVar3 < iVar4)) {
//             iVar3 = iVar4;
//           }
//           if ((*(int *)(param_5 + 0x94) <= iVar3) && (iVar6 <= *(int *)(param_5 + 0x98))) {
//             iVar6 = (int)*(short *)((int)r0 + 10);
//             iVar3 = (int)*(short *)((int)r0 + 0x16);
//             iVar2 = iVar6;
//             if (iVar3 < iVar6) {
//               iVar2 = iVar3;
//               iVar3 = iVar6;
//             }
//             iVar4 = (int)*(short *)((int)r0 + 0x22);
//             iVar6 = iVar4;
//             if ((iVar2 <= iVar4) && (iVar6 = iVar2, iVar3 < iVar4)) {
//               iVar3 = iVar4;
//             }
//             if ((*(int *)(param_5 + 0x9c) <= iVar3) && (iVar6 <= *(int *)(param_5 + 0xa0))) {
//               gte_stsz3(r0_00,r1,(long *)(param_5 + 100));
//               iVar6 = *(int *)(param_5 + 0x5c);
//               iVar3 = *(int *)(param_5 + 0x60);
//               iVar2 = iVar6;
//               if (iVar3 < iVar6) {
//                 iVar2 = iVar3;
//                 iVar3 = iVar6;
//               }
//               iVar4 = *(int *)(param_5 + 100);
//               iVar6 = iVar4;
//               if ((iVar2 <= iVar4) && (iVar6 = iVar2, iVar3 < iVar4)) {
//                 iVar3 = iVar4;
//               }
//               iVar2 = iVar3;
//               if (iVar3 < 0) {
//                 iVar2 = iVar3 + 3;
//               }
//               *(int *)(param_5 + 0x78) = iVar2 >> 2;
//               if (iVar6 <= *(int *)(param_5 + 0x84)) {
//                 iVar2 = *(int *)(param_5 + 0x8c);
//                 pCVar7 = (CVECTOR *)(r0 + 1);
//                 if (iVar2 < iVar3) {
//                   iVar3 = *(int *)(param_5 + 0x5c);
//                   gte_ldrgb(pCVar7);
//                   gte_ldIR0(iVar3 - iVar2);
//                   gte_dpcs();
//                   if (iVar2 < iVar3) {
//                     gte_strgb(pCVar7);
//                   }
//                   pCVar7 = (CVECTOR *)(r0 + 4);
//                   iVar2 = *(int *)(param_5 + 0x60);
//                   gte_ldrgb(pCVar7);
//                   gte_ldIR0(iVar2 - *(int *)(param_5 + 0x8c));
//                   gte_dpcs();
//                   if (*(int *)(param_5 + 0x8c) < iVar2) {
//                     gte_strgb(pCVar7);
//                   }
//                   pCVar7 = (CVECTOR *)(r0 + 7);
//                   iVar2 = *(int *)(param_5 + 100);
//                   gte_ldrgb(pCVar7);
//                   gte_ldIR0(iVar2 - *(int *)(param_5 + 0x8c));
//                   gte_dpcs();
//                   if (*(int *)(param_5 + 0x8c) < iVar2) {
//                     gte_strgb(pCVar7);
//                   }
//                 }
//                 puVar5 = (uint *)(*(int *)(*(int *)(param_5 + 0x90) + 4) +
//                                  (*(int *)(param_5 + 0x78) >> (*(uint *)(param_5 + 0x88) & 0x1f)) *
//                                  4);
//                 *r0 = *puVar5;
//                 *(undefined1 *)((int)r0 + 3) = 9;
//                 puVar9 = r0;
//                 puVar1 = param_3;
//                 do {
//                   puVar10 = puVar1;
//                   puVar8 = puVar9;
//                   uVar12 = puVar8[1];
//                   uVar13 = puVar8[2];
//                   uVar14 = puVar8[3];
//                   *puVar10 = *puVar8;
//                   puVar10[1] = uVar12;
//                   puVar10[2] = uVar13;
//                   puVar10[3] = uVar14;
//                   puVar9 = puVar8 + 4;
//                   puVar1 = puVar10 + 4;
//                 } while (puVar9 != r0 + 8);
//                 uVar12 = (uint)param_3 & 0xffffff;
//                 param_3 = param_3 + 10;
//                 uVar13 = puVar8[5];
//                 puVar10[4] = *puVar9;
//                 puVar10[5] = uVar13;
//                 *puVar5 = uVar12;
//               }
//             }
//           }
//         }
//       }
//       param_4 = param_4 + -1;
//       puVar11 = puVar11 + 9;
//     } while (param_4 != 0);
//   }
//   return param_3;
// }
