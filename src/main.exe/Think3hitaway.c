#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3hitaway(void);
 *     THINK_3.C:172, 33 src lines, frame 24 bytes, saved-reg mask 0x80000000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $a1       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern short Degree;
 *     extern short Attrib;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/Think3hitaway", Think3hitaway);

// triage: MEDIUM — 97 insns, mul/div, indirect-call, 4 callees, ~0.07 to AttackAnimal
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short Think3hitaway(void)
//
// {
//   Humanoid *pHVar1;
//   ushort uVar2;
//   int iVar3;
//
//   if ((Distance < 10000) && (SR != -2)) {
//     SR = 0;
//   }
//   if (Me_THINK_C->status == 7) {
//     Me_THINK_C->actflg = '\0';
//     pHVar1 = Me_THINK_C;
//     Me_THINK_C->chase[1] = 0;
//     pHVar1->chase[0] = 0;
//     uVar2 = SuccessionAttack(3000,0x5dc);
//   }
//   else if (Me_THINK_C->actflg == '\0') {
//     iVar3 = (int)Degree;
//     if (iVar3 < 0) {
//       iVar3 = -iVar3;
//     }
//     if (iVar3 < 1000) {
//       uVar2 = FUN_8002b990(0,0);
//       uVar2 = uVar2 & 0xe000 | 0x4000;
//     }
//     else {
//       uVar2 = ChasetoTarget(5000);
//     }
//     if ((Distance < 2000) && (iVar3 = rand(), iVar3 == (iVar3 / 0x1e) * 0x1e)) {
//       uVar2 = uVar2 | 0x40;
//     }
//     if ((4000 < Distance) || ((Attrib & 0x400U) != 0)) {
//       Me_THINK_C->actflg = '\x01';
//     }
//   }
//   else {
//     uVar2 = (*(code *)AttackFunc[(int)((uint)(ushort)Me_THINK_C->wpatk << 0x10) >> 0x14])();
//   }
//   return uVar2;
// }
