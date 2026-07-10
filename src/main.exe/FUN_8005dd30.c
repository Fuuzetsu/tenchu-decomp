#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8005dd30", FUN_8005dd30);

// triage: VERY-HARD — 93 insns, 3 GTE CMD — UN-SPLITTABLE (as can't assemble), 1 loop, 0 callees, ~0.04 to character_balma_around_main_routine_
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Toolchain gotchas: GTE command opcodes — `as` rejects them, so the per-function split does not even assemble; needs the GTE->.word pass first

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_8005dd30(void)
//
// {
//   int in_v0;
//   uint uVar1;
//   SVECTOR *r0;
//   SVECTOR *r1;
//   u_long uVar2;
//   SVECTOR *r2;
//   u_long uVar3;
//   u_long *in_t0;
//   int iVar4;
//   int in_t2;
//   u_long *in_t3;
//   int in_t4;
//   int in_t5;
//   int iVar5;
//   int iVar6;
//   int unaff_s0;
//   int in_t8;
//   int in_t9;
//
//   iVar6 = 1;
//   if (in_v0 != 0x607) {
//     iVar6 = in_v0;
//   }
//   do {
//     iVar5 = in_t5;
//     r0 = (SVECTOR *)((uint)*(ushort *)(iVar5 + 0x14) * 8 + in_t4);
//     r1 = (SVECTOR *)((uint)*(ushort *)(iVar5 + 0x16) * 8 + in_t4);
//     r2 = (SVECTOR *)((uint)*(ushort *)(iVar5 + 0x18) * 8 + in_t4);
//     gte_ldv3(r0,r1,r2);
//     gte_rtpt_b();
//     if (unaff_s0 != 0) {
//       *in_t0 = (u_long)in_t3;
//       *(undefined1 *)((int)in_t0 + 3) = 0;
//       gte_stRGB2();
//       *(char *)((int)in_t3 + 7) = (char)unaff_s0;
//       r0 = *(SVECTOR **)(iVar5 + -0x18);
//       r1 = *(SVECTOR **)(iVar5 + -0x14);
//       r2 = *(SVECTOR **)(iVar5 + -0x10);
//       in_t3[3] = (u_long)r0;
//       in_t3[5] = (u_long)r1;
//       in_t3[7] = (u_long)r2;
//       in_t3 = in_t3 + 8;
//     }
//     uVar1 = gte_stFLAG();
//     unaff_s0 = 0;
//     iVar4 = gte_stMAC1();
//     in_t0 = (u_long *)(iVar4 + 0x2000);
//     if ((uVar1 & 0x1c66000) == 0) {
//       in_t0 = (u_long *)((uint)in_t0 >> 0xe);
//       gte_nclip_b();
//       read_sz_fifo3(r0,r1,r2);
//       if (in_t0 == (u_long *)0x0) {
//         if ((int)r0 - (int)r1 < 0) {
//           r0 = r1;
//         }
//         iVar4 = gte_stMAC0();
//         if ((int)r0 - (int)r2 < 0) {
//           r0 = r2;
//         }
//         in_t0 = (u_long *)((uint)r0 >> 4);
//         if (((0 < iVar4) && (iVar4 = (int)in_t0 - in_t9, 0 < (int)in_t0 - in_t8)) &&
//            (in_t0 = (u_long *)((int)in_t0 * 4), iVar4 < 0)) {
//           gte_ldRGB(iVar5 + 0x10);
//           in_t0 = (u_long *)((int)in_t0 + in_t2);
//           gte_dpcs_b();
//           *in_t3 = *in_t0;
//           *(undefined1 *)((int)in_t3 + 3) = 7;
//           gte_stsxy3_g3(in_t3);
//           unaff_s0 = 0x24;
//         }
//       }
//     }
//     iVar6 = iVar6 + -1;
//     in_t5 = iVar5 + 0x1c;
//   } while (0 < iVar6);
//   if (unaff_s0 != 0) {
//     *in_t0 = (u_long)in_t3;
//     *(undefined1 *)((int)in_t0 + 3) = 0;
//     gte_stRGB2();
//     *(char *)((int)in_t3 + 7) = (char)unaff_s0;
//     uVar2 = *(u_long *)(iVar5 + 8);
//     uVar3 = *(u_long *)(iVar5 + 0xc);
//     in_t3[3] = *(u_long *)(iVar5 + 4);
//     in_t3[5] = uVar2;
//     in_t3[7] = uVar3;
//   }
//   return;
// }
