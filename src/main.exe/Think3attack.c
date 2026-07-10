#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3attack(void);
 *     THINK_3.C:70, 29 src lines, frame 24 bytes, saved-reg mask 0x80010000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $v0       short rng
 *     reg   $s0       short pad
 *     reg   $a2       short idx
 *
 * Globals it touches, as the original declared them:
 *     extern short SR;
 *     extern long Distance;
 *     extern short Degree;
 *     extern short EngageLevel;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/Think3attack", Think3attack);

// triage: MEDIUM — 284 insns, mul/div, 5 callees, ~0.05 to bow_shoot_logic
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short Think3attack(void)
//
// {
//   short sVar1;
//   ushort uVar2;
//   uint uVar3;
//   int iVar4;
//   int iVar5;
//   long dist;
//   short cmd;
//   short sVar6;
//
//   uVar2 = 0;
//   iVar5 = (int)((uint)(ushort)Me_THINK_C->wpatk << 0x10) >> 0x14;
//   if (Me_THINK_C->status == 7) {
//     dist = 3000;
//     if (iVar5 == 3) {
//       dist = 20000;
//       sVar1 = 500;
//     }
//     else {
//       sVar1 = 0x5dc;
//     }
//     sVar1 = SuccessionAttack(dist,sVar1);
//     return sVar1;
//   }
//   if ((SR != -2) && (((iVar5 == 3 && (Distance < 14000)) || (Distance < 10000)))) {
//     SR = 0;
//   }
//   sVar1 = Me_THINK_C->wpatk >> 4;
//   iVar5 = (int)(short)((4 - sVar1) * Me_THINK_C->turn);
//   if (iVar5 < Degree) {
//     uVar2 = 0x2000;
//   }
//   else if ((int)Degree < -iVar5) {
//     uVar2 = 0x8000;
//   }
//   if (sVar1 == 3) {
//     sVar6 = 4000;
//   }
//   else {
//     sVar6 = (short)((uint)(((int)((uint)(ushort)atkd[sVar1] << 0x10) >> 0x10) -
//                           ((int)((uint)(ushort)atkd[sVar1] << 0x10) >> 0x1f)) >> 1);
//   }
//   if (Distance < sVar6) {
//     iVar5 = (int)Degree;
//     if (iVar5 < 0) {
//       iVar5 = -iVar5;
//     }
//     if ((999 < iVar5) || (Me_THINK_C->motion->count != 0)) {
//       uVar2 = uVar2 | 0x4000;
//       goto LAB_8002d488;
//     }
//     if (Distance < 2000) {
//       iVar5 = rand();
//       iVar4 = EngageLevel + 1;
//       if (iVar4 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar4 == -1) && (iVar5 == -0x80000000)) {
//         trap(0x1800);
//       }
//       uVar2 = uVar2 | 0x80;
//       if (iVar5 % iVar4 == 0) {
//         uVar2 = 0xa0;
//       }
//       goto LAB_8002d488;
//     }
// LAB_8002d3a4:
//     uVar2 = uVar2 | 0x80;
//     goto LAB_8002d488;
//   }
//   if (Distance < atkd[sVar1]) {
//     if (sVar1 == 3) {
//       if (uVar2 == 0) {
//         iVar5 = rand();
//         iVar4 = (int)EngageLevel << 2;
//         if (iVar4 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar4 == -1) && (iVar5 == -0x80000000)) {
//           trap(0x1800);
//         }
//         if (iVar5 % iVar4 == 0) {
//           uVar2 = 0x80;
//         }
//       }
//       goto LAB_8002d488;
//     }
//     iVar5 = (int)Degree;
//     if (iVar5 < 0) {
//       iVar5 = -iVar5;
//     }
//     if ((99 < iVar5) || (cmd = 0x21, Distance <= atkd[sVar1] + -1000)) {
//       if (Me_THINK_C->motion->count == 0) {
//         iVar5 = (int)Degree;
//         if (iVar5 < 0) {
//           iVar5 = -iVar5;
//         }
//         if (iVar5 < 0x4b0) goto LAB_8002d3a4;
//       }
//       if (sVar6 + 500 < Distance) {
//         uVar2 = uVar2 | 0x1000;
//       }
//       goto LAB_8002d488;
//     }
//   }
//   else {
//     if (Me_THINK_C->status != 5) goto LAB_8002d488;
//     if (StagePlayer->status != 0xe) {
//       if (Me_THINK_C->motion->count == 0) {
//         iVar5 = rand();
//         cmd = 1;
//         if (iVar5 == (iVar5 / 3) * 3) goto LAB_8002d470;
//       }
//       ItemUse();
//       goto LAB_8002d488;
//     }
//     uVar3 = rand();
//     cmd = 4;
//     if ((uVar3 & 1) != 0) {
//       cmd = 3;
//     }
//   }
// LAB_8002d470:
//   uVar2 = SetCommand(&Me_THINK_C->pad,cmd);
// LAB_8002d488:
//   if (((Me_THINK_C->motion->count == 0) && (iVar5 = rand(), iVar5 == (iVar5 / 0x1e) * 0x1e)) &&
//      (Me_THINK_C->status == 5)) {
//     SetNowMotion(Me_THINK_C,0x713,1);
//   }
//   return uVar2;
// }
