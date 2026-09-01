#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int IsVisible(long x, long y, long z, long s);
 *     WORLD.C:685, 63 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long x
 *     param $a1       long y
 *     param $a2       long z
 *     param $a3       long s
 * END PSX.SYM */

/*
 * STATUS: MATCHING. The last 9-byte residual was a three-way saved-register
 * rotation. Two separate `fail = 1` source assignments gave `fail` 4 refs / 22
 * live insns (priority 3636), so it outranked `q2` (3333) and `qs` (1764).
 * Both failing tests now jump to one `failed:` assignment, reducing `fail` to
 * 3 refs / 20 live insns (1500) and producing the target allocation
 * `q2=$s0`, `qs=$s1`, `fail=$s2`. The separate `done:` join is essential:
 * the success path jumps to it while the failure assignment falls through,
 * so cc1 retains the runtime `!fail` expression instead of folding the
 * failure path to literal zero. This exact pure-C body is 520/520 bytes.
 *
 * IsVisible (0x8003b604, 0x208 bytes) — same TU as GetCenterAndSize.c/
 * leFindEnemy.c (WORLD.C): a cheap frustum-ish visibility test for a
 * construction billboard/effect at world (x,y,z): first a fast axis-aligned
 * box reject against the camera state DrawConstruction cached in the shared
 * ConstructionVisibilityWorkspace, then a real
 * perspective test (rotate the delta into view space via ApplyRotMatrix,
 * then compare the projected x/y against a screen-space rectangle scaled by
 * `s`, the object's on-screen half-size).
 *
 * Matching notes:
 *  - `view` (the workspace's cached-view member) is a genuine CACHED
 *    POINTER kept live in a callee-saved register ACROSS the three `abs()`
 *    calls — matching the "cached pointer across calls" rule — not three
 *    independent member dereferences. `view_space` (the result used
 *    after ApplyRotMatrix) gets the SAME treatment for its own three reads,
 *    but its member address is materialized separately.
 *  - All THREE divisions by `z` (a runtime value, not a constant) are
 *    computed EAGERLY, back-to-back, into named temporaries (`q0`, `qs`,
 *    `q2`) — BEFORE either `abs()` call that consumes them. Ghidra's own
 *    rendering shows the third division folded inline into the second
 *    `abs()` call's argument (`abs(iVar3/z)`), which is an SSA/statement-
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
    enum
    {
        SXW = 160,
        SYW = 120
    };
    enum
    {
        NEAR = 150
    };
    GsRVIEW2 *view;
    VECTOR *view_space;
    s32 dx, dy, dz;
    s32 zs;
    s32 aq;
    s32 qs;
    s32 q0, q2;
    s32 fail;

    view = CONSTRUCTION_VISIBILITY_VIEW;

    dx = x - view->vpx;
    if (30000 < abs(dx))
        return 0;

    dy = y - view->vpy;
    if (30000 < abs(dy))
        return 0;

    dz = z - view->vpz;
    if (30000 < abs(dz))
        return 0;

    CONSTRUCTION_VISIBILITY_RELATIVE->vx = (s16)dx;
    CONSTRUCTION_VISIBILITY_RELATIVE->vy = (s16)dy;
    CONSTRUCTION_VISIBILITY_RELATIVE->vz = (s16)dz;
    ApplyRotMatrix(CONSTRUCTION_VISIBILITY_RELATIVE,
                   CONSTRUCTION_VISIBILITY_VIEW_SPACE);

    view_space = CONSTRUCTION_VISIBILITY_VIEW_SPACE;
    zs = view_space->vz + s;
    if (zs <= NEAR)
        return 0;
    if (17000 < view_space->vz - s)
        return 0;

    q0 = (view_space->vx * PROJECTION_DISTANCE) / zs;
    qs = (s * PROJECTION_DISTANCE) / zs;
    q2 = (view_space->vy * PROJECTION_DISTANCE) / zs;
    fail = 0;
    aq = abs(q0);
    if (qs + SXW < aq)
        goto failed;

    zs = abs(q2);
    if (qs + SYW < zs)
        goto failed;
    goto done;

failed:
    fail = 1;
done:
    return !fail;
}
