#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ChasetoTarget(long length);
 *     THINK.C:269, 21 src lines, frame 48 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s5       long length
 *     reg   $s3       long xx
 *     reg   $s2       long zz
 *     reg   $s4       long * chase
 *     reg   $s3       long vx
 *     reg   $s2       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Attrib;
 *     extern long Distance;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ChasetoTarget", ChasetoTarget);

// triage: MEDIUM — 82 insns, mul/div, 4 callees, ~0.07 to SetupMotionRegist
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short ChasetoTarget(long length)
//
// {
//   Humanoid *pHVar1;
//   short sVar2;
//   int iVar3;
//   int iVar4;
//   ModelType *pMVar5;
//   int iVar6;
//   int iVar7;
//
//   pHVar1 = Me_THINK_C;
//   pMVar5 = Me_THINK_C->target;
//   sVar2 = 0;
//   if (pMVar5 != (ModelType *)0x0) {
//     iVar7 = ((pMVar5->locate).coord.t[0] + Me_THINK_C->chase[0]) - Me_THINK_C->locate->vx;
//     iVar6 = ((pMVar5->locate).coord.t[2] + Me_THINK_C->chase[1]) - Me_THINK_C->locate->vz;
//     iVar3 = iVar7;
//     if (iVar7 < 0) {
//       iVar3 = -iVar7;
//     }
//     if (iVar3 < 500) {
//       iVar3 = iVar6;
//       if (iVar6 < 0) {
//         iVar3 = -iVar6;
//       }
//       if (iVar3 < 500) {
//         return 0;
//       }
//     }
//     sVar2 = 0;
//     if (((Attrib & 0x400U) == 0) && (sVar2 = 0, 999 < Distance)) {
//       if (((Attrib & 0xc000U) != 0) || (Me_THINK_C->chase[0] == 0 && Me_THINK_C->chase[1] == 0)) {
//         iVar3 = rand();
//         iVar4 = rcos((int)(short)iVar3);
//         pHVar1->chase[0] = iVar4 * length >> 0xc;
//         iVar3 = rsin((int)(short)iVar3);
//         pHVar1->chase[1] = iVar3 * length >> 0xc;
//       }
//       sVar2 = FUN_8002b990(iVar7,iVar6);
//     }
//   }
//   return sVar2;
// }
