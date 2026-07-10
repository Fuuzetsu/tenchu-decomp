#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void * vmemoryGC(void *pt);
 *     VALLOC.C:259, 40 src lines, frame 56 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s2       void * pt
 *     reg   $s3       struct VMhead * vhp
 *     reg   $a3       struct VMhead * svhp
 *     stack sp+16     struct VMhead vh
 *     reg   $s1       unsigned long * vmpt
 *     reg   $s6       unsigned long size
 *     reg   $s2       void * pt
 *     reg   $s2       void * pt
 *     reg   $a0       struct VMhead * svhp
 *     reg   $s0       struct VMhead * vhp
 *     reg   $s1       void * pt
 *     reg   $a0       struct VMhead * svhp
 *     reg   $s0       struct VMhead * vhp
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *virtual_memory_pool;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/vmemoryGC", vmemoryGC);

// triage: MEDIUM — 195 insns, 3 loop, 3 callees, ~0.06 to ReqItemMakibishi
// likely-relevant cookbook sections:
//   - Loops: 3 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// undefined * vmemoryGC(undefined *pt)
//
// {
//   uchar *puVar1;
//   int *piVar2;
//   int *piVar3;
//   int *piVar4;
//   uint *puVar5;
//   uint uVar6;
//   int *piVar7;
//   ulong size;
//   int local_28;
//   uint *local_24;
//
//   piVar7 = (int *)(pt + -8);
//   size = *(int *)(pt + -8) << 2;
//   puVar1 = valloc(size);
//   if (puVar1 != (uchar *)0x0) {
//     if (puVar1 < pt) {
//       memcpy(puVar1,pt,size);
//       if (pt == (undefined *)0x0) {
//         return puVar1;
//       }
//       if (-1 < (int)*(uint *)(pt + -8)) {
//                     /* WARNING: Subroutine does not return */
//         SystemOut((uchar *)"DOUBLE MEMORY RELEASE");
//       }
//       uVar6 = *(uint *)(pt + -8) & 0x7fffffff;
//       *(uint *)(pt + -8) = uVar6;
//       puVar5 = *(uint **)(pt + -4);
//       if (puVar5 != (uint *)0x0) {
//         if ((~*puVar5 & 0x80000000) != 0) {
//           *(uint *)(pt + -8) = uVar6 + 2 + *puVar5;
//           *(uint *)(pt + -4) = puVar5[1];
//         }
//       }
//       piVar4 = virtual_memory_pool;
//       if (virtual_memory_pool == (int *)0x0) {
//         return puVar1;
//       }
//       do {
//         piVar2 = (int *)piVar4[1];
//         if (piVar2 == piVar7) break;
//         piVar4 = piVar2;
//       } while (piVar2 != (int *)0x0);
//       if (piVar4 == (int *)0x0) {
//         return puVar1;
//       }
//       if (*piVar4 < 0) {
//         return puVar1;
//       }
//       *piVar4 = *piVar4 + 2 + *piVar7;
//       piVar4[1] = *(int *)(pt + -4);
//       return puVar1;
//     }
//     if (puVar1 != (uchar *)0x0) {
//       if (-1 < (int)*(uint *)(puVar1 + -8)) {
//                     /* WARNING: Subroutine does not return */
//         SystemOut((uchar *)"DOUBLE MEMORY RELEASE");
//       }
//       uVar6 = *(uint *)(puVar1 + -8) & 0x7fffffff;
//       *(uint *)(puVar1 + -8) = uVar6;
//       puVar5 = *(uint **)(puVar1 + -4);
//       if (puVar5 != (uint *)0x0) {
//         if ((~*puVar5 & 0x80000000) != 0) {
//           *(uint *)(puVar1 + -8) = uVar6 + 2 + *puVar5;
//           *(uint *)(puVar1 + -4) = puVar5[1];
//         }
//       }
//       piVar4 = virtual_memory_pool;
//       if (virtual_memory_pool != (int *)0x0) {
//         do {
//           piVar2 = (int *)piVar4[1];
//           if (piVar2 == (int *)(puVar1 + -8)) break;
//           piVar4 = piVar2;
//         } while (piVar2 != (int *)0x0);
//         if ((piVar4 != (int *)0x0) && (-1 < *piVar4)) {
//           *piVar4 = *piVar4 + 2 + *(int *)(puVar1 + -8);
//           piVar4[1] = *(int *)(puVar1 + -4);
//         }
//       }
//     }
//   }
//   piVar4 = (int *)pt;
//   piVar2 = virtual_memory_pool;
//   if (virtual_memory_pool != (int *)0x0) {
//     do {
//       piVar3 = (int *)piVar2[1];
//       if (piVar3 == piVar7) break;
//       piVar2 = piVar3;
//     } while (piVar3 != (int *)0x0);
//     if ((piVar2 != (int *)0x0) && (local_28 = *piVar2, -1 < local_28)) {
//       piVar4 = piVar2 + 2;
//       local_24 = *(uint **)(pt + -4);
//       piVar3 = piVar2 + *piVar7 + 2;
//       *piVar2 = *piVar7;
//       piVar2[1] = (int)piVar3;
//       memcpy((uchar *)piVar4,pt,size);
//       if ((local_24 != (uint *)0x0) && ((~*local_24 & 0x80000000) != 0)) {
//         local_28 = local_28 + 2 + *local_24;
//         local_24 = (uint *)local_24[1];
//       }
//       *piVar3 = local_28;
//       piVar3[1] = (int)local_24;
//     }
//   }
//   return (undefined *)piVar4;
// }
