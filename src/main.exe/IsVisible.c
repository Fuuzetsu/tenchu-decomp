#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int IsVisible(long x, long y, long z, long s);
 *     WORLD.C:685, 63 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *     param $a0       long x
 *     param $a1       long y
 *     param $a2       long z
 *     param $a3       long s
 * END PSX.SYM */

/*
 * STATUS: MATCHING. The last 9-byte residual was a three-way saved-register
 * rotation. Two separate `fail = 1` source assignments gave `fail` 4 refs / 22
 * live insns (priority 3636), so it outranked `q2` (3333) and `iVar4` (1764).
 * Both failing tests now jump to one `failed:` assignment, reducing `fail` to
 * 3 refs / 20 live insns (1500) and producing the target allocation
 * `q2=$s0`, `iVar4=$s1`, `fail=$s2`. The separate `done:` join is essential:
 * the success path jumps to it while the failure assignment falls through,
 * so cc1 retains the runtime `!fail` expression instead of folding the
 * failure path to literal zero. This exact pure-C body is 520/520 bytes.
 *
 * IsVisible (0x8003b604, 0x208 bytes) — same TU as GetCenterAndSize.c/
 * leFindEnemy.c (WORLD.C): a cheap frustum-ish visibility test for a
 * construction billboard/effect at world (x,y,z): first a fast axis-aligned
 * box reject against the GTE's cached view position (raw PSX scratchpad RAM
 * @0x1F800000, still holding the last ApplyRotMatrix/perspective-transform
 * inputs from whatever earlier GTE call populated it), then a real
 * perspective test (rotate the delta into view space via ApplyRotMatrix,
 * then compare the projected x/y against a screen-space rectangle scaled by
 * `s`, the object's on-screen half-size).
 *
 * Matching notes:
 *  - `view` (the `0x1F800038`-based scratchpad read) is a genuine CACHED
 *    POINTER kept live in a callee-saved register ACROSS the three `abs()`
 *    calls — matching the "cached pointer across calls" rule — not three
 *    independent raw-literal dereferences. `scratch` (`0x1F800000`, used
 *    after ApplyRotMatrix) gets the SAME treatment for its own 3 reads, but
 *    is a SEPARATE raw literal — cc1 never merges the two hardcoded
 *    constants even though both sit in PSX scratchpad RAM.
 *  - All THREE divisions by `iVar1` (a runtime value, not a constant) are
 *    computed EAGERLY, back-to-back, into named temporaries (`q0`, `iVar4`,
 *    `q2`) — BEFORE either `abs()` call that consumes them. Ghidra's own
 *    rendering shows the third division folded inline into the second
 *    `abs()` call's argument (`abs(iVar3/iVar1)`), which is an SSA/statement-
 *    order artifact: the raw asm computes all three quotients up front
 *    (sharing the one divisor register across all three `div`s before a
 *    call could clobber it), then does the two `abs()` calls.
 *  - The three range guards are all flat guard-clause early returns
 *    (`if (!(cond)) return 0;`) — nesting the second/third (Ghidra's own
 *    literal `if (a) { if (b) {...} }` rendering) makes zero byte difference
 *    here, so the flat form is kept for readability.
 *  - The final success/failure test is a `fail` flag (0=ok), NOT a plain
 *    `if (cond) return 1; return 0;` — cc1 compiles the return as `!fail`
 *    (an `xori`). Both failing tests jump to ONE shared `fail = 1` block,
 *    while success jumps past that assignment to the separate `done:` join.
 *    Returning directly from `failed:` lets cc1 constant-fold `!1` to a
 *    literal 0 and is 3 instructions shorter; duplicating `fail = 1` in the
 *    two source arms raises its allocation priority and rotates `$s0-$s2`.
 *  - The three divisions need ASPSX's guarded div expansion — this file is
 *    in Build.hs's `maspsxGpExterns` `extra`/`--expand-div` list (and
 *    permute.py's MASPSX_EXTRA); no explicit `trap()` calls belong in the C,
 *    maspsx inserts the break-7/break-6 guards around a plain `/`
 *    automatically.
 */

extern s32 abs(s32 x);

int IsVisible(s32 x, s32 y, s32 z, s32 s)
{
    s32 *view;
    s32 *scratch;
    s32 dx, dy, dz;
    s32 iVar1;
    s32 iVar2;
    s32 iVar4;
    s32 q0, q2;
    s32 fail;

    view = (s32 *)TENCHU_SCRATCHPAD(0x38);

    dx = x - view[0];
    if (30000 < abs(dx))
        return 0;

    dy = y - view[1];
    if (!(abs(dy) < 0x7531))
        return 0;

    dz = z - view[2];
    if (!(abs(dz) < 0x7531))
        return 0;

    ((SVECTOR *)TENCHU_SCRATCHPAD(0x10))->vx = (s16)dx;
    ((SVECTOR *)TENCHU_SCRATCHPAD(0x10))->vy = (s16)dy;
    ((SVECTOR *)TENCHU_SCRATCHPAD(0x10))->vz = (s16)dz;
    ApplyRotMatrix((SVECTOR *)TENCHU_SCRATCHPAD(0x10),
                   (VECTOR *)TENCHU_SCRATCHPAD_ADDRESS);

    scratch = (s32 *)TENCHU_SCRATCHPAD_ADDRESS;
    iVar1 = scratch[2] + s;
    if (iVar1 < 0x97)
        return 0;
    if (17000 < scratch[2] - s)
        return 0;

    q0 = (scratch[0] * 300) / iVar1;
    iVar4 = (s * 300) / iVar1;
    q2 = (scratch[1] * 300) / iVar1;
    fail = 0;
    iVar2 = abs(q0);
    if (iVar4 + 0xA0 < iVar2)
        goto failed;

    iVar1 = abs(q2);
    if (iVar4 + 0x78 < iVar1)
        goto failed;
    goto done;

failed:
    fail = 1;
done:
    return !fail;
}
