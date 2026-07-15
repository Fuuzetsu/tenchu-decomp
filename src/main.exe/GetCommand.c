#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetCommand(struct PADtype *pad);
 *     PADCMD.C:333, 22 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $t0       struct PADtype * pad
 *     reg   $a0       unsigned short * cmd
 *     reg   $a2       short i
 *     reg   $a1       short j
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned short *Command[12];
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 59 of 292 bytes differ (same LENGTH as target,
 * 73 instructions both sides); every branch/loop shape and field access
 * this function establishes is otherwise correct (independently proven —
 * see notes below).
 *
 * GetCommand (0x8001af14) — checks pad->stream[] (the recent-input ring,
 * newest first) against every entry in the global Command table (same
 * NULL-terminated `u16 *` table as SetCommand.c: entry[0] = command id,
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
 *  - Inner match loop is `do { if (cmd[j] != pad->stream[j]) break; j++; }
 *    while (cmd[j] != 0xFFFF);` — the post-loop `if (cmd[j] != 0xFFFF)
 *    continue;` reads the SAME `j` whether the loop exited via `break`
 *    (mismatch, j unchanged) or via the bottom test (full match, j already
 *    incremented past the last matched element) — both paths' "increment
 *    j" delay slot ALWAYS runs (MIPS branch-delay semantics), so a single
 *    post-loop re-read of `cmd[j]` is correct for both exits.
 *  - The shift loop is Ghidra's literal do-while (`j = 3; do {
 *    pad->stream[j] = pad->stream[j-1]; j--; } while (j > 0);`), not a
 *    `for` — matches the single post-decrement bottom test with no
 *    redundant front check.
 *  - The final return re-derives `Command[i]` fresh (`*(short *)Command[i]`)
 *    rather than reusing an `entry` variable — the asm reloads it by index
 *    at the return, not from a cached pointer.
 *
 * THE RESIDUAL: `i` (the outer loop counter, PSX.SYM's own reg $a2) lands
 * in `$a3` in our build and `$pad` lands in `$t1` instead of target's
 * `$t0` — a whole-function register-renumbering shift, not a structural
 * gap (matchdiff confirms the SAME 73-instruction length both sides).
 * The other real difference: right after computing `cmd` (entry+1), the
 * target materializes a SEPARATE `li $a3,0xffff` used only by the inner
 * loop's own bottom test (a DIFFERENT register from the `$t1` used for the
 * entry `cmd[0]==0xFFFF` guard and the final post-loop check) — our build
 * reuses one shared register for all three 0xFFFF comparisons instead.
 * Tried: a named `u16 term = 0xFFFF;` assigned just inside the `if
 * (cmd[0] != 0xFFFF)` guard and read only in the loop's `while` — this
 * DID force a second register, but regressed badly (59 -> 192 bytes,
 * whole different instruction selection cascade) — the original's split
 * is not simply "the same value written twice in source" the way the
 * cookbook's shared-constant rule usually captures; something about
 * *which* register the split constant defaults to (not necessarily $a3)
 * or *when* it's materialized relative to the loop's own back-edge
 * differs from what a plain local variable produces. A `do{}while(0)`
 * wrapper around the inner match loop (the usual regalloc lever) had
 * zero effect. Not resolved this session; the register split looks like
 * it needs a lever this cookbook doesn't have a rule for yet.
 */

extern u16 *Command[];

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/GetCommand", GetCommand);
#else
short GetCommand(PADtype *pad)
{
    u16 *cmd;
    short i;
    short j;

    for (i = 0; Command[i] != 0; i++)
    {
        cmd = Command[i] + 1;
        j = 0;
        if (cmd[0] != 0xFFFF)
        {
            do
            {
                if (cmd[j] != pad->stream[j])
                    break;
                j++;
            } while (cmd[j] != 0xFFFF);

            if (cmd[j] != 0xFFFF)
                continue;
        }

        j = 3;
        do
        {
            pad->stream[j] = pad->stream[j - 1];
            j--;
        } while (j > 0);
        pad->stream[0] = 0;
        return *(short *)Command[i];
    }
    return 0;
}
#endif
