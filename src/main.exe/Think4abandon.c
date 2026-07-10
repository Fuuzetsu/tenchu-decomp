#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think4abandon(void);
 *     THINK_4.C:14, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     reg   $v1       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Attrib;
 *     extern short SR;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/Think4abandon", Think4abandon);

// triage: MEDIUM — 117 insns, mul/div, 4 callees, ~0.04 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short Think4abandon(void)
//
// {
//   Humanoid *pHVar1;
//   ushort uVar2;
//   int iVar3;
//   ushort uVar4;
//
//   pHVar1 = Me_THINK_C;
//   uVar4 = Attrib & 0xffec;
//   uVar2 = Me_THINK_C->type;
//   Me_THINK_C->chase[1] = 0;
//   pHVar1->chase[0] = 0;
//   if ((uVar2 & 0xf0) == 0x80) {
//     if ((((ushort)SR - 1 < 2) && (Attrib = uVar4 | 2, pHVar1->motion->count == 0)) &&
//        (iVar3 = rand(), iVar3 == (iVar3 / 0x3c) * 0x3c)) {
//       Sound(Me_THINK_C,0xd);
//     }
//   }
//   else {
//     if (DAT_800979c0 == 0) {
//       if (pHVar1->think[3] == (undefined **)Think4abandon) {
//         uVar2 = FUN_8002b990(0,0);
//         if ((uVar2 & 0xa000) != 0) {
//           return uVar2 & 0xa000;
//         }
//       }
//       if (SR == 1) {
//         Attrib = uVar4 | 2;
//         return 0;
//       }
//       if (1 < SR) {
//         if (SR != 2) {
//           return 0;
//         }
//         Attrib = uVar4 | 1;
//         SetNowMotion(Me_THINK_C,0x80f,1);
//         return 0;
//       }
//       if (-1 < SR) {
//         return 0;
//       }
//       if (SR < -2) {
//         return 0;
//       }
//       Attrib = uVar4;
//       SetNowMotion(Me_THINK_C,0x80f,1);
//       Sound(Me_THINK_C,0xe);
//       return 0;
//     }
//     if (SR == 1) {
//       Attrib = uVar4 | 2;
//     }
//   }
//   uVar2 = FUN_8002b990(0,0);
//   return uVar2 & 0xa000;
// }
