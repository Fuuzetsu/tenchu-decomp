#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1random(void);
 *     THINK_1.C:74, frame 32 bytes, saved-reg mask 0x80030000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $a1       long xx
 *     reg   $v1       long zz
 *     reg   $s1       short pad
 *     reg   $a1       long vx
 *     reg   $v1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Attrib;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/Think1random", Think1random);

// triage: MEDIUM — 93 insns, mul/div, 2 callees, ~0.05 to ReqItemNingyo
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// short Think1random(void)
//
// {
//   short sVar1;
//   int iVar2;
//   uchar uVar3;
//   int iVar4;
//
//   uVar3 = Me_THINK_C->actcnt + '\x01';
//   Me_THINK_C->actcnt = uVar3;
//   if (uVar3 == '\x01') {
//     iVar2 = rand();
//     Me_THINK_C->chase[0] = Me_THINK_C->point[0] + iVar2 % 10000 + -5000;
//     iVar2 = rand();
//     Me_THINK_C->chase[1] = Me_THINK_C->point[1] + iVar2 % 10000 + -5000;
//     return 0;
//   }
//   iVar2 = Me_THINK_C->chase[0] - Me_THINK_C->locate->vx;
//   iVar4 = Me_THINK_C->chase[1] - Me_THINK_C->locate->vz;
//   if (iVar2 < 0) {
//     iVar2 = -iVar2;
//   }
//   if (iVar2 < 1000) {
//     if (iVar4 < 0) {
//       iVar4 = -iVar4;
//     }
//     if (iVar4 < 1000) goto LAB_8002c204;
//   }
//   if ((Attrib & 0x400U) == 0) {
//     sVar1 = FUN_8002b990();
//     return sVar1;
//   }
// LAB_8002c204:
//   Me_THINK_C->actcnt = '\0';
//   return 0;
// }
