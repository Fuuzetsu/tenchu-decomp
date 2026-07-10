#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DoMiscProc(void);
 *     MISC.C:713, 19 src lines, frame 32 bytes, saved-reg mask 0x80070000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $s1       int i
 *     reg   $s1       int i
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TMisc misc[200];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DoMiscProc", DoMiscProc);

// triage: MEDIUM — 113 insns, mul/div, 2 loop, indirect-call, 1 callees, ~0.08 to ResetAllMisc
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void DoMiscProc(void)
//
// {
//   int iVar1;
//   tag_TMisc *ptVar2;
//   int iVar3;
//
//   if (DAT_80097c44 == '\0') {
//     AdtMessageBox("misc not initialized");
//   }
//   else {
//     if (GameClock == (GameClock / 10) * 10) {
//       iVar3 = 0;
//       ptVar2 = misc;
//       do {
//         if (ptVar2->proc != (undefined **)0x0) {
//           iVar1 = ViewInfo.vrx - ptVar2->x;
//           if (iVar1 < 0) {
//             iVar1 = -iVar1;
//           }
//           if (iVar1 < 15000) {
//             iVar1 = ViewInfo.vry - ptVar2->y;
//             if (iVar1 < 0) {
//               iVar1 = -iVar1;
//             }
//             if (iVar1 < 15000) {
//               iVar1 = ViewInfo.vrz - ptVar2->z;
//               if (iVar1 < 0) {
//                 iVar1 = -iVar1;
//               }
//               if (iVar1 < 15000) {
//                 if (ptVar2->pause != '\0') {
//                   (*(code *)ptVar2->proc)(ptVar2,3);
//                   ptVar2->pause = '\0';
//                 }
//                 goto LAB_8004d49c;
//               }
//             }
//           }
//           if (ptVar2->pause == '\0') {
//             (*(code *)ptVar2->proc)(ptVar2,2);
//             ptVar2->pause = '\x01';
//           }
//         }
// LAB_8004d49c:
//         iVar3 = iVar3 + 1;
//         ptVar2 = ptVar2 + 1;
//       } while (iVar3 < 200);
//     }
//     _DrawTMDmode = 0x20;
//     iVar3 = 0;
//     ptVar2 = misc;
//     do {
//       if ((ptVar2->proc != (undefined **)0x0) && (ptVar2->pause == '\0')) {
//         (*(code *)ptVar2->proc)(ptVar2,4);
//       }
//       iVar3 = iVar3 + 1;
//       ptVar2 = ptVar2 + 1;
//     } while (iVar3 < 200);
//   }
//   return;
// }
