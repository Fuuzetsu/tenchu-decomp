#include "common.h"
#include "main.exe.h"

/*
 * MATCH.
 *
 * Add a newly pressed pad state to the twelve-entry history and compare that
 * history with each 0xffff-terminated special-button sequence.  A successful
 * sequence is consumed by shifting the history once more and inserting zero.
 *
 * The combination table walk is deliberately written with an integer index.
 * loop.c strength-reduces it to the target's $a3 pointer induction, but creates
 * the initial low-half address after the hoisted history address.  A source
 * pointer cursor emits the same three instructions and registers in the wrong
 * schedule order (the former 12-byte residual).
 */
extern s16 *SPECIAL_BUTTON_COMBINATIONS_PTR[7];
extern u16 RECENTLY_PRESSED_BUTTONS[12];

s16 check_for_known_button_combination(u16 buttons, s16 newly_pressed)
{
    u16 *history;
    s32 combination_index;
    s16 *guard_entry;
    s16 *entry;
    s16 *pattern;
    s16 *pattern_start;
    u32 outer_end;
    u32 inner_end;
    s32 offset;
    s32 i;

    if (newly_pressed != 0) {
        i = 11;
        do {
            RECENTLY_PRESSED_BUTTONS[i] = RECENTLY_PRESSED_BUTTONS[i - 1];
            i--;
        } while (i > 0);
        guard_entry = SPECIAL_BUTTON_COMBINATIONS_PTR[0];
        do {
            do {
                do {
                    RECENTLY_PRESSED_BUTTONS[0] = buttons;
                } while (0);
            } while (0);
        } while (0);
        if (guard_entry != NULL) {
            outer_end = 0xffff;
            combination_index = 0;
            do {
                entry = SPECIAL_BUTTON_COMBINATIONS_PTR[combination_index];
                i = 0;
                pattern_start = entry + 1;
                if ((u16)entry[1] == outer_end)
                    goto matched;

                if (RECENTLY_PRESSED_BUTTONS[1] != 0)
                    inner_end = 0xffff;
                else
                    inner_end = 0xffff;
                pattern = pattern_start;
                history = RECENTLY_PRESSED_BUTTONS;
                do {
                    do {
                    } while (0);
                    if ((u16)*pattern != *history) {
                        offset = i << 1;
                        goto compare_end;
                    }
                    pattern++;
                    history++;
                    i++;
                } while ((u16)*pattern != inner_end);
                offset = i * 2;

compare_end:
                offset += (s32)pattern_start;
                if (*(u16 *)offset == outer_end) {
matched:
                    i = 11;
                    do {
                        RECENTLY_PRESSED_BUTTONS[i] = RECENTLY_PRESSED_BUTTONS[i - 1];
                        i--;
                    } while (i > 0);
                    RECENTLY_PRESSED_BUTTONS[0] = 0;
                    return SPECIAL_BUTTON_COMBINATIONS_PTR[combination_index][0];
                }

                combination_index++;
            } while (SPECIAL_BUTTON_COMBINATIONS_PTR[combination_index] != NULL);
        }
    }
    return 0;
}

