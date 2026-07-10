#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8005d8d4", FUN_8005d8d4);

// triage: VERY-HARD — 91 insns, 5 GTE CMD — UN-SPLITTABLE (as can't assemble), 1 loop, 0 callees
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Toolchain gotchas: GTE command opcodes — `as` rejects them, so the per-function split does not even assemble; needs the GTE->.word pass first

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_8005d8d4(void)
//
// {
//   int in_v0;
//   uint uVar1;
//   int iVar2;
//   u_long uVar3;
//   undefined1 uVar4;
//   uint uVar5;
//   SVECTOR *r0;
//   SVECTOR *r1;
//   SVECTOR *r2;
//   SVECTOR *r0_00;
//   u_long *in_t0;
//   int in_t2;
//   u_long *in_t3;
//   int in_t4;
//   int in_t5;
//   int iVar6;
//   int unaff_s0;
//   int in_t9;
//
//   iVar6 = 1;
//   if (in_v0 != 0x608) {
//     iVar6 = in_v0;
//   }
//   do {
//     r0 = (SVECTOR *)((uint)*(ushort *)(in_t5 + 0x14) * 8 + in_t4);
//     r1 = (SVECTOR *)((uint)*(ushort *)(in_t5 + 0x16) * 8 + in_t4);
//     r2 = (SVECTOR *)((uint)*(ushort *)(in_t5 + 0x18) * 8 + in_t4);
//     gte_ldv3(r0,r1,r2);
//     gte_stSXY2();
//     uVar5 = 0x1c66000;
//     gte_rtpt_b();
//     r0_00 = (SVECTOR *)((uint)*(ushort *)(in_t5 + 0x1a) * 8 + in_t4);
//     if (unaff_s0 != 0) {
//       *in_t0 = (u_long)in_t3;
//       *(undefined1 *)((int)in_t0 + 3) = 0;
//       gte_strgb3_g3(in_t3);
//       *(char *)((int)in_t3 + 7) = (char)unaff_s0;
//       in_t3 = in_t3 + 9;
//     }
//     uVar1 = gte_stFLAG();
//     unaff_s0 = 0;
//     if ((uVar1 & uVar5) == 0) {
//       gte_nclip_b();
//       gte_ldRGB(in_t5 + 0x10);
//       read_sz_fifo3(r0,r1,r2);
//       uVar5 = (int)&r2->vx + (int)&r1->vx + (int)&r0->vx;
//       gte_dpcs_b();
//       iVar2 = gte_stMAC0();
//       in_t0 = (u_long *)((uVar5 >> 4) + (uVar5 >> 2) >> 4);
//       if (((0 < iVar2) && (iVar2 = (int)in_t0 - in_t9, in_t0 != (u_long *)0x0)) &&
//          (in_t0 = (u_long *)((int)in_t0 * 4), iVar2 < 0)) {
//         gte_stRGB2();
//         gte_ldRGB0(in_t5 + 4);
//         gte_ldRGB1(in_t5 + 8);
//         gte_ldRGB2(in_t5 + 0xc);
//         in_t0 = (u_long *)((int)in_t0 + in_t2);
//         uVar3 = *in_t0;
//         gte_dpct_b();
//         uVar4 = 8;
//         unaff_s0 = 0x38;
//         gte_stsxy3_g3(in_t3);
//         gte_ldv0(r0_00);
//         *in_t3 = uVar3;
//         *(undefined1 *)((int)in_t3 + 3) = uVar4;
//         gte_rtps_b();
//       }
//     }
//     iVar6 = iVar6 + -1;
//     in_t5 = in_t5 + 0x1c;
//   } while (0 < iVar6);
//   if (unaff_s0 != 0) {
//     *in_t0 = (u_long)in_t3;
//     *(undefined1 *)((int)in_t0 + 3) = 0;
//     gte_strgb3_g3(in_t3);
//     *(char *)((int)in_t3 + 7) = (char)unaff_s0;
//     gte_stSXY2();
//   }
//   return;
// }
