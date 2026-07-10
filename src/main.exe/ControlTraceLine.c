#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ControlTraceLine(struct Humanoid *human);
 *     HUMAN.C:526, 33 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s5       struct Humanoid * human
 *     reg   $s4       struct TracePoint * point
 *     reg   $s1       struct TraceLine * trcl
 *     reg   $s2       long dx
 *     reg   $s0       long dz
 *     reg   $s6       long dist
 *     reg   $s3       short pad
 *     reg   $s2       long dx
 *     reg   $s0       long dz
 *     reg   $s0       short roty
 *     reg   $a1       short degree
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ControlTraceLine", ControlTraceLine);

// triage: MEDIUM — 126 insns, mul/div, 2 callees, ~0.11 to GetDirection
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short ControlTraceLine(Humanoid *human)
//
// {
//   ushort uVar1;
//   ushort uVar2;
//   long lVar3;
//   long lVar4;
//   short sVar5;
//   int iVar6;
//   int iVar7;
//   TraceLine *pTVar8;
//   TracePoint *pTVar9;
//
//   pTVar8 = human->trace;
//   uVar2 = 0x1000;
//   if (pTVar8 == (TraceLine *)0x0) {
//     return 0;
//   }
//   pTVar9 = pTVar8->point + pTVar8->index;
//   iVar7 = pTVar9->x - human->locate->vx;
//   iVar6 = pTVar9->z - human->locate->vz;
//   lVar3 = SquareRoot0(iVar7 * iVar7 + iVar6 * iVar6);
//   if ((human->attribute & 0x400U) != 0) {
//     pTVar8->count = -0x1e;
//   }
//   uVar1 = pTVar8->count;
//   pTVar8->count = uVar1 + 1;
//   if ((int)((uint)uVar1 << 0x10) < 1) goto LAB_8002911c;
//   uVar1 = human->rotate->vy;
//   lVar4 = ratan2(-iVar7,-iVar6);
//   iVar6 = lVar4 - (uint)uVar1;
//   sVar5 = (short)iVar6;
//   iVar6 = iVar6 * 0x10000 >> 0x10;
//   if (iVar6 < 0x801) {
//     if (iVar6 < -0x7ff) goto LAB_800290c0;
//   }
//   else {
//     sVar5 = -sVar5;
// LAB_800290c0:
//     sVar5 = sVar5 + 0x1000;
//   }
//   if ((int)sVar5 < (int)human->turn) {
//     if ((int)sVar5 <= -(int)human->turn) {
//       uVar2 = 0x9000;
//     }
//   }
//   else {
//     uVar2 = 0x3000;
//   }
//   iVar6 = (int)sVar5;
//   if (iVar6 < 0) {
//     iVar6 = -iVar6;
//   }
//   if (500 < iVar6) {
//     uVar2 = uVar2 & 0xa000;
//   }
// LAB_8002911c:
//   if (lVar3 <= pTVar9->range) {
//     sVar5 = pTVar8->index + 1;
//     pTVar8->index = sVar5;
//     uVar2 = uVar2 | pTVar8->point[sVar5].pad;
//     if (pTVar8->point[sVar5].pad == -1) {
//       pTVar8->index = 0;
//       uVar2 = 0xf000;
//     }
//   }
//   return uVar2;
// }
