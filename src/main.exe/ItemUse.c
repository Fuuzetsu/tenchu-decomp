#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short ItemUse(void);
 *     THINK_3.C:225, 18 src lines, frame 24 bytes, saved-reg mask 0x80010000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $s0       short id
 *
 * Globals it touches, as the original declared them:
 *     extern short Degree;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ItemUse", ItemUse);

// triage: EASY — 67 insns, mul/div, 2 callees, ~0.07 to AttackAnimal
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short ItemUse(void)
//
// {
//   short sVar1;
//   int iVar2;
//
//   if (Me_THINK_C->motion->count != 0) {
//     return 5;
//   }
//   if (Me_THINK_C->status != 5) {
//     return 5;
//   }
//   if ((Me_THINK_C->item[3] != '\0') &&
//      (iVar2 = ((int)Me_THINK_C->lifemax / 3) * 0x10000 >> 0x10, Me_THINK_C->life < iVar2)) {
//     ReqItemDefault(Me_THINK_C,ITEM_KUSURI);
//     return (short)iVar2;
//   }
//   if (Me_THINK_C->item[1] != '\0') {
//     iVar2 = (int)Degree;
//     if (iVar2 < 0) {
//       iVar2 = -iVar2;
//     }
//     sVar1 = 0xe00;
//     if (iVar2 < 100) goto LAB_8002dd00;
//   }
//   if (Me_THINK_C->item[4] == '\0') {
//     return 0;
//   }
//   iVar2 = (int)Degree;
//   if (iVar2 < 0) {
//     iVar2 = -iVar2;
//   }
//   sVar1 = 0xf02;
//   if (iVar2 >= 300) {
//     return (ushort)(iVar2 < 300);
//   }
// LAB_8002dd00:
//   sVar1 = SetNowMotion(Me_THINK_C,sVar1,1);
//   return sVar1;
// }
