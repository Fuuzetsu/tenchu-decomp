#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8005e4bc", FUN_8005e4bc);

// triage: VERY-HARD — 103 insns, 5 GTE CMD — UN-SPLITTABLE (as can't assemble), 1 loop, 0 callees
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Toolchain gotchas: GTE command opcodes — `as` rejects them, so the per-function split does not even assemble; needs the GTE->.word pass first

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_8005e4bc(void)
//
// {
//   int in_v0;
//   uint uVar1;
//   u_long uVar2;
//   undefined1 uVar3;
//   SVECTOR *pSVar4;
//   uint uVar5;
//   SVECTOR *r0;
//   SVECTOR *r0_00;
//   SVECTOR *r0_01;
//   u_long uVar6;
//   SVECTOR *in_t0;
//   int iVar7;
//   int in_t2;
//   u_long *in_t3;
//   int in_t4;
//   int in_t5;
//   int iVar8;
//   int unaff_s0;
//   int in_t8;
//   int in_t9;
//
//   iVar8 = 1;
//   if (in_v0 != 0x608) {
//     iVar8 = in_v0;
//   }
//   do {
//     uVar5 = (uint)*(ushort *)(in_t5 + 0x14);
//     gte_ldv0((SVECTOR *)((uint)*(ushort *)(in_t5 + 0x1a) * 8 + in_t4));
//     gte_rtps_b();
//     r0 = (SVECTOR *)(uVar5 * 8 + in_t4);
//     r0_00 = (SVECTOR *)((uint)*(ushort *)(in_t5 + 0x16) * 8 + in_t4);
//     r0_01 = (SVECTOR *)((uint)*(ushort *)(in_t5 + 0x18) * 8 + in_t4);
//     gte_ldv1(r0_00);
//     gte_ldv2(r0_01);
//     uVar6 = gte_stSXY2();
//     gte_ldv0(r0);
//     uVar5 = 0x1c66000;
//     gte_rtpt_b();
//     if (unaff_s0 != 0) {
//       *(u_long **)in_t0 = in_t3;
//       *(undefined1 *)((int)&in_t0->vy + 1) = 0;
//       gte_strgb3_g3(in_t3);
//       *(char *)((int)in_t3 + 7) = (char)unaff_s0;
//       in_t3 = in_t3 + 9;
//     }
//     uVar1 = gte_stFLAG();
//     unaff_s0 = 0;
//     iVar7 = gte_stMAC1();
//     in_t0 = (SVECTOR *)(iVar7 + 0x2000);
//     if ((uVar1 & uVar5) == 0) {
//       in_t0 = (SVECTOR *)((uint)in_t0 >> 0xe);
//       gte_nclip_b();
//       gte_ldRGB(in_t5 + 0x10);
//       if (in_t0 == (SVECTOR *)0x0) {
//         read_sz_fifo3(r0,r0_00,r0_01);
//         pSVar4 = (SVECTOR *)gte_stSZ0();
//         in_t0 = r0;
//         if ((int)r0 - (int)r0_00 < 0) {
//           in_t0 = r0_00;
//         }
//         gte_dpcs_b();
//         iVar7 = gte_stMAC0();
//         if ((int)r0_01 - (int)pSVar4 < 0) {
//           r0_01 = pSVar4;
//         }
//         if (0 < iVar7) {
//           if ((int)in_t0 - (int)r0_01 < 0) {
//             in_t0 = r0_01;
//           }
//           in_t0 = (SVECTOR *)((uint)in_t0 >> 4);
//           iVar7 = (int)in_t0 - in_t9;
//           if ((0 < (int)in_t0 - in_t8) && (in_t0 = (SVECTOR *)((int)in_t0 * 4), iVar7 < 0)) {
//             gte_stRGB2();
//             gte_ldRGB0(in_t5 + 4);
//             gte_ldRGB1(in_t5 + 8);
//             gte_ldRGB2(in_t5 + 0xc);
//             in_t0 = (SVECTOR *)((int)&in_t0->vx + in_t2);
//             uVar2._0_2_ = in_t0->vx;
//             uVar2._2_2_ = in_t0->vy;
//             gte_dpct_b();
//             uVar3 = 8;
//             unaff_s0 = 0x38;
//             gte_stsxy3_g3(in_t3);
//             in_t3[8] = uVar6;
//             *in_t3 = uVar2;
//             *(undefined1 *)((int)in_t3 + 3) = uVar3;
//           }
//         }
//       }
//     }
//     iVar8 = iVar8 + -1;
//     in_t5 = in_t5 + 0x1c;
//   } while (0 < iVar8);
//   if (unaff_s0 != 0) {
//     *(u_long **)in_t0 = in_t3;
//     *(undefined1 *)((int)&in_t0->vy + 1) = 0;
//     gte_strgb3_g3(in_t3);
//     *(char *)((int)in_t3 + 7) = (char)unaff_s0;
//   }
//   return;
// }
