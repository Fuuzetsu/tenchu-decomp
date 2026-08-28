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
    s32 i;

    if (newly_pressed != 0)
    {
        i = 11;
        do
        {
            RECENTLY_PRESSED_BUTTONS[i] = RECENTLY_PRESSED_BUTTONS[i - 1];
            i--;
        } while (i > 0);
        guard_entry = SPECIAL_BUTTON_COMBINATIONS_PTR[0];
        RECENTLY_PRESSED_BUTTONS[0] = buttons;
        if (guard_entry != NULL)
        {
            outer_end = 0xffff;
            combination_index = 0;
            do
            {
                entry = SPECIAL_BUTTON_COMBINATIONS_PTR[combination_index];
                i = 0;
                pattern_start = entry + 1;
                if ((u16)entry[1] == outer_end)
                    goto matched;

                /* Identical arms, and byte-required (measured both ways):
                 * the CONDITIONAL assignment survives loop.c's invariant
                 * motion, so inner_end's `li 0xffff` re-materializes inside
                 * the outer loop exactly where retail has it; jump threading
                 * later folds the branch and the dead [1] read away. A plain
                 * assignment gets hoisted (12B off), and literal 0xffff
                 * comparisons let cse unify the two end-marker constants
                 * into ONE register where retail keeps two (t3 outer, t1
                 * inner). No demo homolog exists to consult. */
                if (RECENTLY_PRESSED_BUTTONS[1] != 0)
                    inner_end = 0xffff;
                else
                    inner_end = 0xffff;
                pattern = pattern_start;
                history = RECENTLY_PRESSED_BUTTONS;
                do
                {

                    if ((u16)*pattern != *history)
                    {
                        goto compare_end;
                    }
                    pattern++;
                    history++;
                    i++;
                } while ((u16)*pattern != inner_end);

            compare_end:
                if ((u16)pattern_start[i] == outer_end)
                {
                matched:
                    i = 11;
                    do
                    {
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
