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

/*
 * STATUS: MATCHED — exact retail bytes (292 bytes, 73 instructions).
 *
 * GetCommand (0x8001af14) — checks pad->stream[] (the recent-input ring,
 * newest first) against every entry in the global Command table (same
 * NULL-terminated `COMMAND *` table as SetCommand.c: entry[0] = command id,
 * entry[1..] = a 0xFFFF-terminated button-sequence pattern). The first
 * entry whose pattern is a prefix of pad->stream[] wins: pad->stream[] is
 * shifted down by one (dropping the oldest, stream[0] cleared for the next
 * frame) and entry[0] is returned. Returns 0 if no entry matches.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - Outer loop is a plain rotated `for (i = 0; Command[i] != 0; i++)` —
 *    jump.c elides the front test (i=0 provably true) and fuses the
 *    increment into the "go check the next entry" continuation, matching
 *    the asm computing `i+1` only as part of testing `Command[i+1]`.
 *  - Inner matching is a natural
 *    `for (j = 0; cmd[j] != PAD_COMMAND_END; j++)`.
 *    That source form makes cc1 keep the initial sentinel test in the loop
 *    preheader and materialize a separate 0xFFFF carrier for the backedge;
 *    the resulting live ranges place `cmd`, `i`, and `pad` in the registers
 *    recorded by PSX.SYM. Spelling this as an explicit guard plus do-while
 *    merges the two sentinel identities and displaces those registers.
 *  - The shift loop is Ghidra's literal do-while (`j = 3; do {
 *    pad->stream[j] = pad->stream[j-1]; j--; } while (j > 0);`), not a
 *    `for` — matches the single post-decrement bottom test with no
 *    redundant front check.
 *  - The final return re-derives `Command[i]` fresh (`*(short *)Command[i]`)
 *    rather than reusing an `entry` variable — the asm reloads it by index
 *    at the return, not from a cached pointer.
 */

pad_command GetCommand(PADtype *pad)
{
    COMMAND *cmd;
    short i;
    short j;

    for (i = 0; Command[i] != 0; i++)
    {
        cmd = Command[i] + 1;
        for (j = 0; cmd[j] != PAD_COMMAND_END; j++)
        {
            if (cmd[j] != pad->stream[j])
                break;
        }
        if (cmd[j] != PAD_COMMAND_END)
            continue;

        j = PAD_COMMAND_STREAM_LENGTH - 1;
        do
        {
            pad->stream[j] = pad->stream[j - 1];
            j--;
        } while (j > 0);
        pad->stream[0] = 0;
        /* (pad_command *): the return is an lh where every table read above
         * is lhu — byte-required (verified against the .s). */
        return *(pad_command *)Command[i];
    }
    return CMD_NONE;
}
