#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupTelop(unsigned char *telop);
 *     CHRANIM.C:372, 44 src lines, frame 560 bytes, saved-reg mask 0x801f0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $s4       unsigned char * telop
 *     reg   $a2       short * font
 *     stack sp+16     short [16][16] bitmap
 *     reg   $s0       short n
 *     reg   $t0       short u
 *     reg   $t1       short v
 *     stack sp+528    struct RECT rect
 *
 * Globals it touches, as the original declared them:
 *     extern struct POLY_FT4 TelopP;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetupTelop", SetupTelop);

// triage: HARD — 269 insns, 4 loop, frame 0x230, 6 callees, ~0.04 to BriefingAndInventorySelectionScreen
// likely-relevant cookbook sections:
//   - Loops: 4 back-edge(s) — for/while/do vs goto shape
//   - Stack objects: 0x230 frame — buffer casts / spills

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void SetupTelop(uchar *telop)
//
// {
//   short sVar1;
//   short sVar2;
//   undefined2 *puVar3;
//   int iVar4;
//   int iVar5;
//   int iVar6;
//   int in_a1;
//   int iVar7;
//   undefined2 *puVar8;
//   int iVar9;
//   ushort uVar10;
//   int iVar11;
//   int iVar12;
//   u_long auStack_220 [128];
//   RECT local_20;
//
//   TelopP.u1 = '\0';
//   TelopP.u0 = '\0';
//   if (((*telop & 0x80) != 0) && ((telop[2] & 0x80) != 0)) {
//     iVar5 = (in_a1 << 0x10) >> 0xc;
//     local_20.x = 0x300;
//     sVar1 = (short)iVar5;
//     local_20.y = 0x1f0 - sVar1;
//     local_20.w = 0x100;
//     local_20.h = 0xf;
//     ClearImage(&local_20,'\0','\0','\0');
//     DrawSync(0);
//     local_20.w = 0x10;
//     local_20.h = 0xf;
//     if (*telop == '\0') {
//       TelopP.u1 = '\0';
//       TelopP.u0 = '\0';
//     }
//     else {
//       iVar12 = 0;
//       do {
//         sVar2 = (short)iVar12;
//         if (0x1f < sVar2) break;
//         if ((telop[sVar2] == 0x81) && ((telop + sVar2)[1] == 0x99)) {
//           puVar8 = &DAT_8008f078;
//         }
//         else {
//           puVar8 = (undefined2 *)Krom2RawAdd((uint)CONCAT11(telop[sVar2],(telop + sVar2)[1]));
//         }
//         iVar11 = 0;
//         if (puVar8 != (undefined2 *)0xffffffff) {
//           do {
//             iVar4 = 0;
//             uVar10 = puVar8[(short)iVar11];
//             iVar6 = 0;
//             do {
//               puVar3 = (undefined2 *)
//                        ((int)auStack_220 + (0xf - (iVar6 >> 0x10)) * 2 + (short)iVar11 * 0x20);
//               if (((int)(short)(uVar10 >> 8 | uVar10 << 8) >> (iVar6 >> 0x10 & 0x1fU) & 1U) == 0) {
//                 *puVar3 = 0;
//               }
//               else {
//                 *puVar3 = 0x7fff;
//               }
//               iVar4 = iVar4 + 1;
//               iVar6 = iVar4 * 0x10000;
//             } while (iVar4 * 0x10000 >> 0x10 < 0x10);
//             iVar11 = iVar11 + 1;
//           } while (iVar11 * 0x10000 >> 0x10 < 0xf);
//           iVar4 = 1;
//           uVar10 = 1;
//           iVar11 = 0x10000;
//           do {
//             iVar11 = iVar11 >> 0x10;
//             iVar9 = iVar11 * 2;
//             iVar7 = (int)(short)uVar10;
//             iVar6 = iVar7 * 0x20;
//             if (*(short *)((int)auStack_220 + iVar9 + iVar6) == 0) {
//               sVar2 = *(short *)((int)auStack_220 + iVar9 + (iVar7 + -1) * 0x20);
//               if (((((sVar2 == 0x7fff) &&
//                     (*(short *)((int)auStack_220 + (iVar11 + -1) * 2 + iVar6) == 0x7fff)) ||
//                    ((*(short *)((int)auStack_220 + (iVar11 + -1) * 2 + iVar6) == 0x7fff &&
//                     (*(short *)((int)auStack_220 + iVar9 + (iVar7 + 1) * 0x20) == 0x7fff)))) ||
//                   ((*(short *)((int)auStack_220 + iVar9 + (iVar7 + 1) * 0x20) == 0x7fff &&
//                    (*(short *)((int)auStack_220 + (iVar11 + 1) * 2 + iVar6) == 0x7fff)))) ||
//                  ((*(short *)((int)auStack_220 + (iVar11 + 1) * 2 + iVar6) == 0x7fff &&
//                   (sVar2 == 0x7fff)))) {
//                 *(undefined2 *)
//                  ((int)auStack_220 + ((iVar4 << 0x10) >> 0xf) + ((int)((uint)uVar10 << 0x10) >> 0xb)
//                  ) = 0x1ce7;
//               }
//             }
//             iVar4 = iVar4 + 1;
//             if (0xe < iVar4 * 0x10000 >> 0x10) {
//               iVar4 = 1;
//               uVar10 = uVar10 + 1;
//             }
//             iVar11 = iVar4 << 0x10;
//           } while ((short)uVar10 < 0xe);
//           LoadImage(&local_20,auStack_220);
//           DrawSync(0);
//           local_20.x = local_20.x + local_20.w;
//         }
//         iVar12 = iVar12 + 2;
//       } while (telop[iVar12 * 0x10000 >> 0x10] != '\0');
//       memset((uchar *)&TelopP,0xff,0x28);
//       TelopP.tag._3_1_ = 9;
//       TelopP._2 = 0xf0 - (char)iVar5;
//       TelopP.code = ',';
//       TelopP.u2 = '\0';
//       TelopP.u0 = '\0';
//       TelopP.u1 = (char)local_20.x + 0xff;
//       TelopP.v2 = (char)local_20.h + TelopP._2;
//       TelopP._3 = TelopP._2;
//       TelopP.u3 = TelopP.u1;
//       TelopP.v3 = TelopP.v2;
//       TelopP.tpage = GetTPage(2,0,0x300,0x1f0 - sVar1);
//     }
//   }
//   return;
// }
