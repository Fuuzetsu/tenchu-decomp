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

/*
 * MATCHED: SetCommand (0x8001b038, 0x10c bytes) finds cmd in the
 * NULL-terminated Command table. Each entry contains an id followed by a
 * 0xFFFF-terminated argument list. On a hit it copies arguments after the
 * first into pad->stream, sets pad->time to 1, and returns the first argument;
 * a miss returns zero.
 *
 * Matching constraints:
 *  - The outer search is i = 0; while (Command[i] != 0), with i++ last in the
 *    body. Reorg moves that increment into the comparison branch's delay slot
 *    while preserving old and new i separately. An increment before the if
 *    becomes an in-place add and changes the schedule. The plain draft with
 *    that early increment and a direct comparison had the right length but
 *    left 46 of 268 bytes different.
 *  - found = (entry[0] == cmd) must precede one = 1. This creates cmd's
 *    sign-extension RTL before the otherwise independent invariant constant,
 *    giving the scheduler the target order. A direct if or earlier one
 *    assignment reverses those independent invariant operations.
 *  - Argument counting is while (args[n] != 0xFFFF). Copying is an explicitly
 *    guarded do/while; spelling it as for/while adds jump.c's duplicated front
 *    test on top of the source guard.
 *  - one is an unconditional s32 assignment in the outer loop. Because every
 *    iteration reaches it, loop.c hoists one materialization, and the copy
 *    guard and pad->time store share that value. A literal uses slti; s16
 *    creates a second widened copy.
 *  - The matched-entry do { ... } while (0) is intentional loop-depth
 *    weighting. Removing it changes the outer index and table-base allocation.
 */
short SetCommand(PADtype *pad, short cmd)
{
    s16 i;
    COMMAND *entry;
    COMMAND *args;
    s16 n;
    s16 j;
    s32 one;
    s16 found;

    i = 0;
    while (Command[i] != 0)
    {
        entry = Command[i];
        found = (entry[0] == cmd);
        one = 1;
        if (found)
        {
            args = entry + 1;
            n = 0;
            while (args[n] != 0xFFFF)
            {
                n++;
            }
            if (one < n)
            {
                j = 1;
                do
                {
                    pad->stream[j - 1] = args[j];
                    j++;
                } while (j < n);
            }
            pad->time = one;
            return (s16)args[0];
        }
        i++;
    }
    return 0;
}
