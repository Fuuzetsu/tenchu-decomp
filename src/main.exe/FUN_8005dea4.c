#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8005dea4", FUN_8005dea4);

// triage: VERY-HARD — 94 insns, 4 GTE CMD — UN-SPLITTABLE (as can't assemble), 1 loop, 0 callees
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Toolchain gotchas: GTE command opcodes — `as` rejects them, so the per-function split does not even assemble; needs the GTE->.word pass first

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_8005dea4(void)
//
// {
//   int in_v0;
//   u_long uVar1;
//   undefined1 uVar2;
//   SVECTOR *pSVar3;
//   uint uVar4;
//   SVECTOR *r0;
//   SVECTOR *r0_00;
//   SVECTOR *r0_01;
//   u_long uVar5;
//   SVECTOR *in_t0;
//   int iVar6;
//   int in_t2;
//   u_long *in_t3;
//   int in_t4;
//   int in_t5;
//   int iVar7;
//   int unaff_s0;
//   int in_t8;
//   int in_t9;
//
//   iVar7 = 1;
//   if (in_v0 != 0x305) {
//     iVar7 = in_v0;
//   }
//   do {
//     uVar4 = (uint)*(ushort *)(in_t5 + 8);
//     gte_ldv0((SVECTOR *)((uint)*(ushort *)(in_t5 + 0xe) * 8 + in_t4));
//     gte_rtps_b();
//     r0 = (SVECTOR *)(uVar4 * 8 + in_t4);
//     r0_00 = (SVECTOR *)((uint)*(ushort *)(in_t5 + 10) * 8 + in_t4);
//     r0_01 = (SVECTOR *)((uint)*(ushort *)(in_t5 + 0xc) * 8 + in_t4);
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
//       in_t3 = in_t3 + 6;
//     }
//     uVar4 = gte_stFLAG();
//     unaff_s0 = 0;
//     iVar6 = gte_stMAC1();
//     in_t0 = (SVECTOR *)(iVar6 + 0x2000);
//     if ((uVar4 & 0x1c66000) == 0) {
//       in_t0 = (SVECTOR *)((uint)in_t0 >> 0xe);
//       gte_nclip_b();
//       read_sz_fifo3(r0,r0_00,r0_01);
//       if (in_t0 == (SVECTOR *)0x0) {
//         pSVar3 = (SVECTOR *)gte_stSZ0();
//         in_t0 = r0;
//         if ((int)r0 - (int)r0_00 < 0) {
//           in_t0 = r0_00;
//         }
//         iVar6 = gte_stMAC0();
//         if ((int)r0_01 - (int)pSVar3 < 0) {
//           r0_01 = pSVar3;
//         }
//         if (0 < iVar6) {
//           if ((int)in_t0 - (int)r0_01 < 0) {
//             in_t0 = r0_01;
//           }
//           in_t0 = (SVECTOR *)((uint)in_t0 >> 4);
//           iVar6 = (int)in_t0 - in_t9;
//           if ((0 < (int)in_t0 - in_t8) && (in_t0 = (SVECTOR *)((int)in_t0 * 4), iVar6 < 0)) {
//             gte_ldRGB(in_t5 + 4);
//             in_t0 = (SVECTOR *)((int)&in_t0->vx + in_t2);
//             uVar1._0_2_ = in_t0->vx;
//             uVar1._2_2_ = in_t0->vy;
//             gte_dpcs_b();
//             uVar2 = 5;
//             unaff_s0 = 0x28;
//             gte_stsxy3_f3(in_t3);
//             in_t3[5] = uVar5;
//             *in_t3 = uVar1;
//             *(undefined1 *)((int)in_t3 + 3) = uVar2;
//           }
//         }
//       }
//     }
//     iVar7 = iVar7 + -1;
//     in_t5 = in_t5 + 0x10;
//   } while (0 < iVar7);
//   if (unaff_s0 != 0) {
//     *(u_long **)in_t0 = in_t3;
//     *(undefined1 *)((int)&in_t0->vy + 1) = 0;
//     gte_stRGB2();
//     *(char *)((int)in_t3 + 7) = (char)unaff_s0;
//   }
//   return;
// }
