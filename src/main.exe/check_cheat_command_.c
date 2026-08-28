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
extern s16 *CHEAT_COMMANDS_[7];
extern u16 PAD_HISTORY_[12];

s16 check_cheat_command_(u16 buttons, s16 newly_pressed)
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
            PAD_HISTORY_[i] = PAD_HISTORY_[i - 1];
            i--;
        } while (i > 0);
        guard_entry = CHEAT_COMMANDS_[0];
        PAD_HISTORY_[0] = buttons;
        if (guard_entry != NULL)
        {
            outer_end = 0xffff;
            combination_index = 0;
            do
            {
                entry = CHEAT_COMMANDS_[combination_index];
                i = 0;
                pattern_start = entry + 1;
                if ((u16)entry[1] == outer_end)
                    goto matched;

                /* Identical arms, and byte-required (measured both ways,
                 * then pinned against gcc 2.8.1's own sources): retail keeps
                 * TWO 0xffff registers (t3 outer, t1 inner), and cse1 unifies
                 * any straight-line `inner_end = 0xffff` — or literal
                 * comparisons — with outer_end's constant into one. The
                 * conditional's join makes inner_end's value flow-dependent,
                 * which is the only thing that hides the constant from cse
                 * (loop.c could not hoist it anyway: a REG_USERVAR set past
                 * the matched-exit jump is maybe_never and fails all three
                 * movability clauses). A ternary folds at tree level and
                 * fails the same way; jump threading later deletes the
                 * branch and the dead [1] read. No demo homolog exists. */
                if (PAD_HISTORY_[1] != 0)
                    inner_end = 0xffff;
                else
                    inner_end = 0xffff;
                pattern = pattern_start;
                history = PAD_HISTORY_;
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
                        PAD_HISTORY_[i] = PAD_HISTORY_[i - 1];
                        i--;
                    } while (i > 0);
                    PAD_HISTORY_[0] = 0;
                    return CHEAT_COMMANDS_[combination_index][0];
                }

                combination_index++;
            } while (CHEAT_COMMANDS_[combination_index] != NULL);
        }
    }
    return 0;
}
