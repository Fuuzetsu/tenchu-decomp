#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "padcmd.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetCommand(struct PADtype *pad);
 *     PADCMD.C:333, 22 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t0       struct PADtype * pad
 *     reg   $a0       unsigned short * cmd
 *     reg   $a2       short i
 *     reg   $a1       short j
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned short *Command[12];
 * END PSX.SYM */

pad_command GetCommand(PADtype *pad)
{
    u16 *pattern;
    short i;
    short j;

    for (i = 0; Command[i] != 0; i++)
    {
        pattern = Command[i]->inputs;
        for (j = 0; pattern[j] != PAD_COMMAND_END; j++)
        {
            if (pattern[j] != pad->stream[j])
                break;
        }
        if (pattern[j] != PAD_COMMAND_END)
            continue;

        j = PAD_COMMAND_STREAM_LENGTH - 1;
        do
        {
            pad->stream[j] = pad->stream[j - 1];
            j--;
        } while (j > 0);
        pad->stream[0] = 0;
        return Command[i]->command;
    }
    return CMD_NONE;
}
