#include "common.h"
#include "main.exe.h"

/*
 * FUN_80052ea8 (0x80052ea8) — awards inventory stock at the end of a
 * stage.  The result kind chooses a deterministic one- or two-item sweep,
 * optional random bonuses, and the stage-specific reward; locked stock
 * entries use 0xfe and are opened by adding through the byte value.
 *
 * STATUS: MATCHED — exact 1244 bytes / 311 instructions.
 *
 * Matching notes:
 *  - Keep the three deterministic sweeps as separate source loops.  cc1
 *    emits the target's repeated blocks and keeps their s16 induction
 *    variables in caller-saved registers; factoring them would change the
 *    control flow.
 *  - The final per-character flag first forms a raw row base and then uses
 *    row[0x41f].  Writing it as state->stock[chr*0x20 + 0x13] makes cc1 add
 *    0x13 to the index in a separate instruction instead of folding 0x41f
 *    into the lbu/sb memory operands.
 *  - `kind` and `remaining` are signed 16-bit values.  An unsigned-width
 *    mechanical rewrite happens to restore the instruction count while
 *    replacing the required sll/sra sign extension with andi/sltiu.
 */

typedef struct
{
    u8 pad[10];
    u16 reward_kind;
} EndStageResult;

extern s16 D_8008ED50[];
extern s32 rand(void);

void FUN_80052ea8(TLinkInfo *state, EndStageResult *result)
{
    s16 kind;
    s16 i;
    s16 remaining;
    u8 *row;

    kind = 4 - result->reward_kind;
    if (kind >= 3)
    {
        remaining = 5;
        if (kind == 4)
        {
            remaining = 3;
        }
        while (remaining != 0)
        {
            i = rand() % 18 + 1;
            if (i < 9)
            {
                if (state->stock[i + state->CharType * 0x20] == 0xfe)
                {
                    state->stock[i + state->CharType * 0x20] =
                        state->stock[i + state->CharType * 0x20] + 2;
                }
                state->stock[i + state->CharType * 0x20] =
                    state->stock[i + state->CharType * 0x20] + 1;
                remaining--;
            }
            else if (state->stock[i + state->CharType * 0x20] != 0xfe)
            {
                state->stock[i + state->CharType * 0x20] =
                    state->stock[i + state->CharType * 0x20] + 1;
                remaining--;
            }
        }
    }
    else if (kind == 2)
    {
        i = 1;
        do
        {
            if (state->stock[i + state->CharType * 0x20] == 0xfe)
            {
                state->stock[i + state->CharType * 0x20] =
                    state->stock[i + state->CharType * 0x20] + 2;
            }
            state->stock[i + state->CharType * 0x20] =
                state->stock[i + state->CharType * 0x20] + 1;
            i++;
        } while (i < 9);
        while (i < 0x14)
        {
            if (state->stock[i + state->CharType * 0x20] != 0xfe)
            {
                state->stock[i + state->CharType * 0x20] =
                    state->stock[i + state->CharType * 0x20] + 1;
            }
            i++;
        }
    }
    else if (kind == 1)
    {
        i = 1;
        do
        {
            if (state->stock[i + state->CharType * 0x20] == 0xfe)
            {
                state->stock[i + state->CharType * 0x20] =
                    state->stock[i + state->CharType * 0x20] + 2;
            }
            state->stock[i + state->CharType * 0x20] =
                state->stock[i + state->CharType * 0x20] + 1;
            i++;
        } while (i < 9);
        while (i < 0x14)
        {
            if (state->stock[i + state->CharType * 0x20] != 0xfe)
            {
                state->stock[i + state->CharType * 0x20] =
                    state->stock[i + state->CharType * 0x20] + 1;
            }
            i++;
        }

        remaining = 5;
        do
        {
            i = rand() % 18 + 1;
            if (i < 9)
            {
                if (state->stock[i + state->CharType * 0x20] == 0xfe)
                {
                    state->stock[i + state->CharType * 0x20] =
                        state->stock[i + state->CharType * 0x20] + 2;
                }
                state->stock[i + state->CharType * 0x20] =
                    state->stock[i + state->CharType * 0x20] + 1;
                remaining--;
            }
            else if (state->stock[i + state->CharType * 0x20] != 0xfe)
            {
                state->stock[i + state->CharType * 0x20] =
                    state->stock[i + state->CharType * 0x20] + 1;
                remaining--;
            }
        } while (remaining != 0);
    }
    else
    {
        i = 1;
        do
        {
            if (state->stock[i + state->CharType * 0x20] == 0xfe)
            {
                state->stock[i + state->CharType * 0x20] =
                    state->stock[i + state->CharType * 0x20] + 2;
            }
            state->stock[i + state->CharType * 0x20] =
                state->stock[i + state->CharType * 0x20] + 2;
            i++;
        } while (i < 9);
        while (i < 0x14)
        {
            if (state->stock[i + state->CharType * 0x20] != 0xfe)
            {
                state->stock[i + state->CharType * 0x20] =
                    state->stock[i + state->CharType * 0x20] + 2;
            }
            i++;
        }

        i = D_8008ED50[state->StageNo];
        if (state->stock[i + state->CharType * 0x20] == 0xfe)
        {
            state->stock[i + state->CharType * 0x20] =
                state->stock[i + state->CharType * 0x20] + 3;
        }
    }

    row = (u8 *)state + state->CharType * 0x20;
    if (row[0x41f] != 0xfe)
    {
        row[0x41f] = 1;
    }
}

// triage: HARD — 311 insns, mul/div, 6 loop, 1 callees, ~0.06 to BriefingAndInventorySelectionScreen
// likely-relevant cookbook sections:
//   - Loops: 6 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (historical reference used for the matched C above):
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
