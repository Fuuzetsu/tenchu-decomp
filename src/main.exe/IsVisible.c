#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int IsVisible(long x, long y, long z, long s);
 *     WORLD.C:685, 63 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       long x
 *     param $a1       long y
 *     param $a2       long z
 *     param $a3       long s
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/IsVisible", IsVisible);

// triage: MEDIUM — 130 insns, mul/div, 1 loop, 2 callees, ~0.07 to cd_control
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// int IsVisible(long x,long y,long z,long s)
//
// {
//   int iVar1;
//   int iVar2;
//   int iVar3;
//   int iVar4;
//
//   iVar4 = x - Scratchpad._56_4_;
//   iVar1 = abs(iVar4);
//   if (30000 < iVar1) {
//     return 0;
//   }
//   iVar3 = y - Scratchpad._60_4_;
//   iVar1 = abs(iVar3);
//   if (iVar1 < 0x7531) {
//     iVar2 = z - Scratchpad._64_4_;
//     iVar1 = abs(iVar2);
//     if (iVar1 < 0x7531) {
//       Scratchpad._16_2_ = (undefined2)iVar4;
//       Scratchpad._18_2_ = (undefined2)iVar3;
//       Scratchpad._20_2_ = (undefined2)iVar2;
//       ApplyRotMatrix((SVECTOR *)(Scratchpad + 0x10),(VECTOR *)Scratchpad);
//       iVar1 = Scratchpad._8_4_ + s;
//       if (iVar1 < 0x97) {
//         return 0;
//       }
//       if (17000 < Scratchpad._8_4_ - s) {
//         return 0;
//       }
//       if (iVar1 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar1 == -1) && (Scratchpad._0_4_ * 300 == -0x80000000)) {
//         trap(0x1800);
//       }
//       iVar4 = (s * 300) / iVar1;
//       if (iVar1 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar1 == -1) && (s * 300 == -0x80000000)) {
//         trap(0x1800);
//       }
//       iVar3 = Scratchpad._4_4_ * 300;
//       if (iVar1 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar1 == -1) && (iVar3 == -0x80000000)) {
//         trap(0x1800);
//       }
//       iVar2 = abs((Scratchpad._0_4_ * 300) / iVar1);
//       if ((iVar2 <= iVar4 + 0xa0) && (iVar1 = abs(iVar3 / iVar1), iVar1 <= iVar4 + 0x78)) {
//         return 1;
//       }
//       return 0;
//     }
//   }
//   return 0;
// }
