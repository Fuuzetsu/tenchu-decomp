#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1target(void);
 *     THINK_1.C:154, 110 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     reg   $a0       long xx
 *     reg   $a1       long zz
 *     reg   $a0       long vx
 *     reg   $a1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern short SR;
 *     extern struct Humanoid *StagePlayer;
 *     extern short Attrib;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/Think1target", Think1target);

// triage: MEDIUM — 214 insns, mul/div, 6 callees, ~0.20 to Think1watch
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short Think1target(void)
//
// {
//   byte bVar1;
//   Humanoid *pHVar2;
//   short sVar3;
//   int iVar4;
//   long lVar5;
//   VECTOR *pVVar6;
//   VECTOR *pVVar7;
//   int iVar8;
//   int iVar9;
//
//   if (Me_THINK_C->target == (ModelType *)0x0) {
//     sVar3 = 0;
//     if ((Me_THINK_C->actcnt & 0x7f) == 0) {
//       sVar3 = -0x8000;
//       if (Me_THINK_C->actflg != '\0') {
//         sVar3 = 0x2000;
//       }
//       bVar1 = Me_THINK_C->actscnt;
//       Me_THINK_C->actscnt = bVar1 + 1;
//       if (10 < bVar1) {
//         iVar4 = rand();
//         Me_THINK_C->actflg = (byte)iVar4 & 1;
//         Me_THINK_C->actscnt = '\0';
//         Me_THINK_C->actcnt = Me_THINK_C->actcnt + '\x01';
//       }
//     }
//     else {
//       Me_THINK_C->actcnt = Me_THINK_C->actcnt + 1;
//     }
//   }
//   else {
//     SR = -1;
//     if ((GameClock & 0x1fU) == 0) {
//       pVVar7 = Me_THINK_C->locate;
//       pVVar6 = StagePlayer->locate;
//       iVar9 = pVVar6->vx - pVVar7->vx;
//       iVar8 = pVVar6->vz - pVVar7->vz;
//       iVar4 = pVVar6->vy - pVVar7->vy;
//       lVar5 = SquareRoot0(iVar9 * iVar9 + iVar8 * iVar8);
//       sVar3 = GetDirection(iVar9,iVar8,Me_THINK_C->rotate->vy);
//       pHVar2 = Me_THINK_C;
//       iVar8 = (int)sVar3;
//       if (lVar5 < 0xfa1) {
//         if (iVar4 < 0) {
//           iVar4 = -iVar4;
//         }
//         if (iVar4 < 0xbb9) {
//           if (iVar8 < 0) {
//             iVar8 = -iVar8;
//           }
//           if ((iVar8 < 900) && (StagePlayer->itmctl != 0xb)) {
//             Attrib = Attrib & 0xfffcU | 2;
//             Me_THINK_C->target = (ModelType *)StagePlayer->model;
//             SetNowMotion(pHVar2,0x80e,1);
//             pHVar2 = Me_THINK_C;
//             Me_THINK_C->chase[1] = 0;
//             pHVar2->chase[0] = 0;
//             Sound(pHVar2,0xd);
//             DAT_800979c0 = 300;
//             if (PersistentState._88_1_ == '\x02') {
//               DAT_800979c0 = 600;
//             }
//           }
//         }
//       }
//     }
//     iVar8 = (Me_THINK_C->target->locate).coord.t[0] - Me_THINK_C->locate->vx;
//     iVar4 = (Me_THINK_C->target->locate).coord.t[2] - Me_THINK_C->locate->vz;
//     lVar5 = SquareRoot0(iVar8 * iVar8 + iVar4 * iVar4);
//     sVar3 = 0;
//     if (199 < lVar5) {
//       if (lVar5 < 4000) {
//         iVar9 = (Me_THINK_C->target->locate).coord.t[1] - Me_THINK_C->locate->vy;
//         if (iVar9 < 0) {
//           iVar9 = -iVar9;
//         }
//         if (2000 < iVar9) {
//           if ((Me_THINK_C->actcnt & 0x7f) != 0) {
//             Me_THINK_C->actcnt = Me_THINK_C->actcnt + 1;
//             return 0;
//           }
//           sVar3 = -0x8000;
//           if (Me_THINK_C->actflg != '\0') {
//             sVar3 = 0x2000;
//           }
//           bVar1 = Me_THINK_C->actscnt;
//           Me_THINK_C->actscnt = bVar1 + 1;
//           if (bVar1 < 0xb) {
//             return sVar3;
//           }
//           iVar4 = rand();
//           Me_THINK_C->actflg = (byte)iVar4 & 1;
//           Me_THINK_C->actscnt = '\0';
//           Me_THINK_C->actcnt = Me_THINK_C->actcnt + '\x01';
//           return sVar3;
//         }
//       }
//       sVar3 = FUN_8002b990(iVar8,iVar4);
//     }
//   }
//   return sVar3;
// }
