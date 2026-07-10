#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int IsVisible(long x, long y, long z, long s);
 *     WORLD.C:685, 63 src lines, frame 32 bytes, saved-reg mask 0x80030000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $a0       long x
 *     param $a1       long y
 *     param $a2       long z
 *     param $a3       long s
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 9 of 520 bytes differ; RIGHT LENGTH (130
 * instructions both sides, confirmed by asmdiff --structural: zero
 * inserts/deletes, only register-name replacements in one block). The
 * residual is a 3-way rotation of which callee-saved register ($s0/$s1/$s2)
 * ends up holding `q2` (the third eager division's quotient), `iVar4` (the
 * second), and `fail` — a pure global-alloc tie: `tools/regalloc.py` shows
 * no copy-chain pinning any of the three to a specific hard register.
 * `tools/permute.py` ran ~42000 iterations (--stop-on-zero, -j4, ~8 min
 * bounded) without finding a byte-exact candidate; its best-scoring
 * candidates were either identical modulo unreachable dead code (a
 * `do{}while(0)` injected after a `return`) or otherwise did not reduce the
 * real `matchdiff` byte count — re-verified per the cookbook's warning that
 * the permuter's own score isn't raw bytes. Parked per the sub-C-level
 * early-stop.
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
 *    (an `xori`), and importantly the FIRST failing branch does NOT return
 *    early; both branches converge on ONE shared `return !fail;` at the
 *    bottom. Returning immediately after `fail = 1;` on the first branch
 *    lets cc1 constant-fold `!1` to a literal 0 there, which is 3
 *    instructions shorter than the target (it doesn't fold, because the
 *    real source funnels both branches through one common return).
 *  - The three divisions need ASPSX's guarded div expansion — this file is
 *    in Build.hs's `maspsxGpExterns` `extra`/`--expand-div` list (and
 *    permute.py's MASPSX_EXTRA); no explicit `trap()` calls belong in the C,
 *    maspsx inserts the break-7/break-6 guards around a plain `/`
 *    automatically.
 */

extern s32 abs(s32 x);
extern void ApplyRotMatrix(SVECTOR *v, VECTOR *out);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/IsVisible", IsVisible);
#else

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

    view = (s32 *)0x1F800038;

    dx = x - view[0];
    if (30000 < abs(dx))
        return 0;

    dy = y - view[1];
    if (!(abs(dy) < 0x7531))
        return 0;

    dz = z - view[2];
    if (!(abs(dz) < 0x7531))
        return 0;

    ((SVECTOR *)0x1F800010)->vx = (s16)dx;
    ((SVECTOR *)0x1F800010)->vy = (s16)dy;
    ((SVECTOR *)0x1F800010)->vz = (s16)dz;
    ApplyRotMatrix((SVECTOR *)0x1F800010, (VECTOR *)0x1F800000);

    scratch = (s32 *)0x1F800000;
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
    {
        fail = 1;
    }
    else
    {
        iVar1 = abs(q2);
        if (iVar4 + 0x78 < iVar1)
            fail = 1;
    }
    return !fail;
}
#endif
