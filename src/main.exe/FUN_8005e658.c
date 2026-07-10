#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8005e658", FUN_8005e658);

// triage: VERY-HARD — 119 insns, 5 GTE CMD — UN-SPLITTABLE (as can't assemble), 1 loop, 0 callees, ~0.04 to DeleteConflict
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Toolchain gotchas: GTE command opcodes — `as` rejects them, so the per-function split does not even assemble; needs the GTE->.word pass first

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_8005e658(void)
//
// {
//   int in_v0;
//   uint uVar1;
//   undefined1 uVar2;
//   SVECTOR *pSVar3;
//   uint uVar4;
//   SVECTOR *r0;
//   SVECTOR *r0_00;
//   u_long uVar5;
//   SVECTOR *r0_01;
//   u_long uVar6;
//   u_long uVar7;
//   SVECTOR *in_t0;
//   int iVar8;
//   int in_t2;
//   u_long *in_t3;
//   int in_t4;
//   int in_t5;
//   int iVar9;
//   int unaff_s0;
//   int in_t8;
//   int in_t9;
//
//   iVar9 = 1;
//   if (in_v0 != 0xa0c) {
//     iVar9 = in_v0;
//   }
//   do {
//     uVar4 = (uint)*(ushort *)(in_t5 + 0x24);
//     gte_ldv0((SVECTOR *)((uint)*(ushort *)(in_t5 + 0x2a) * 8 + in_t4));
//     gte_rtps_b();
//     r0 = (SVECTOR *)(uVar4 * 8 + in_t4);
//     r0_00 = (SVECTOR *)((uint)*(ushort *)(in_t5 + 0x26) * 8 + in_t4);
//     r0_01 = (SVECTOR *)((uint)*(ushort *)(in_t5 + 0x28) * 8 + in_t4);
//     gte_ldv1(r0_00);
//     gte_ldv2(r0_01);
//     uVar7 = gte_stSXY2();
//     gte_ldv0(r0);
//     uVar4 = 0x1c66000;
//     gte_rtpt_b();
//     if (unaff_s0 != 0) {
//       *(u_long **)in_t0 = in_t3;
//       *(undefined1 *)((int)&in_t0->vy + 1) = 0;
//       gte_strgb3_gt3(in_t3);
//       *(char *)((int)in_t3 + 7) = (char)unaff_s0;
//       r0 = *(SVECTOR **)(in_t5 + -0x24);
//       r0_00 = *(SVECTOR **)(in_t5 + -0x20);
//       r0_01 = *(SVECTOR **)(in_t5 + -0x1c);
//       in_t3[3] = *(u_long *)(in_t5 + -0x28);
//       in_t3[6] = (u_long)r0;
//       in_t3[9] = (u_long)r0_00;
//       in_t3[0xc] = (u_long)r0_01;
//       in_t3 = in_t3 + 0xd;
//     }
//     uVar1 = gte_stFLAG();
//     unaff_s0 = 0;
//     iVar8 = gte_stMAC1();
//     in_t0 = (SVECTOR *)(iVar8 + 0x2000);
//     if ((uVar1 & uVar4) == 0) {
//       in_t0 = (SVECTOR *)((uint)in_t0 >> 0xe);
//       gte_nclip_b();
//       gte_ldRGB(in_t5 + 0x20);
//       if (in_t0 == (SVECTOR *)0x0) {
//         read_sz_fifo3(r0,r0_00,r0_01);
//         pSVar3 = (SVECTOR *)gte_stSZ0();
//         in_t0 = r0;
//         if ((int)r0 - (int)r0_00 < 0) {
//           in_t0 = r0_00;
//         }
//         gte_dpcs_b();
//         iVar8 = gte_stMAC0();
//         if ((int)r0_01 - (int)pSVar3 < 0) {
//           r0_01 = pSVar3;
//         }
//         if (0 < iVar8) {
//           if ((int)in_t0 - (int)r0_01 < 0) {
//             in_t0 = r0_01;
//           }
//           in_t0 = (SVECTOR *)((uint)in_t0 >> 4);
//           iVar8 = (int)in_t0 - in_t9;
//           if ((0 < (int)in_t0 - in_t8) && (in_t0 = (SVECTOR *)((int)in_t0 * 4), iVar8 < 0)) {
//             gte_stRGB2();
//             gte_ldRGB0(in_t5 + 0x14);
//             gte_ldRGB1(in_t5 + 0x18);
//             gte_ldRGB2(in_t5 + 0x1c);
//             in_t0 = (SVECTOR *)((int)&in_t0->vx + in_t2);
//             uVar5._0_2_ = in_t0->vx;
//             uVar5._2_2_ = in_t0->vy;
//             gte_dpct_b();
//             uVar2 = 0xc;
//             unaff_s0 = 0x3c;
//             gte_stsxy3_gt3(in_t3);
//             in_t3[0xb] = uVar7;
//             *in_t3 = uVar5;
//             *(undefined1 *)((int)in_t3 + 3) = uVar2;
//           }
//         }
//       }
//     }
//     iVar9 = iVar9 + -1;
//     in_t5 = in_t5 + 0x2c;
//   } while (0 < iVar9);
//   if (unaff_s0 != 0) {
//     *(u_long **)in_t0 = in_t3;
//     *(undefined1 *)((int)&in_t0->vy + 1) = 0;
//     gte_strgb3_gt3(in_t3);
//     *(char *)((int)in_t3 + 7) = (char)unaff_s0;
//     uVar7 = *(u_long *)(in_t5 + -0x24);
//     uVar5 = *(u_long *)(in_t5 + -0x20);
//     uVar6 = *(u_long *)(in_t5 + -0x1c);
//     in_t3[3] = *(u_long *)(in_t5 + -0x28);
//     in_t3[6] = uVar7;
//     in_t3[9] = uVar5;
//     in_t3[0xc] = uVar6;
//   }
//   return;
// }