// triage: MEDIUM — 72 insns, 4 loop, 0 callees, ~0.01 to ReturnNormal
// likely-relevant cookbook sections:
//   - Loops: 4 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// int FUN_80051f64(undefined2 param_1,short param_2)
//
// {
//   int iVar1;
//   undefined *puVar2;
//   short *psVar3;
//   int iVar4;
//   short *psVar5;
//   undefined **ppuVar6;
//
//   iVar4 = 0xb;
//   if (param_2 != 0) {
//     do {
//       iVar1 = iVar4 + -1;
//       (&DAT_800c2cf0)[iVar4] = (&DAT_800c2cf0)[iVar1];
//       iVar4 = iVar1;
//     } while (0 < iVar1);
//     DAT_800c2cf0 = param_1;
//     if (PTR_DAT_8008eddc != (undefined *)0x0) {
//       ppuVar6 = &PTR_DAT_8008eddc;
//       puVar2 = PTR_DAT_8008eddc;
//       do {
//         iVar4 = 0;
//         psVar3 = (short *)(puVar2 + 2);
//         psVar5 = &DAT_800c2cf0;
//         if (*(short *)(puVar2 + 2) == -1) {
// LAB_80052028:
//           iVar4 = 0xb;
//           do {
//             iVar1 = iVar4 + -1;
//             (&DAT_800c2cf0)[iVar4] = (&DAT_800c2cf0)[iVar1];
//             iVar4 = iVar1;
//           } while (0 < iVar1);
//           DAT_800c2cf0 = 0;
//           return (int)*(short *)*ppuVar6;
//         }
//         do {
//           iVar1 = iVar4 << 1;
//           if (*psVar3 != *psVar5) goto LAB_80052014;
//           psVar3 = psVar3 + 1;
//           iVar4 = iVar4 + 1;
//           psVar5 = psVar5 + 1;
//         } while (*psVar3 != -1);
//         iVar1 = iVar4 * 2;
// LAB_80052014:
//         if (*(short *)(iVar1 + (int)(puVar2 + 2)) == -1) goto LAB_80052028;
//         ppuVar6 = ppuVar6 + 1;
//         puVar2 = *ppuVar6;
//       } while (puVar2 != (undefined *)0x0);
//     }
//   }
//   return 0;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// extern u16 RECENTLY_PRESSED_BUTTONS;
// extern s16 *SPECIAL_BUTTON_COMBINATIONS_PTR;
//
// s16 check_for_known_button_combination(u16 arg0, s32 arg1) {
//     s16 **var_a3;
//     s16 *temp_t0;
//     s16 *var_a0;
//     s16 *var_v0;
//     s32 temp_v1;
//     s32 temp_v1_2;
//     s32 var_a1;
//     s32 var_a1_2;
//     s32 var_a1_3;
//     s32 var_v0_2;
//     u16 *var_a2;
//
//     var_a1 = 0xB;
//     if ((arg1 << 0x10) != 0) {
//         do {
//             temp_v1 = var_a1 * 2;
//             var_a1 -= 1;
//             *(temp_v1 + &RECENTLY_PRESSED_BUTTONS) = (&RECENTLY_PRESSED_BUTTONS)[var_a1];
//         } while (var_a1 > 0);
//         RECENTLY_PRESSED_BUTTONS = arg0;
//         if (SPECIAL_BUTTON_COMBINATIONS_PTR != NULL) {
//             var_a3 = &SPECIAL_BUTTON_COMBINATIONS_PTR;
//             var_v0 = SPECIAL_BUTTON_COMBINATIONS_PTR;
//             var_a1_2 = 0;
// loop_5:
//             temp_t0 = var_v0 + 2;
//             if (var_v0->unk2 != 0xFFFF) {
//                 var_a0 = temp_t0;
//                 var_a2 = &RECENTLY_PRESSED_BUTTONS;
// loop_7:
//                 var_v0_2 = var_a1_2 * 2;
//                 if ((u16) *var_a0 == *var_a2) {
//                     var_a0 += 2;
//                     var_a2 += 2;
//                     var_a1_2 += 1;
//                     if ((u16) *var_a0 == 0xFFFF) {
//                         var_v0_2 = var_a1_2 * 2;
//                     } else {
//                         goto loop_7;
//                     }
//                 }
//                 if (*(var_v0_2 + temp_t0) == 0xFFFF) {
//                     goto block_11;
//                 }
//                 var_a3 += 4;
//                 var_v0 = *var_a3;
//                 var_a1_2 = 0;
//                 if (var_v0 == NULL) {
//                     /* Duplicate return node #15. Try simplifying control flow for better match */
//                     return 0;
//                 }
//                 goto loop_5;
//             }
// block_11:
//             var_a1_3 = 0xB;
//             do {
//                 temp_v1_2 = var_a1_3 * 2;
//                 var_a1_3 -= 1;
//                 *(temp_v1_2 + &RECENTLY_PRESSED_BUTTONS) = (&RECENTLY_PRESSED_BUTTONS)[var_a1_3];
//             } while (var_a1_3 > 0);
//             RECENTLY_PRESSED_BUTTONS = 0;
//             return **var_a3;
//         }
//         /* Duplicate return node #15. Try simplifying control flow for better match */
//         return 0;
//     }
//     return 0;
// }
