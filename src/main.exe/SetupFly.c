#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void SetupFly(struct param_fly *pfly, struct VECTOR *start, struct VECTOR *end, int yw, int yh, int time);
 *     ITEM.C:792, 39 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct param_fly * pfly
 *     param $a1       struct VECTOR * start
 *     param $a2       struct VECTOR * end
 *     param $s5       int yw
 *     param stack+16  int yh
 *     param stack+20  int time
 *     reg   $s6       int yh
 *     reg   $s0       int time
 *     reg   $a0       long len
 *     reg   $s1       struct tag_fly * fly
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetupFly", SetupFly);

// triage: MEDIUM — 166 insns, mul/div, 2 callees, ~0.10 to bow_shoot_logic
// likely-relevant cookbook sections:
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void SetupFly(int *pfly,VECTOR *start,VECTOR *end,int yw)
//
// {
//   int iVar1;
//   int iVar2;
//   int iVar3;
//   int iVar4;
//   int iVar5;
//   int iVar6;
//   int iVar7;
//   int iVar8;
//   int in_stack_00000010;
//   int in_stack_00000014;
//
//   *(undefined1 *)(pfly + 10) = 0;
//   *pfly = start->vx;
//   pfly[1] = start->vy;
//   pfly[2] = start->vz;
//   pfly[3] = end->vx;
//   pfly[4] = end->vy;
//   pfly[5] = end->vz;
//   iVar1 = GetVectorDistance(start,end);
//   if (0 < in_stack_00000014) {
//     if (in_stack_00000014 == 0) {
//       trap(0x1c00);
//     }
//     if ((in_stack_00000014 == -1) && (iVar1 == -0x80000000)) {
//       trap(0x1800);
//     }
//     *(char *)(pfly + 9) = (char)(iVar1 / in_stack_00000014);
//     if ((iVar1 / in_stack_00000014 & 0xffU) != 0) goto LAB_8003e28c;
//   }
//   *(undefined1 *)(pfly + 9) = 1;
// LAB_8003e28c:
//   iVar7 = iVar1 * (yw / 2);
//   *(char *)((int)pfly + 0x25) = (char)pfly[9];
//   if (iVar7 < 0) {
//     iVar7 = iVar7 + 0xfff;
//   }
//   iVar1 = iVar1 * (in_stack_00000010 / 2);
//   iVar7 = iVar7 >> 0xc;
//   if (iVar1 < 0) {
//     iVar1 = iVar1 + 0xfff;
//   }
//   iVar2 = *pfly;
//   iVar5 = pfly[3];
//   iVar8 = iVar7 << 1;
//   if (iVar8 < 1) {
//     iVar8 = -iVar7;
//   }
//   else {
//     iVar3 = rand();
//     if (iVar8 == 0) {
//       trap(0x1c00);
//     }
//     if ((iVar8 == -1) && (iVar3 == -0x80000000)) {
//       trap(0x1800);
//     }
//     iVar8 = iVar3 % iVar8 - iVar7;
//   }
//   iVar4 = pfly[1];
//   iVar6 = pfly[4];
//   iVar3 = (iVar1 >> 0xc) - (iVar1 >> 0x1f) >> 1;
//   iVar1 = (iVar1 >> 0xc) - iVar3;
//   pfly[6] = (iVar2 + iVar5) / 2 + iVar8;
//   if (0 < iVar1) {
//     iVar2 = rand();
//     if (iVar1 == 0) {
//       trap(0x1c00);
//     }
//     if ((iVar1 == -1) && (iVar2 == -0x80000000)) {
//       trap(0x1800);
//     }
//     iVar3 = iVar2 % iVar1 + iVar3;
//   }
//   iVar1 = pfly[2];
//   iVar2 = pfly[5];
//   iVar5 = iVar7 << 1;
//   pfly[7] = (iVar4 + iVar6) / 2 - iVar3;
//   if (iVar5 < 1) {
//     iVar7 = -iVar7;
//   }
//   else {
//     iVar8 = rand();
//     if (iVar5 == 0) {
//       trap(0x1c00);
//     }
//     if ((iVar5 == -1) && (iVar8 == -0x80000000)) {
//       trap(0x1800);
//     }
//     iVar7 = iVar8 % iVar5 - iVar7;
//   }
//   pfly[8] = (iVar1 + iVar2) / 2 + iVar7;
//   *(char *)(pfly + 9) = (char)pfly[9] + -1;
//   return;
// }
