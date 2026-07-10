#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long ComputeAreaLevel(struct AreaNodeType *node, long x, long z);
 *     CONFLICT.C:83, 20 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $t0       struct AreaNodeType * node
 *     param $a1       long x
 *     param $a2       long z
 *     reg   $a0       short yy
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ComputeAreaLevel", ComputeAreaLevel);

// triage: MEDIUM — 105 insns, mul/div, 0 callees, ~0.06 to bow_shoot_logic
// likely-relevant cookbook sections:
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// long ComputeAreaLevel(AreaNodeType *node,long x,long z)
//
// {
//   int iVar1;
//   uint uVar2;
//   short sVar3;
//   int iVar4;
//   int iVar5;
//   int iVar6;
//   int iVar7;
//   int iVar8;
//   short sVar9;
//
//   iVar6 = (int)((z - (uint)(ushort)node->z1) * 0x10000) >> 0x10;
//   iVar4 = iVar6 << 2;
//   iVar8 = (int)((((uint)(ushort)node->z2 - (uint)(ushort)node->z1) + 1) * 0x10000) >> 0x10;
//   if (iVar8 == 0) {
//     trap(0x1c00);
//   }
//   if ((iVar8 == -1) && (iVar4 == -0x80000000)) {
//     trap(0x1800);
//   }
//   iVar5 = (int)((x - (uint)(ushort)node->x1) * 0x10000) >> 0x10;
//   iVar1 = iVar5 << 2;
//   iVar7 = (int)((((uint)(ushort)node->x2 - (uint)(ushort)node->x1) + 1) * 0x10000) >> 0x10;
//   if (iVar7 == 0) {
//     trap(0x1c00);
//   }
//   if ((iVar7 == -1) && (iVar1 == -0x80000000)) {
//     trap(0x1800);
//   }
//   if (((uint)(ushort)node->division & 1 << ((iVar4 / iVar8) * 4 + iVar1 / iVar7 & 0x1fU)) == 0) {
//     return -0x80000000;
//   }
//   sVar3 = node->y;
//   uVar2 = (int)node->attribute & 0xc000;
//   if (uVar2 == 0x4000) {
//     iVar5 = iVar5 * node->dy;
//     sVar9 = (short)(iVar5 / iVar7);
//     if (iVar7 == 0) {
//       trap(0x1c00);
//     }
//     if ((iVar7 == -1) && (iVar5 == -0x80000000)) {
//       trap(0x1800);
//     }
//   }
//   else {
//     if (uVar2 != 0x8000) goto LAB_80019a00;
//     iVar6 = iVar6 * node->dy;
//     sVar9 = (short)(iVar6 / iVar8);
//     if (iVar8 == 0) {
//       trap(0x1c00);
//     }
//     if ((iVar8 == -1) && (iVar6 == -0x80000000)) {
//       trap(0x1800);
//     }
//   }
//   sVar3 = sVar3 + sVar9;
// LAB_80019a00:
//   return (int)sVar3;
// }
