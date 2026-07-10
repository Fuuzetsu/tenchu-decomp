#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think4chase(void);
 *     THINK_4.C:75, 189 src lines, frame 32 bytes, saved-reg mask 0x80070000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $a0       short pad
 *     reg   $s1       long xx
 *     reg   $s2       long zz
 *     reg   $s1       long vx
 *     reg   $s2       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short SR;
 *     extern short Attrib;
 *     extern short Degree;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/Think4chase", Think4chase);

// triage: MEDIUM — 101 insns, mul/div, 3 callees, ~0.06 to GetVectorRotation
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short Think4chase(void)
//
// {
//   Humanoid *pHVar1;
//   short sVar2;
//   long lVar3;
//   int iVar4;
//   int iVar5;
//
//   if (SR == 1) {
//     Attrib = Attrib & 0xfffcU | 2;
//     sVar2 = 0;
//   }
//   else if ((Me_THINK_C->chase[0] == 0) && (Me_THINK_C->chase[1] == 0)) {
//     if (Me_THINK_C->actcnt < 0x5b) {
//       Me_THINK_C->actcnt = Me_THINK_C->actcnt + 1;
//       sVar2 = 0x1000;
//       if (Me_THINK_C->actcnt < 0x1e) {
//         if ((int)Me_THINK_C->turn < (int)Degree) {
//           sVar2 = 0x3000;
//         }
//         else {
//           sVar2 = 0x1000;
//           if ((int)Degree < -(int)Me_THINK_C->turn) {
//             sVar2 = -0x7000;
//           }
//         }
//       }
//     }
//     else {
//       sVar2 = Think4abandon();
//     }
//   }
//   else {
//     Me_THINK_C->actscnt = Me_THINK_C->actscnt + '\x01';
//     iVar5 = Me_THINK_C->chase[0] - Me_THINK_C->locate->vx;
//     iVar4 = Me_THINK_C->chase[1] - Me_THINK_C->locate->vz;
//     sVar2 = FUN_8002b990(iVar5,iVar4);
//     lVar3 = SquareRoot0(iVar5 * iVar5 + iVar4 * iVar4);
//     pHVar1 = Me_THINK_C;
//     if ((lVar3 < 1000) || (Me_THINK_C->actscnt == '\0')) {
//       Me_THINK_C->chase[1] = 0;
//       pHVar1->chase[0] = 0;
//       pHVar1->actcnt = '\0';
//     }
//   }
//   return sVar2;
// }
