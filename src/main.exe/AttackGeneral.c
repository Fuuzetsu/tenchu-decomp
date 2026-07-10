#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short AttackGeneral(void);
 *     THINK_3.C:368, 78 src lines, frame 24 bytes, saved-reg mask 0x80010000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $s0       short pad
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern struct BattleType BattleDB[78];
 *     extern long Distance;
 *     extern short Degree;
 *     extern short EngageLevel;
 *     extern short Attrib;
 *     extern long GameClock;
 *     extern long AttackActionCount;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/AttackGeneral", AttackGeneral);

// triage: HARD — 361 insns, mul/div, 4 callees, ~0.05 to AttackAnimal
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short AttackGeneral(void)
//
// {
//   Humanoid *pHVar1;
//   short sVar2;
//   int iVar3;
//   uint uVar4;
//   int iVar5;
//   ushort uVar6;
//
//   uVar6 = 0;
//   if (Me_THINK_C->status != 7) {
//     if (Me_THINK_C->status == 9) {
//       return 0;
//     }
//     if (Me_THINK_C->motion->mid == 0x500) {
//       iVar3 = rand();
//       iVar5 = EngageLevel + 1;
//       if (iVar5 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar5 == -1) && (iVar3 == -0x80000000)) {
//         trap(0x1800);
//       }
//       return (ushort)(iVar3 % iVar5 != 0) << 0xe;
//     }
//     if (Me_THINK_C->actmode == '\0') {
//       sVar2 = ChasetoTarget(3000);
//       if ((sVar2 == 0) || ((Attrib & 0x4000U) != 0)) {
//         Me_THINK_C->actmode = '\x01';
//       }
//       if (Distance < 0x1389) {
//         return sVar2;
//       }
//       iVar3 = (int)Degree;
//       if (iVar3 < 0) {
//         iVar3 = -iVar3;
//       }
//       if (99 < iVar3) {
//         return sVar2;
//       }
//       iVar3 = rand();
//       if (iVar3 != (iVar3 / 5) * 5) {
//         return sVar2;
//       }
//       return 0x1040;
//     }
//     if ((Me_THINK_C->motion->count & 0xfU) != 0) {
//       uVar6 = (Me_THINK_C->pad).data;
//       if (1999 < Distance) {
//         return uVar6;
//       }
//       iVar3 = (int)Degree;
//       if (iVar3 < 0) {
//         iVar3 = -iVar3;
//       }
//       if (iVar3 < 1000) {
//         return 0x4000;
//       }
//       if (iVar3 < 0x5dd) {
//         return uVar6;
//       }
//       return 0x1000;
//     }
//     if (5000 < Distance) {
//       Me_THINK_C->actmode = '\0';
//       pHVar1 = Me_THINK_C;
//       Me_THINK_C->chase[1] = 0;
//       pHVar1->chase[0] = 0;
//       ItemUse();
//       return 0;
//     }
//     if ((Attrib & 0x400U) != 0) {
//       Me_THINK_C->actmode = '\0';
//     }
//     if (Degree < 0x1f5) {
//       if (Degree < -500) {
//         uVar6 = 0x8000;
//       }
//     }
//     else {
//       uVar6 = 0x2000;
//     }
//     if (Distance - 0x3e9U < 1999) {
//       iVar3 = (int)Degree;
//       if (iVar3 < 0) {
//         iVar3 = -iVar3;
//       }
//       if (iVar3 < 0x4b0) {
//         iVar3 = rand();
//         iVar5 = EngageLevel + 1;
//         if (iVar5 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar5 == -1) && (iVar3 == -0x80000000)) {
//           trap(0x1800);
//         }
//         if ((iVar3 % iVar5 == 0) && (AttackActionCount < GameClock)) {
//           AttackActionCount = GameClock + EngageLevel * 10;
//           iVar3 = rand();
//           if (iVar3 == (iVar3 / 3) * 3) {
//             uVar6 = 0x4000;
//           }
//           return uVar6 | 0x80;
//         }
//       }
//     }
//     iVar3 = (int)Degree;
//     if (iVar3 < 0) {
//       iVar3 = -iVar3;
//     }
//     if ((iVar3 < 0x3e9) && (1999 < Distance)) {
//       if (Distance < 0xbb9) {
//         uVar4 = rand();
//         if ((uVar4 & 1) == 0) {
//           return uVar6;
//         }
//         if (Degree < 0x65) {
//           sVar2 = 3;
//           if (-0x65 < Degree) {
//             return uVar6;
//           }
//         }
//         else {
//           sVar2 = 4;
//         }
//       }
//       else {
//         if (Distance < 0xfa1) {
//           return uVar6 | 0x1000;
//         }
//         uVar4 = rand();
//         sVar2 = 1;
//         if ((uVar4 & 1) == 0) {
//           iVar3 = (int)Degree;
//           if (iVar3 < 0) {
//             iVar3 = -iVar3;
//           }
//           sVar2 = 0x21;
//           if (499 < iVar3) {
//             ItemUse();
//             return uVar6 | 0x1000;
//           }
//         }
//       }
//     }
//     else {
//       if (999 < Distance) {
//         return uVar6 | 0x4000;
//       }
//       iVar5 = rand();
//       iVar3 = iVar5;
//       if (iVar5 < 0) {
//         iVar3 = iVar5 + 3;
//       }
//       iVar5 = iVar5 + (iVar3 >> 2) * -4;
//       if (iVar5 == 1) {
//         return 0xa0;
//       }
//       if (iVar5 < 2) {
//         if (iVar5 != 0) {
//           return uVar6;
//         }
//         return 0x4040;
//       }
//       if (iVar5 != 2) {
//         if (iVar5 != 3) {
//           return uVar6;
//         }
//         return uVar6 | 0x80;
//       }
//       sVar2 = 2;
//     }
//     sVar2 = SetCommand(&Me_THINK_C->pad,sVar2);
//     return sVar2;
//   }
//   if (Me_THINK_C->motion->count != BattleDB[Me_THINK_C->warid].contfrm) {
//     return 0;
//   }
//   if (Distance < 2000) {
//     iVar3 = (int)Degree;
//     if (iVar3 < 0) {
//       iVar3 = -iVar3;
//     }
//     if (iVar3 < 500) goto LAB_8002e468;
//   }
//   iVar3 = rand();
//   iVar5 = EngageLevel + 1;
//   if (iVar5 == 0) {
//     trap(0x1c00);
//   }
//   if ((iVar5 == -1) && (iVar3 == -0x80000000)) {
//     trap(0x1800);
//   }
//   if (iVar3 % iVar5 != 0) {
//     return 0;
//   }
// LAB_8002e468:
//   if (Degree < 0x12d) {
//     if (-0x12d < Degree) {
//       return 0x80;
//     }
//     uVar6 = 0x8000;
//   }
//   else {
//     uVar6 = 0x2000;
//   }
//   return uVar6 | 0x80;
// }
