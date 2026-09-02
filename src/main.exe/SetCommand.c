#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "padcmd.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SetCommand(struct PADtype *pad, short cmd);
 *     PADCMD.C:359, 17 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t0       struct PADtype * pad
 *     param $a1       short cmd
 *     reg   $a2       unsigned short * com
 *     reg   $a0       short i
 *     reg   $a0       short j
 *     reg   $a1       short k
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned short *Command[12];
 * END PSX.SYM */

short SetCommand(PADtype *pad, pad_command cmd)
{
    s16 i;
    PadCommandSequence *entry;
    u16 *inputs;
    s16 n;
    s16 j;
    s32 one;
    s16 found;

    i = 0;
    while (Command[i] != 0)
    {
        entry = Command[i];
        found = ((u16)entry->command == cmd);
        one = 1;
        if (found)
        {
            inputs = entry->inputs;
            n = 0;
            while (inputs[n] != PAD_COMMAND_END)
            {
                n++;
            }
            if (one < n)
            {
                j = 1;
                do
                {
                    pad->stream[j - 1] = inputs[j];
                    j++;
                } while (j < n);
            }
            pad->time = one;
            return (s16)inputs[0];
        }
        i++;
    }
    return 0;
}
