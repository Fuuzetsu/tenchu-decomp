#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8005e01c", FUN_8005e01c);

// triage: VERY-HARD — 110 insns, 4 GTE CMD — UN-SPLITTABLE (as can't assemble), 1 loop, 0 callees, ~0.04 to character_balma_around_main_routine_
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Toolchain gotchas: GTE command opcodes — `as` rejects them, so the per-function split does not even assemble; needs the GTE->.word pass first

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_8005e01c(void)
//
// {
//   int in_v0;
//   undefined1 uVar1;
//   SVECTOR *pSVar2;
//   uint uVar3;
//   SVECTOR *r0;
//   SVECTOR *r0_00;
//   SVECTOR *r0_01;
//   u_long uVar4;
//   u_long uVar5;
//   u_long uVar6;
//   SVECTOR *in_t0;
//   int iVar7;
//   int in_t2;
//   u_long *in_t3;
//   int in_t4;
//   int in_t5;
//   int iVar8;
//   int iVar9;
//   int unaff_s0;
//   int in_t8;
//   int in_t9;
//
//   iVar9 = 1;
//   if (in_v0 != 0x709) {
//     iVar9 = in_v0;
//   }
//   do {
//     iVar8 = in_t5;
//     uVar3 = (uint)*(ushort *)(iVar8 + 0x18);
//     gte_ldv0((SVECTOR *)((uint)*(ushort *)(iVar8 + 0x1e) * 8 + in_t4));
//     gte_rtps_b();
//     r0 = (SVECTOR *)(uVar3 * 8 + in_t4);
//     r0_00 = (SVECTOR *)((uint)*(ushort *)(iVar8 + 0x1a) * 8 + in_t4);
//     r0_01 = (SVECTOR *)((uint)*(ushort *)(iVar8 + 0x1c) * 8 + in_t4);
//     gte_ldv1(r0_00);
//     gte_ldv2(r0_01);
//     uVar5 = gte_stSXY2();
//     gte_ldv0(r0);
//     gte_rtpt_b();
//     if (unaff_s0 != 0) {
//       *(u_long **)in_t0 = in_t3;
//       *(undefined1 *)((int)&in_t0->vy + 1) = 0;
//       gte_stRGB2();
//       *(char *)((int)in_t3 + 7) = (char)unaff_s0;
//       r0 = *(SVECTOR **)(iVar8 + -0x18);
//       r0_00 = *(SVECTOR **)(iVar8 + -0x14);
//       r0_01 = *(SVECTOR **)(iVar8 + -0x10);
//       in_t3[3] = *(u_long *)(iVar8 + -0x1c);
//       in_t3[5] = (u_long)r0;
//       in_t3[7] = (u_long)r0_00;
//       in_t3[9] = (u_long)r0_01;
//       in_t3 = in_t3 + 10;
//     }
//     uVar3 = gte_stFLAG();
//     unaff_s0 = 0;
//     iVar7 = gte_stMAC1();
//     in_t0 = (SVECTOR *)(iVar7 + 0x2000);
//     if ((uVar3 & 0x1c66000) == 0) {
//       in_t0 = (SVECTOR *)((uint)in_t0 >> 0xe);
//       gte_nclip_b();
//       read_sz_fifo3(r0,r0_00,r0_01);
//       if (in_t0 == (SVECTOR *)0x0) {
//         pSVar2 = (SVECTOR *)gte_stSZ0();
//         in_t0 = r0;
//         if ((int)r0 - (int)r0_00 < 0) {
//           in_t0 = r0_00;
//         }
//         iVar7 = gte_stMAC0();
//         if ((int)r0_01 - (int)pSVar2 < 0) {
//           r0_01 = pSVar2;
//         }
//         if (0 < iVar7) {
//           if ((int)in_t0 - (int)r0_01 < 0) {
//             in_t0 = r0_01;
//           }
//           in_t0 = (SVECTOR *)((uint)in_t0 >> 4);
//           iVar7 = (int)in_t0 - in_t9;
//           if ((0 < (int)in_t0 - in_t8) && (in_t0 = (SVECTOR *)((int)in_t0 * 4), iVar7 < 0)) {
//             gte_ldRGB(iVar8 + 0x14);
//             in_t0 = (SVECTOR *)((int)&in_t0->vx + in_t2);
//             uVar4._0_2_ = in_t0->vx;
//             uVar4._2_2_ = in_t0->vy;
//             gte_dpcs_b();
//             uVar1 = 9;
//             unaff_s0 = 0x2c;
//             gte_stsxy3_g3(in_t3);
//             in_t3[8] = uVar5;
//             *in_t3 = uVar4;
//             *(undefined1 *)((int)in_t3 + 3) = uVar1;
//           }
//         }
//       }
//     }
//     iVar9 = iVar9 + -1;
//     in_t5 = iVar8 + 0x20;
//   } while (0 < iVar9);
//   if (unaff_s0 != 0) {
//     *(u_long **)in_t0 = in_t3;
//     *(undefined1 *)((int)&in_t0->vy + 1) = 0;
//     gte_stRGB2();
//     *(char *)((int)in_t3 + 7) = (char)unaff_s0;
//     uVar5 = *(u_long *)(iVar8 + 8);
//     uVar4 = *(u_long *)(iVar8 + 0xc);
//     uVar6 = *(u_long *)(iVar8 + 0x10);
//     in_t3[3] = *(u_long *)(iVar8 + 4);
//     in_t3[5] = uVar5;
//     in_t3[7] = uVar4;
//     in_t3[9] = uVar6;
//   }
//   return;
// }
