#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1chase(void);
 *     THINK_1.C:119, frame 40 bytes, saved-reg mask 0x800f0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $a0       struct Humanoid * enemy
 *     reg   $s1       long xx
 *     reg   $s2       long zz
 *     reg   $s3       short pad
 *     reg   $s1       long vx
 *     reg   $s2       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/Think1chase", Think1chase);

// triage: MEDIUM — 94 insns, mul/div, 3 callees, ~0.07 to ReqItemNingyo
// likely-relevant cookbook sections:
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short Think1chase(void)
//
// {
//   Humanoid *pHVar1;
//   short sVar2;
//   Humanoid *pHVar3;
//   int iVar4;
//   uchar uVar5;
//
//   uVar5 = Me_THINK_C->actcnt + '\x01';
//   Me_THINK_C->actcnt = uVar5;
//   sVar2 = 0;
//   if (uVar5 == '\x01') {
//     pHVar3 = GetNearestHumanoid(Me_THINK_C,5000);
//     pHVar1 = Me_THINK_C;
//     if (pHVar3 == (Humanoid *)0x0) {
//       iVar4 = rand();
//       Me_THINK_C->chase[0] = Me_THINK_C->point[0] + iVar4 % 10000 + -5000;
//       iVar4 = rand();
//       Me_THINK_C->chase[1] = Me_THINK_C->point[1] + iVar4 % 10000 + -5000;
//     }
//     else {
//       Me_THINK_C->chase[0] = pHVar3->locate->vx;
//       pHVar1->chase[1] = pHVar3->locate->vz;
//     }
//   }
//   else {
//     sVar2 = FUN_8002b990(Me_THINK_C->chase[0] - Me_THINK_C->locate->vx,
//                          Me_THINK_C->chase[1] - Me_THINK_C->locate->vz);
//     if (sVar2 == 0) {
//       Me_THINK_C->actcnt = '\0';
//       sVar2 = 0x80;
//     }
//   }
//   return sVar2;
// }
