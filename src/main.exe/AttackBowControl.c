#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackBowControl(void);
 *     MOTION.C:800, 28 src lines, frame 72 bytes, saved-reg mask 0x80000000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 *     reg   $v0       struct VECTOR * pos
 *     stack sp+56     struct SVECTOR vect
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short MotionUpdateMode;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/AttackBowControl", AttackBowControl);

// triage: EASY — 68 insns, 5 callees, ~0.06 to ProcItemGoshikimai
// likely-relevant cookbook sections:
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void AttackBowControl(void)
//
// {
//   short sVar1;
//   int iVar2;
//   VECTOR *pVVar3;
//   int in_a0;
//
//   sVar1 = dtM->count;
//   if (sVar1 == 1) {
//     Sound(Me_MOTION_C,2);
//   }
//   else {
//     iVar2 = (in_a0 << 0x10) >> 0xe;
//     if ((*(short *)(&DAT_80097714 + iVar2) <= sVar1) && (sVar1 < *(short *)(&DAT_80097716 + iVar2)))
//     {
//       UpdateOrnament(Me_MOTION_C->weapon[2],0);
//       DrawOrnament(Me_MOTION_C->weapon[2]);
//     }
//   }
//   if (dtM->count == *(short *)(&DAT_80097716 + ((in_a0 << 0x10) >> 0xe))) {
//     pVVar3 = GetAbsolutePosition(Me_MOTION_C->model->object[0xd],0,0,0);
//     FUN_80027554(0x15,pVVar3);
//     Sound(Me_MOTION_C,3);
//   }
//   return;
// }
