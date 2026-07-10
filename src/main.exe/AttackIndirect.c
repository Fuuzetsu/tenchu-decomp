#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short AttackIndirect(void);
 *     THINK_3.C:525, 54 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern struct BattleType BattleDB[78];
 *     extern long Distance;
 *     extern short Degree;
 *     extern short EngageLevel;
 *     extern short SR;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/AttackIndirect", AttackIndirect);

// triage: MEDIUM — 212 insns, mul/div, 4 callees, ~0.07 to AttackAnimal
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short AttackIndirect(void)
//
// {
//   ushort uVar1;
//   int iVar2;
//   int iVar3;
//   short cmd;
//   uint uVar4;
//
//   uVar4 = 0;
//   if (Me_THINK_C->status != 7) {
//     iVar2 = 0;
//     if (Me_THINK_C->status == 9) goto LAB_8002f160;
//     if ((Distance < 20000) && (SR != -2)) {
//       SR = 0;
//     }
//     if (Distance < 5000) {
//       iVar2 = (int)Degree;
//       if (iVar2 < 0) {
//         iVar2 = -iVar2;
//       }
//       if (999 < iVar2) {
//         uVar4 = 0x1000;
//         goto LAB_8002f128;
//       }
//       uVar4 = FUN_8002b990(0,0);
//       uVar4 = uVar4 & 0xffffa000;
//       if (2000 < Distance - 1000U) {
//         uVar4 = uVar4 | 0x4000;
//       }
//       iVar3 = (int)Degree;
//       if (iVar3 < 0) {
//         iVar3 = -iVar3;
//       }
//       iVar2 = uVar4 << 0x10;
//       if ((iVar3 < 200) && (iVar2 = uVar4 << 0x10, Me_THINK_C->motion->mid == 0x501)) {
//         uVar4 = 0x80;
//         goto LAB_8002f128;
//       }
//     }
//     else {
//       iVar2 = rand();
//       iVar3 = (int)EngageLevel << 2;
//       if (iVar3 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar3 == -1) && (iVar2 == -0x80000000)) {
//         trap(0x1800);
//       }
//       if (iVar2 % iVar3 == 0) {
//         iVar2 = (int)Degree;
//         if (iVar2 < 0) {
//           iVar2 = -iVar2;
//         }
//         if ((iVar2 < 200) && (Me_THINK_C->motion->mid == 0x501)) {
//           uVar4 = 0x80;
//         }
//       }
//       if (Distance < 0x3a99) {
//         if (Degree < 0xc9) {
//           if (Degree < 0x65) {
//             if (Degree < -200) {
//               uVar4 = 0xffff8000;
//             }
//             else {
//               cmd = 3;
//               if (Degree < -100) goto LAB_8002f110;
//               ItemUse();
//             }
//           }
//           else {
//             cmd = 4;
// LAB_8002f110:
//             uVar1 = SetCommand(&Me_THINK_C->pad,cmd);
//             uVar4 = (uint)uVar1;
//           }
//         }
//         else {
//           uVar4 = 0x2000;
//         }
//       }
//       else {
//         uVar4 = FUN_8002b990(0,0);
//       }
// LAB_8002f128:
//       iVar2 = uVar4 << 0x10;
//     }
//     iVar2 = iVar2 >> 0x10;
//     if (iVar2 == 0x80) {
//       Me_THINK_C->rotate->vy = Me_THINK_C->rotate->vy + Degree;
//     }
//     goto LAB_8002f160;
//   }
//   uVar1 = 0;
//   if (Me_THINK_C->motion->count == BattleDB[Me_THINK_C->warid].contfrm) {
//     if (Distance < 20000) {
//       iVar2 = (int)Degree;
//       if (iVar2 < 0) {
//         iVar2 = -iVar2;
//       }
//       if (499 < iVar2) goto LAB_8002eea0;
//     }
//     else {
// LAB_8002eea0:
//       iVar2 = rand();
//       iVar3 = EngageLevel + 1;
//       if (iVar3 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar3 == -1) && (iVar2 == -0x80000000)) {
//         trap(0x1800);
//       }
//       uVar1 = 0;
//       if (iVar2 % iVar3 != 0) goto LAB_8002ef20;
//     }
//     if (Degree < 0x12d) {
//       uVar1 = 0x80;
//       if (-0x12d < Degree) goto LAB_8002ef20;
//       uVar1 = 0x8000;
//     }
//     else {
//       uVar1 = 0x2000;
//     }
//     uVar1 = uVar1 | 0x80;
//   }
// LAB_8002ef20:
//   iVar2 = (int)(short)uVar1;
// LAB_8002f160:
//   return (short)iVar2;
// }
