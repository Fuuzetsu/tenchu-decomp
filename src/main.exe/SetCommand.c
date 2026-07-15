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
 * SetCommand (0x8001b038, 0x10c bytes) — looks up `cmd` in the global
 * command table Command (a NULL-terminated array of `u16 *` entries:
 * entry[0] = command id, entry[1..] = a 0xFFFF-terminated argument list),
 * copies the entry's remaining arguments into pad->stream[], sets
 * pad->time = 1, and returns the entry's first argument (sign-extended).
 * Returns 0 if the table is empty or `cmd` isn't found. PADtype/.time/
 * .stream are the proven item.h struct (GetRealPad.c/PadShock.c).
 *
 * Three loops, all standard shapes (docs/matching-cookbook.md):
 *  - outer search: `i = 0; while (Command[i] != 0) { entry = Command[i];
 *    found = (entry[0] == cmd); one = 1; if (found) {...match...} i++; }`
 *    — the classic duplicate_loop_exit_test bottom-test do-while (provable
 *    i=0 entry, same shape as this session's FUN_800566c0/FUN_800565f0).
 *    Two non-obvious things about this loop's exact spelling, both found
 *    by reading cc1's `-dS`/`-dg` RTL dumps (tools/rtldump.py) after the
 *    plain Ghidra-shaped draft (i++ as the SECOND statement, `if
 *    (entry[0]==cmd)` directly) landed at the right length but 46/268
 *    bytes wrong, in two clusters:
 *      1. `i++` must be the LAST statement of the loop body (after the
 *         whole if), not the second one. A while/for loop's own
 *         increment sits right before the bottom-of-loop re-test in the
 *         unscheduled RTL, and reorg's delay-slot filler then hoists that
 *         independent instruction BACKWARD into the entry-comparison
 *         branch's delay slot — producing the target's `addiu v0,a0,1`
 *         into a fresh register (v0) while `a0` (old `i`) stays live for
 *         the non-taken path, only becoming the new `i` via a `move` in
 *         the bottom-test's own delay slot. Writing `i++` textually
 *         BEFORE the if (as a literal self-increment reachable on both
 *         match and no-match paths) instead compiles as a plain in-place
 *         `addiu a0,a0,1` — cc1 has no reason to keep the pre- and
 *         post-increment values in different registers when the
 *         increment's own position doesn't put it downstream of a
 *         branch with unrelated code after it.
 *      2. `one`'s hoisted-out-of-the-loop materialization (see below)
 *         must be scheduled AFTER the `cmd`-widening pair (`sll/sra`),
 *         not before. Both are loop-invariant plain assignments with no
 *         cross-dependency, so cc1's list scheduler (sched.c, a
 *         single-issue in-order scheduler on this target) breaks the tie
 *         by each instruction's RTL creation order (UID) — which follows
 *         textual/scan order at expand time. `one = 1;` written as a
 *         bare statement before the `if` gets its RTL insn created
 *         BEFORE the if-condition's widen (whether placed right after
 *         `entry = Command[i]` or right after `i++`; either position is
 *         scanned before the `if`). The fix: force the widen's own
 *         creation to happen first by hoisting the comparison itself
 *         into an unconditional boolean temp — `found = (entry[0] ==
 *         cmd);` computed BEFORE `one = 1;`, then `if (found)` merely
 *         branches on the already-widened result. This is a source-level
 *         reordering trick with no semantic effect (found is used
 *         nowhere else), purely to control which RTL insn gets the
 *         lower UID — a lever worth generalizing: **when two
 *         loop-invariant assignments must land in a specific relative
 *         order and neither depends on the other, capture whichever one
 *         needs to sort LATER as an explicit boolean/temp BEFORE the
 *         other statement, forcing its RTL creation order earlier.**
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
 * `one`: a loop-invariant constant, written as a bare `one = 1;` statement
 * unconditionally in the loop body (NOT inside the `if`: cc1 only hoists an
 * invariant assignment out of the loop when it is reached on every
 * iteration — placing it inside the returning `if` instead leaves it
 * un-hoisted, right there, and shuffles an unrelated callee register
 * assignment for `pad`). Hoisted, it lands once, right after the first
 * (duplicated) entry test, and is read again both by the "if (one < n)"
 * guard and the final `pad->time = one` store — too far apart (crossing two
 * more loops) to be an incidental CSE of a bare literal; matches the
 * cookbook's shared-constant-variable rule (a value stored to two distant
 * sites is one C local, not two independent `1`s). A bare literal
 * `if (1 < n)` instead canonicalizes to the cheap `slti n,2` form
 * (fold-const rewrites "1 < n" to "n >= 2"), which the target does NOT use
 * (it spends an extra `li`+`slt`) — so the variable is required, not
 * optional. Declaring `one` as `s32` (not `s16`) is required too: an
 * `s16 one` gets a SEPARATE re-widened SImode copy materialized for the
 * comparison (a second, different hard register), splitting one logical
 * value across two `li`s; `s32` needs no such widening and both uses share
 * the same register, like the target's single `$t1`.
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
 */
extern u16 *Command[];

short SetCommand(PADtype *pad, short cmd)
{
    s16 i;
    u16 *entry;
    u16 *args;
    s16 n;
    s16 j;
    s32 one;
    s16 found;

    i = 0;
    while (Command[i] != 0) {
        entry = Command[i];
        found = (entry[0] == cmd);
        one = 1;
        if (found) {
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
        i++;
    }
    return 0;
}
