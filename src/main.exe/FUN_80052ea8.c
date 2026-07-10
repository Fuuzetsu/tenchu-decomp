#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80052ea8", FUN_80052ea8);

// triage: HARD — 311 insns, mul/div, 6 loop, 1 callees, ~0.06 to BriefingAndInventorySelectionScreen
// likely-relevant cookbook sections:
//   - Loops: 6 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_80052ea8(int param_1,int param_2)
//
// {
//   char cVar1;
//   int iVar2;
//   int iVar3;
//   int iVar4;
//   short sVar5;
//
//   iVar2 = (int)((4 - (uint)*(ushort *)(param_2 + 10)) * 0x10000) >> 0x10;
//   if (iVar2 < 3) {
//     if (iVar2 == 2) {
//       iVar2 = 1;
//       iVar3 = 0x10000;
//       do {
//         iVar4 = param_1 + (iVar3 >> 0x10) + (uint)*(byte *)(param_1 + 4) * 0x20;
//         if (*(char *)(iVar4 + 0x40c) == -2) {
//           *(undefined1 *)(iVar4 + 0x40c) = 0;
//         }
//         iVar2 = iVar2 + 1;
//         iVar3 = param_1 + (iVar3 >> 0x10) + (uint)*(byte *)(param_1 + 4) * 0x20;
//         iVar4 = iVar2 * 0x10000 >> 0x10;
//         *(char *)(iVar3 + 0x40c) = *(char *)(iVar3 + 0x40c) + '\x01';
//         iVar3 = iVar2 * 0x10000;
//       } while (iVar4 < 9);
//       while (iVar4 < 0x14) {
//         iVar3 = param_1 + (int)(short)iVar2 + (uint)*(byte *)(param_1 + 4) * 0x20;
//         cVar1 = *(char *)(iVar3 + 0x40c);
//         if (cVar1 != -2) {
//           *(char *)(iVar3 + 0x40c) = cVar1 + '\x01';
//         }
//         iVar2 = iVar2 + 1;
//         iVar4 = iVar2 * 0x10000 >> 0x10;
//       }
//     }
//     else {
//       iVar3 = 1;
//       if (iVar2 == 1) {
//         iVar2 = 1;
//         iVar3 = 0x10000;
//         do {
//           iVar4 = param_1 + (iVar3 >> 0x10) + (uint)*(byte *)(param_1 + 4) * 0x20;
//           if (*(char *)(iVar4 + 0x40c) == -2) {
//             *(undefined1 *)(iVar4 + 0x40c) = 0;
//           }
//           iVar2 = iVar2 + 1;
//           iVar3 = param_1 + (iVar3 >> 0x10) + (uint)*(byte *)(param_1 + 4) * 0x20;
//           iVar4 = iVar2 * 0x10000 >> 0x10;
//           *(char *)(iVar3 + 0x40c) = *(char *)(iVar3 + 0x40c) + '\x01';
//           iVar3 = iVar2 * 0x10000;
//         } while (iVar4 < 9);
//         while (iVar4 < 0x14) {
//           iVar3 = param_1 + (int)(short)iVar2 + (uint)*(byte *)(param_1 + 4) * 0x20;
//           cVar1 = *(char *)(iVar3 + 0x40c);
//           if (cVar1 != -2) {
//             *(char *)(iVar3 + 0x40c) = cVar1 + '\x01';
//           }
//           iVar2 = iVar2 + 1;
//           iVar4 = iVar2 * 0x10000 >> 0x10;
//         }
//         sVar5 = 5;
//         do {
//           iVar2 = rand();
//           iVar2 = (iVar2 % 0x12 + 1) * 0x10000 >> 0x10;
//           if (iVar2 < 9) {
//             iVar3 = param_1 + iVar2 + (uint)*(byte *)(param_1 + 4) * 0x20;
//             if (*(char *)(iVar3 + 0x40c) == -2) {
//               *(undefined1 *)(iVar3 + 0x40c) = 0;
//             }
//             iVar2 = param_1 + iVar2 + (uint)*(byte *)(param_1 + 4) * 0x20;
//             sVar5 = sVar5 + -1;
//             *(char *)(iVar2 + 0x40c) = *(char *)(iVar2 + 0x40c) + '\x01';
//           }
//           else {
//             iVar2 = param_1 + iVar2 + (uint)*(byte *)(param_1 + 4) * 0x20;
//             cVar1 = *(char *)(iVar2 + 0x40c);
//             if (cVar1 != -2) {
//               *(char *)(iVar2 + 0x40c) = cVar1 + '\x01';
//               sVar5 = sVar5 + -1;
//             }
//           }
//         } while (sVar5 != 0);
//       }
//       else {
//         iVar2 = 0x10000;
//         do {
//           iVar4 = param_1 + (iVar2 >> 0x10) + (uint)*(byte *)(param_1 + 4) * 0x20;
//           if (*(char *)(iVar4 + 0x40c) == -2) {
//             *(undefined1 *)(iVar4 + 0x40c) = 0;
//           }
//           iVar3 = iVar3 + 1;
//           iVar2 = param_1 + (iVar2 >> 0x10) + (uint)*(byte *)(param_1 + 4) * 0x20;
//           iVar4 = iVar3 * 0x10000 >> 0x10;
//           *(char *)(iVar2 + 0x40c) = *(char *)(iVar2 + 0x40c) + '\x02';
//           iVar2 = iVar3 * 0x10000;
//         } while (iVar4 < 9);
//         while (iVar4 < 0x14) {
//           iVar2 = param_1 + (int)(short)iVar3 + (uint)*(byte *)(param_1 + 4) * 0x20;
//           cVar1 = *(char *)(iVar2 + 0x40c);
//           if (cVar1 != -2) {
//             *(char *)(iVar2 + 0x40c) = cVar1 + '\x02';
//           }
//           iVar3 = iVar3 + 1;
//           iVar4 = iVar3 * 0x10000 >> 0x10;
//         }
//         iVar2 = param_1 + (int)*(short *)(&DAT_8008ed50 + (uint)*(byte *)(param_1 + 5) * 2) +
//                           (uint)*(byte *)(param_1 + 4) * 0x20;
//         if (*(char *)(iVar2 + 0x40c) == -2) {
//           *(undefined1 *)(iVar2 + 0x40c) = 1;
//         }
//       }
//     }
//   }
//   else {
//     sVar5 = 5;
//     if (iVar2 == 4) {
//       sVar5 = 3;
//     }
//     while (sVar5 != 0) {
//       iVar2 = rand();
//       iVar2 = (iVar2 % 0x12 + 1) * 0x10000 >> 0x10;
//       if (iVar2 < 9) {
//         iVar3 = param_1 + iVar2 + (uint)*(byte *)(param_1 + 4) * 0x20;
//         if (*(char *)(iVar3 + 0x40c) == -2) {
//           *(undefined1 *)(iVar3 + 0x40c) = 0;
//         }
//         iVar2 = param_1 + iVar2 + (uint)*(byte *)(param_1 + 4) * 0x20;
//         sVar5 = sVar5 + -1;
//         *(char *)(iVar2 + 0x40c) = *(char *)(iVar2 + 0x40c) + '\x01';
//       }
//       else {
//         iVar2 = param_1 + iVar2 + (uint)*(byte *)(param_1 + 4) * 0x20;
//         cVar1 = *(char *)(iVar2 + 0x40c);
//         if (cVar1 != -2) {
//           *(char *)(iVar2 + 0x40c) = cVar1 + '\x01';
//           sVar5 = sVar5 + -1;
//         }
//       }
//     }
//   }
//   param_1 = param_1 + (uint)*(byte *)(param_1 + 4) * 0x20;
//   if (*(char *)(param_1 + 0x41f) != -2) {
//     *(undefined1 *)(param_1 + 0x41f) = 1;
//   }
//   return;
// }
