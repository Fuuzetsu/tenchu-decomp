#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void * vrealloc(void *pt, unsigned long size);
 *     VALLOC.C:130, 69 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s2       void * pt
 *     param $s0       unsigned long size
 *     reg   $a3       struct VMhead * vhp
 *     reg   $a1       struct VMhead * svhp
 *     stack sp+16     struct VMhead vh
 *     reg   $s1       void * newp
 *     reg   $a2       unsigned long size2
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/vrealloc", vrealloc);

// triage: EASY — 129 insns, 4 callees, ~0.08 to AddMisc
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// undefined * vrealloc(undefined *pt,ulong size)
//
// {
//   uchar *puVar1;
//   int *piVar2;
//   int iVar3;
//   ulong uVar4;
//   uint uVar5;
//   uint uVar6;
//   uint *puVar7;
//   uint uVar8;
//   int local_18;
//   uint local_14;
//
//   if (pt == (undefined *)0x0) {
//     puVar1 = valloc(size);
//   }
//   else {
//     if ((size & 3) != 0) {
//       size = size + 4;
//     }
//     uVar8 = size >> 2;
//     uVar5 = *(uint *)(pt + -8);
//     puVar7 = *(uint **)(pt + -4);
//     puVar1 = pt;
//     if (uVar5 < uVar8) {
//       if (((puVar7 == (uint *)0x0) || ((int)*puVar7 < 0)) ||
//          (uVar5 = uVar5 & 0x7fffffff, uVar5 + *puVar7 + 2 < uVar8)) {
//         puVar1 = valloc(uVar8 << 2);
//         if (pt != (undefined *)0x0) {
//           uVar4 = vsize(pt);
//           if (uVar8 < uVar4) {
//             uVar4 = uVar8;
//           }
//           memcpy(puVar1,pt,uVar4);
//         }
//         vfree(pt);
//       }
//       else {
//         *(uint *)(pt + -8) = uVar5;
//         uVar6 = (uVar5 + *puVar7) - uVar8;
//         if (uVar6 < 0x11) {
//           *(uint *)(pt + -8) = uVar5 + *puVar7 + 2 | 0x80000000;
//           *(uint *)(pt + -4) = puVar7[1];
//         }
//         else {
//           uVar5 = puVar7[1];
//           *(uint *)(pt + -8) = uVar8 | 0x80000000;
//           iVar3 = uVar8 * 4;
//           *(undefined **)(pt + -4) = pt + iVar3;
//           *(uint *)(pt + iVar3) = uVar6;
//           *(uint *)(pt + iVar3 + 4) = uVar5;
//         }
//       }
//     }
//     else {
//       *(uint *)(pt + -8) = uVar5 & 0x7fffffff;
//       uVar5 = (uVar5 & 0x7fffffff) - uVar8;
//       if (0x12 < uVar5) {
//         local_18 = uVar5 - 2;
//         local_14 = *(uint *)(pt + -4);
//         *(uint *)(pt + -8) = uVar8 | 0x80000000;
//         *(undefined **)(pt + -4) = pt + uVar8 * 4;
//         if ((puVar7 != (uint *)0x0) && ((~*puVar7 & 0x80000000) != 0)) {
//           local_18 = uVar5 + *puVar7;
//           local_14 = puVar7[1];
//         }
//         piVar2 = *(int **)(pt + -4);
//         *piVar2 = local_18;
//         piVar2[1] = local_14;
//       }
//     }
//   }
//   return puVar1;
// }
