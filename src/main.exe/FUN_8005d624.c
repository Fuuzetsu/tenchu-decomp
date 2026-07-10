#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8005d624", FUN_8005d624);

// triage: VERY-HARD — 80 insns, 3 GTE CMD — UN-SPLITTABLE (as can't assemble), 1 loop, 0 callees
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Toolchain gotchas: GTE command opcodes — `as` rejects them, so the per-function split does not even assemble; needs the GTE->.word pass first

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_8005d624(void)
//
// {
//   int in_v0;
//   uint uVar1;
//   int iVar2;
//   uint uVar3;
//   SVECTOR *r0;
//   SVECTOR *r1;
//   SVECTOR *r2;
//   u_long *in_t0;
//   int in_t2;
//   u_long *in_t3;
//   int in_t4;
//   int in_t5;
//   int iVar4;
//   int unaff_s0;
//   int in_t9;
//
//   iVar4 = 1;
//   if (in_v0 != 0x506) {
//     iVar4 = in_v0;
//   }
//   do {
//     r0 = (SVECTOR *)((uint)*(ushort *)(in_t5 + 0x10) * 8 + in_t4);
//     r1 = (SVECTOR *)((uint)*(ushort *)(in_t5 + 0x12) * 8 + in_t4);
//     r2 = (SVECTOR *)((uint)*(ushort *)(in_t5 + 0x14) * 8 + in_t4);
//     gte_ldv3(r0,r1,r2);
//     uVar3 = 0x1c66000;
//     gte_rtpt_b();
//     if (unaff_s0 != 0) {
//       *in_t0 = (u_long)in_t3;
//       *(undefined1 *)((int)in_t0 + 3) = 0;
//       gte_strgb3_g3(in_t3);
//       *(char *)((int)in_t3 + 7) = (char)unaff_s0;
//       in_t3 = in_t3 + 7;
//     }
//     uVar1 = gte_stFLAG();
//     unaff_s0 = 0;
//     if ((uVar1 & uVar3) == 0) {
//       gte_nclip_b();
//       read_sz_fifo3(r0,r1,r2);
//       uVar3 = (int)&r2->vx + (int)&r1->vx + (int)&r0->vx;
//       iVar2 = gte_stMAC0();
//       in_t0 = (u_long *)((uVar3 >> 4) + (uVar3 >> 2) >> 4);
//       if (((0 < iVar2) && (iVar2 = (int)in_t0 - in_t9, in_t0 != (u_long *)0x0)) &&
//          (in_t0 = (u_long *)((int)in_t0 * 4), iVar2 < 0)) {
//         gte_ldRGB0(in_t5 + 4);
//         gte_ldRGB1(in_t5 + 8);
//         gte_ldRGB2(in_t5 + 0xc);
//         in_t0 = (u_long *)((int)in_t0 + in_t2);
//         gte_dpct_b();
//         *in_t3 = *in_t0;
//         *(undefined1 *)((int)in_t3 + 3) = 6;
//         gte_stsxy3_g3(in_t3);
//         unaff_s0 = 0x30;
//       }
//     }
//     iVar4 = iVar4 + -1;
//     in_t5 = in_t5 + 0x18;
//   } while (0 < iVar4);
//   if (unaff_s0 != 0) {
//     *in_t0 = (u_long)in_t3;
//     *(undefined1 *)((int)in_t0 + 3) = 0;
//     gte_strgb3_g3(in_t3);
//     *(char *)((int)in_t3 + 7) = (char)unaff_s0;
//   }
//   return;
// }
