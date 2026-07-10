#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SetCommand(struct PADtype *pad, short cmd);
 *     PADCMD.C:359, 17 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 * STATUS: NON_MATCHING — 46 of 268 bytes differ.
 *
 * SetCommand (0x8001b038, 0x10c bytes) — looks up `cmd` in the global
 * command table Command (a NULL-terminated array of `u16 *` entries:
 * entry[0] = command id, entry[1..] = a 0xFFFF-terminated argument list),
 * copies the entry's remaining arguments into pad->stream[], sets
 * pad->time = 1, and returns the entry's first argument (sign-extended).
 * Returns 0 if the table is empty or `cmd` isn't found. PADtype/.time/
 * .stream are the proven item.h struct (GetRealPad.c/PadShock.c).
 *
 * Three loops, all standard shapes (docs/matching-cookbook.md):
 *  - outer search: `i = 0; while (Command[i] != 0) { entry =
 *    Command[i]; i++; if (entry[0] == cmd) {...match...} }` — the
 *    classic duplicate_loop_exit_test bottom-test do-while (provable i=0
 *    entry, same shape as this session's FUN_800566c0/FUN_800565f0).
 *  - arg count: `n = 0; while (args[n] != 0xFFFF) n++;` — same shape,
 *    entry test folds to args[0] since n=0 is provable.
 *  - copy: `j = 1; do { pad->stream[j-1] = args[j]; j++; } while (j < n);`
 *    guarded by an explicit `if (one < n)` (see below) — NOT a plain for
 *    loop: a real `for`/`while` here would ALSO get jump.c's automatic
 *    front-test duplication (j=1 substituted into j<n) stacked on top of
 *    the explicit guard, producing two redundant tests in a row (verified:
 *    a `for` version compiles a second `li v0,1; slt; beqz` immediately
 *    after the first). A hand-written `if (guard) { do {...} while(); }`
 *    has no such implicit front test, matching the target's single test.
 *
 * `one`: the constant 1 materializes ONCE at the very top of the function
 * (before the search loop even starts) and is read again both by the
 * "if (one < n)" guard and the final `pad->time = one` store — too far
 * apart (crossing two more loops) to be an incidental CSE of a bare
 * literal; matches the cookbook's shared-constant-variable rule (a value
 * stored to two distant sites is one C local, not two independent `1`s).
 * A bare literal `if (1 < n)` instead canonicalizes to the cheap
 * `slti n,2` form (fold-const rewrites "1 < n" to "n >= 2"), which the
 * target does NOT use (it spends an extra `li`+`slt`) — so the variable is
 * required, not optional. Declaring `one` as `s32` (not `s16`) is required
 * too: an `s16 one` gets a SEPARATE re-widened SImode copy materialized
 * for the comparison (a second, different hard register), splitting one
 * logical value across two `li`s; `s32` needs no such widening and both
 * uses share the same register, like the target's single `$t1`.
 *
 * The `do { ... } while (0);` wrapping the whole matched-entry body is a
 * regalloc lever (cookbook's "do{}while(0) is a regalloc lever" rule):
 * without it, the outer loop's own index (`i`) and the command table's
 * base address end up one register off from the target ($a2/$a3 instead
 * of $a0/$a2) — the wrapper's extra loop_depth weighting on everything
 * inside re-derives the target's exact $a0/$a1/$a2/$t1 assignment
 * (found via one bounded decomp-permuter run; the wrapper was in its best
 * surviving candidate, isolated and confirmed by hand — see
 * tools/permute.py SetCommand).
 *
 * Residual (46 bytes, two clusters, both explored without success):
 *  1. Entry-block instruction scheduling: the target fills the first
 *     branch's delay slot with `i = 0`'s materialization and computes
 *     `one = 1` afterward (right before the loop); our build schedules
 *     `i = 0` at the very front and `one = 1` into the delay slot instead.
 *     Swapping the `i = 0;`/`one = 1;` source statement order has NO
 *     effect on this (tried both ways, byte-identical output) — the
 *     scheduler's tie-break here isn't source-order-driven in any lever
 *     found so far.
 *  2. The outer loop's `i++`: the target computes the incremented value
 *     into a FRESH register ($v0, `addiu v0,a0,1`) while `$a0` (old `i`)
 *     stays untouched, only becoming the new `i` via a `move` on the
 *     not-found continuation path (matches m2c's raw reading: a separate
 *     `temp_v0 = var_a0 + 1;` whose assignment back to `var_a0` is
 *     textually only reachable via the non-match path). Our `i++;`
 *     compiles as a plain self-increment (`addiu a0,a0,1`). Splitting it
 *     into an explicit `next = i + 1;` + deferred `i = next;` (assigned
 *     after the whole if-block, on the implicit non-match fallthrough) —
 *     tried both as a plain `while` and as an explicit `if + do-while`
 *     matching Ghidra's literal shape — regresses BADLY (46 -> 213 bytes,
 *     unrelated registers reshuffle throughout the function), so something
 *     about that split defeats the do{}while(0) lever above. Not yet
 *     root-caused; flagged for a future session rather than more
 *     surgical attempts here (cookbook's sub-C-level attempt cap).
 *  Two bounded decomp-permuter runs (~5 min and ~3 min, --stop-on-zero
 *  -j4, tens of thousands of iterations combined) plateaued at score 505
 *  then 305 (never 0) against these two clusters — logged as permuter-
 *  resistant register/scheduling ties, not a source-shape gap.
 */
extern u16 *Command[];

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetCommand", SetCommand);
#else
short SetCommand(PADtype *pad, short cmd)
{
    s16 i;
    u16 *entry;
    u16 *args;
    s16 n;
    s16 j;
    s32 one;

    one = 1;
    i = 0;
    while (Command[i] != 0) {
        entry = Command[i];
        i++;
        if (entry[0] == cmd) {
            args = entry + 1;
            do {
                n = 0;
                while (args[n] != 0xFFFF) {
                    n++;
                }
                if (one < n) {
                    j = 1;
                    do {
                        pad->stream[j - 1] = args[j];
                        j++;
                    } while (j < n);
                }
                pad->time = one;
                return (s16)args[0];
            } while (0);
        }
    }
    return 0;
}
#endif
