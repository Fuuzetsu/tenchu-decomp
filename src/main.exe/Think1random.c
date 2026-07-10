#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1random(void);
 *     THINK_1.C:74, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $a1       long xx
 *     reg   $v1       long zz
 *     reg   $s1       short pad
 *     reg   $a1       long vx
 *     reg   $v1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Attrib;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 14 of 372 bytes differ (right length; every
 * differing byte is a register-only tie, no wrong/extra/missing
 * instruction).
 *
 * Think1random (0x8002c0c4, 0x174 bytes) — think-handler, same "think" TU as
 * Think1sleep.c/Think1chase.c/Think1trace.c. On the first tick of a new
 * "random walk" cycle (actcnt just rolled over to 1), picks a fresh chase
 * target near the character's spawn point (some_x/z_position +/- up to
 * 5000, via rand()%10000-5000 on each axis). On later ticks, once within
 * 1000 units of the chase target on BOTH axes, resets actcnt to 0 (so the
 * next tick re-rolls); otherwise steers towards it via
 * turn_towards_player_, unless Attrib bit 0x400 is set (in which case it
 * also just resets — the same "reached/blocked" reset as the close case).
 *
 * `FUN_8002b990` (Ghidra's/m2c's placeholder name for this call site) is
 * `turn_towards_player_` at the SAME address (0x8002b990, see
 * config/symbols.main.exe.txt) — not a separate unmatched function.
 * Ghidra's own rendering shows the call with NO arguments
 * (`sVar1 = FUN_8002b990();`), but the asm passes the just-computed
 * (chase - locate) diffs in $a0/$a1 (the abs-value clamp that decides
 * whether to reach the call negates a COPY in $v0, leaving the raw diffs
 * in $a0/$a1 untouched) — the m2c/Ghidra call-arg-undercount tell from the
 * cookbook, confirmed against the raw `.s` (matches Think4contact/
 * Think4chase's explicit `FUN_8002b990(iVar5,iVar4)` sibling call).
 *
 * `actcnt` is re-narrowed to u8 with a defensive `andi 0xff` right after the
 * `+1`/store-back, even though the store already truncates — the
 * u8-local-forces-a-mask rule (the compare needs the masked value, the
 * store doesn't).
 *
 * `dx`/`dz` (the chase-locate diffs) and `adx`/`adz` (their abs values) are
 * separate locals: the asm computes abs into a fresh register ($v0) while
 * leaving the raw diffs live in $a0/$a1 for the later turn_towards_player_
 * call — same "abs_degree stays separate from degree" idiom as
 * Think1trace.c.
 *
 * RESIDUAL (14 bytes, all register-only, no wrong instruction anywhere):
 *  - The `rand() % 10000` quotient's `mfhi` lands in $a3 in the target,
 *    $a2 here, twice (identical both times). The IDENTICAL source snippet
 *    (`Me_THINK_C->some_other_x_position = Me_THINK_C->some_x_position +
 *    rand() % 10000 - 5000;` ×2) compiles bit-for-bit correctly with $a3 in
 *    Think1chase.c's own copy of this exact fallback — proving the tie is a
 *    whole-function global-allocation artifact (pseudo-number tie-break),
 *    not something wrong with this snippet itself.
 *  - The `else` branch's three loads (`some_kind_of_current_position`,
 *    `some_other_x_position`, `some_other_z_position`) land in
 *    {v1,a2,a1}/{a0,v0,v1} depending on whether `locate` is a named local
 *    or inlined — tried both, byte-identical to each other, both a
 *    register-only tie vs. target (confirmed: introducing a named `VECTOR
 *    *locate` changes nothing).
 *  - The `adx`/`adz` negation reads the fresh copy's OWN register in target
 *    (`negu v0,v0`, matching the "conditional negate re-reads its own
 *    destination" rule that already works in Think1trace.c) but re-reads
 *    the ORIGINAL value's register here (`negu v0,a0`) — same shape as the
 *    rule, wrong register only, in a function-context where it doesn't
 *    trigger.
 * Tried and rejected (all byte-identical to what's here, or worse): a named
 * `locate` local; a `do{}while(0)` wrapper around the `if(actcnt==1)` body
 * (priority-boost lever). `tools/permute.py Think1random -- --stop-on-zero
 * -j4`, two bounded runs (~400s, then ~180s that hit an internal permuter
 * crash on this function's `rand()` call — `KeyError: 'rand'` in the
 * permuter's own type-inference, not a bug in this source) — best score
 * found (45) never beat the baseline (12) in >3000 combined iterations.
 * Parked per the cookbook's attempt-cap/sub-C-level-residual guidance.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/Think1random", Think1random);
#else
extern s16 Attrib;

s16 Think1random(void)
{
    u8 actcnt;
    s32 result;

    actcnt = Me_THINK_C->actcnt + 1;
    Me_THINK_C->actcnt = actcnt;
    result = 0;
    if (actcnt == 1)
    {
        Me_THINK_C->some_other_x_position = Me_THINK_C->some_x_position + rand() % 10000 - 5000;
        Me_THINK_C->some_other_z_position = Me_THINK_C->some_z_position + rand() % 10000 - 5000;
    }
    else
    {
        s32 dx, dz;
        s32 adx, adz;

        dx = Me_THINK_C->some_other_x_position;
        dz = Me_THINK_C->some_other_z_position;
        dx = dx - Me_THINK_C->some_kind_of_current_position->vx;
        dz = dz - Me_THINK_C->some_kind_of_current_position->vz;
        adx = dx;
        if (adx < 0)
        {
            adx = -adx;
        }
        if (adx < 1000)
        {
            adz = dz;
            if (adz < 0)
            {
                adz = -adz;
            }
            if (adz < 1000)
            {
                goto reset;
            }
        }
        if (Attrib & 0x400)
        {
        reset:
            Me_THINK_C->actcnt = 0;
        }
        else
        {
            result = turn_towards_player_(dx, dz);
        }
    }
    return result;
}
#endif
