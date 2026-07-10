#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PadProc(void);
 *     PADCMD.C:249, 25 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $a0       int ct
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char ComBuf[2][34];
 *     extern struct tag_TItem items[30];
 *     extern struct PADCMD__141fake PadArrange;
 *     extern struct TPadPort PadPort[2][4];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PadProc", PadProc);

// triage: MEDIUM — 92 insns, mul/div, 1 callees, ~0.06 to bow_shoot_logic
// likely-relevant cookbook sections:
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void PadProc(void)
//
// {
//   int iVar1;
//
//   ComPad(0,ComBuf[0]);
//   ComPad(0x10,ComBuf[0x11]);
//   iVar1 = PadArrange.attack - PadArrange.time;
//   if (iVar1 < 1) {
//     if (iVar1 + PadArrange.release < 1) {
//       DAT_800be6d8 = 0;
//       DAT_800be6d9 = 0;
//       PadArrange.time = PadArrange.time + 1;
//       return;
//     }
//     iVar1 = PadArrange.pow * (iVar1 + PadArrange.release);
//     if (PadArrange.release == 0) {
//       trap(0x1c00);
//     }
//     if ((PadArrange.release == -1) && (iVar1 == -0x80000000)) {
//       trap(0x1800);
//     }
//     if (PersistentState._93_1_ == '\0') {
//       DAT_800be6d8 = 0;
//       DAT_800be6d9 = 0;
//     }
//     else {
//       DAT_800be6d8 = 0;
//       DAT_800be6d9 = (undefined1)(iVar1 / PadArrange.release);
//     }
//   }
//   else {
//     iVar1 = PadArrange.pow * (PadArrange.attack - iVar1);
//     if (PadArrange.attack == 0) {
//       trap(0x1c00);
//     }
//     if ((PadArrange.attack == -1) && (iVar1 == -0x80000000)) {
//       trap(0x1800);
//     }
//     if (PersistentState._93_1_ == '\0') {
//       DAT_800be6d8 = 0;
//       DAT_800be6d9 = 0;
//     }
//     else {
//       DAT_800be6d8 = 1;
//       DAT_800be6d9 = (undefined1)(iVar1 / PadArrange.attack);
//     }
//   }
//   PadArrange.time = PadArrange.time + 2;
//   return;
// }
