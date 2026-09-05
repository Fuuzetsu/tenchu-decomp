#include "common.h"
#include "main.exe.h"
#include "padcmd.h"

enum cheat_command check_cheat_command_(s16 buttons, s16 newly_pressed)
{
    s32 combination_index;
    CheatCommandSequence *entry;
    const u16 *pattern;
    const u16 *pattern_start;
    const u16 *history;
    u32 outer_end;
    u32 inner_end;
    u16 pattern_value;
    s32 i;

    if (newly_pressed != 0)
    {
        for (i = N_CHEAT_HISTORY_ENTRIES - 1; i > 0; i--)
        {
            PAD_HISTORY_[i] = PAD_HISTORY_[i - 1];
        }
        PAD_HISTORY_[0] = buttons;
        if (CHEAT_COMMANDS_[0] != NULL)
        {
            outer_end = CHEAT_COMMAND_END;
            combination_index = 0;
            do
            {
                const u16 *history_start;

                entry = CHEAT_COMMANDS_[combination_index];
                i = 0;
                history_start = PAD_HISTORY_;
                pattern_start = entry->presses;
                pattern_value = *pattern_start;
                if (pattern_value != outer_end)
                {
                    inner_end = CHEAT_COMMAND_END;
                    pattern = pattern_start;
                    history = history_start;
                    do
                    {
                        if (*pattern == *history)
                        {
                            pattern++;
                            history++;
                            i++;
                        }
                        else
                        {
                            break;
                        }
                    } while (*pattern != inner_end);
                    pattern_value = pattern_start[i];
                }

                if (pattern_value == outer_end)
                {
                    for (i = N_CHEAT_HISTORY_ENTRIES - 1; i > 0; i--)
                    {
                        PAD_HISTORY_[i] = PAD_HISTORY_[i - 1];
                    }
                    PAD_HISTORY_[0] = 0;
                    return CHEAT_COMMANDS_[combination_index]->result;
                }

                combination_index++;
            } while (CHEAT_COMMANDS_[combination_index] != NULL);
        }
    }
    return CHEAT_NONE;
}
