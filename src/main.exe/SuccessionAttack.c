#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short SuccessionAttack(long dist, short deg);
 *     THINK_3.C:247, 15 src lines, frame 24 bytes, saved-reg mask 0x80010000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $a0       long dist
 *     param $a1       short deg
 *
 * Globals it touches, as the original declared them:
 *     extern struct BattleType BattleDB[78];
 *     extern long Distance;
 *     extern short Degree;
 *     extern short EngageLevel;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SuccessionAttack", SuccessionAttack);

// triage: EASY — 67 insns, mul/div, 1 callees, ~0.11 to bow_shoot_logic
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short SuccessionAttack(long dist,short deg)
//
// {
//   int iVar1;
//   int iVar2;
//   ushort uVar3;
//
//   if (Me_THINK_C->motion->count != BattleDB[Me_THINK_C->warid].contfrm) {
//     return 0;
//   }
//   if (Distance < dist) {
//     iVar1 = (int)Degree;
//     if (iVar1 < 0) {
//       iVar1 = -iVar1;
//     }
//     if (iVar1 < deg) goto LAB_8002fb84;
//   }
//   iVar1 = rand();
//   iVar2 = EngageLevel + 1;
//   if (iVar2 == 0) {
//     trap(0x1c00);
//   }
//   if ((iVar2 == -1) && (iVar1 == -0x80000000)) {
//     trap(0x1800);
//   }
//   if (iVar1 % iVar2 != 0) {
//     return 0;
//   }
// LAB_8002fb84:
//   if (Degree < 0x12d) {
//     if (-0x12d < Degree) {
//       return 0x80;
//     }
//     uVar3 = 0x8000;
//   }
//   else {
//     uVar3 = 0x2000;
//   }
//   return uVar3 | 0x80;
// }
