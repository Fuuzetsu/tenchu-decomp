#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void * valloc(unsigned long size);
 *     VALLOC.C:76, 50 src lines, frame 1080 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $t0       unsigned long size
 *     stack sp+24     struct VMhead vh
 *     reg   $a2       struct VMhead * vhp
 *     reg   $s1       unsigned long * vmpt
 *     stack sp+32     struct VMhead vh
 *     stack sp+40     unsigned char [1024] str
 *     reg   $a3       unsigned long maxsize
 *     reg   $a0       struct VMhead * vhp
 *     reg   $a1       unsigned long freesize
 *     reg   $v1       struct VMhead * vhp
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *virtual_memory_pool;
 * END PSX.SYM */

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/valloc", valloc);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/valloc", allocate_memory_in_mem_pool___override__prt_80016710_891e8130);

// triage: MEDIUM — 114 insns, 3 loop, frame 0x438, 2 callees, ~0.14 to vgetmaxsize
// likely-relevant cookbook sections:
//   - Loops: 3 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Stack objects: 0x438 frame — buffer casts / spills

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// undefined * valloc(ulong size)
//
// {
//   uint uVar1;
//   uint uVar2;
//   uint *puVar3;
//   int iVar4;
//   uint uVar5;
//   uint *puVar6;
//   uchar auStack_410 [1024];
//
//   if ((uint *)virtual_memory_pool == (uint *)0x0) {
//     virtual_memory_pool = MemoryPool;
//     MemoryPool[0] = 0xfe;
//     MemoryPool[1] = 0x7f;
//     MemoryPool[2] = 4;
//     MemoryPool[3] = 0;
//     MemoryPool[4] = 0;
//     MemoryPool[5] = 0;
//     MemoryPool[6] = 0;
//     MemoryPool[7] = 0;
//   }
//   if ((size & 3) != 0) {
//     size = size + 4;
//   }
//   uVar5 = size >> 2;
//   if ((uint *)virtual_memory_pool != (uint *)0x0) {
//     puVar3 = (uint *)virtual_memory_pool;
//     do {
//       uVar1 = *puVar3;
//       if (((uVar1 & 0x80000000) == 0) && (uVar5 <= uVar1)) {
//         if (uVar1 - uVar5 < 0x13) {
//           *puVar3 = uVar1 | 0x80000000;
//           puVar6 = puVar3 + 2;
//         }
//         else {
//           uVar1 = *puVar3;
//           uVar2 = puVar3[1];
//           puVar6 = puVar3 + uVar5 + 2;
//           *puVar3 = uVar5 | 0x80000000;
//           puVar3[1] = (uint)puVar6;
//           *puVar6 = (uVar1 - uVar5) - 2;
//           puVar6[1] = uVar2;
//           puVar6 = puVar3 + 2;
//         }
//         break;
//       }
//       puVar3 = (uint *)puVar3[1];
//       puVar6 = (uint *)0x0;
//     } while (puVar3 != (uint *)0x0);
//     if (puVar6 != (uint *)0x0) {
//       return (undefined *)puVar6;
//     }
//   }
//   uVar1 = 0;
//   for (puVar3 = (uint *)virtual_memory_pool; puVar3 != (uint *)0x0; puVar3 = (uint *)puVar3[1]) {
//     uVar2 = *puVar3;
//     if (((uVar2 & 0x80000000) == 0) && (uVar1 < uVar2)) {
//       uVar1 = uVar2;
//     }
//   }
//   iVar4 = 0;
//   for (puVar3 = (uint *)virtual_memory_pool; puVar3 != (uint *)0x0; puVar3 = (uint *)puVar3[1]) {
//     if ((*puVar3 & 0x80000000) == 0) {
//       iVar4 = iVar4 + *puVar3;
//     }
//   }
//   sprintf((char *)auStack_410,"OUT OF MEMORY\nREQUEST=%d\nFREE=%d(%d)\n",uVar5 << 2,uVar1 << 2,
//           iVar4 << 2);
//                     /* WARNING: Subroutine does not return */
//   SystemOut(auStack_410);
// }
