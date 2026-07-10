#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern short SkipFrame;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_800514d8", FUN_800514d8);

// triage: EASY — 54 insns, 1 loop, 4 callees, ~0.06 to DoBriefingAndInventorySelection
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_800514d8(void)
//
// {
//   short sVar1;
//   int iVar2;
//   int iVar3;
//   int unaff_s1;
//
//   if ((uint)(byte)PersistentState._5_1_ != (int)DAT_8008ea78) {
//     if ((byte)(&PersistentState.field_0x60)[(byte)PersistentState._4_1_] <
//         StageConfig[(byte)PersistentState._5_1_].uid) {
//       (&PersistentState.field_0x60)[(byte)PersistentState._4_1_] =
//            StageConfig[(byte)PersistentState._5_1_].uid;
//     }
//   }
//   do {
//     StartDrawing();
//     iVar2 = GetRealPad(0);
//     iVar3 = 0;
//     if ((unaff_s1 == 0) && (iVar2 != 0)) {
//       iVar3 = iVar2;
//     }
//     sVar1 = FUN_8005a7a4(iVar3);
//     SkipFrame = 2;
//     EndDrawing(2);
//     unaff_s1 = iVar2;
//   } while (sVar1 == 0);
//   return;
// }
