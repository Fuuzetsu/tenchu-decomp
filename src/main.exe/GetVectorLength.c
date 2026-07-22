#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetVectorLength(long dx, long dy, long dz);
 *     EFFECT.C:493, 12 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     param $a0       long dx
 *     param $a1       long dy
 *     param $a2       long dz
 * END PSX.SYM */

/*
 * GetVectorLength (0x80039944, 0x120 bytes) — magnitude of (dx,dy,dz),
 * clamping-then-scaling by 256 before the sqrt when any component would
 * overflow SquareRoot0's fixed-point input (component magnitude > 0x1000).
 *
 * Matching notes:
 *  - PSX.SYM's frame (24 bytes, mask 0x80000000 — $ra only) does NOT match
 *    this build: the real function needs a 40-byte frame saving s0-s3+ra.
 *    The demo build's symbol table is wrong here, not just differently
 *    allocated (docs/matching-cookbook.md's "~2 of 5" PSX.SYM caveat).
 *  - `abs()` is declared taking/returning `long`, matching the recovered
 *    original prototype. Build.hs now passes `-fno-builtin` to cc1 (not just
 *    cpp), so an ordinary `abs` remains a real call for either common integer
 *    prototype. The target's 3 `jal 0x80076074` calls (confirmed by
 *    `tools/xref.py`) therefore stay physical. Functions whose targets inline
 *    `bgez/move/negu` use the explicit `__builtin_abs` spelling instead.
 *  - Ghidra's `bVar1 = false; if (OR-chain) bVar1 = true; if (bVar1) ...`
 *    is the LITERAL source shape here, not an SSA artifact: writing the
 *    equivalent direct `if (OR-chain) {BIG} else {SMALL}` compiles to
 *    short-circuit jumps straight to each body and is 8 bytes short (one
 *    fewer callee-saved register, no shared flag). The named `big` flag
 *    surviving across all three `abs()` calls is what puts it in $s3.
 *  - Each `if (v < 0) v = v + 0xff;` clamp is a default-then-override temp,
 *    not an in-place reassignment: `t = v; if (v < 0) t = v + 0xff; v = t
 *    >> 8;`. The target's delay-slot-filled `bgez` puts the *default* copy
 *    (`move v0,sN`) in the branch's delay slot (runs unconditionally) and
 *    only the fallthrough (v<0) path overwrites it — reassigning `v`
 *    in place instead produces a shorter, wrong-shaped `addiu` with no
 *    delay-slot move.
 */
extern long abs(long x);

long GetVectorLength(long dx, long dy, long dz)
{
    enum { patch = 4096 };
    enum { div = 256 };
    long len;
    int big;
    long v;

    big = 0;
    if (abs(dx) > patch || abs(dy) > patch || abs(dz) > patch)
    {
        big = 1;
    }
    if (big)
    {
        v = dx;
        if (dx < 0) v = dx + div - 1;
        dx = v >> 8;
        v = dy;
        if (dy < 0) v = dy + div - 1;
        dy = v >> 8;
        v = dz;
        if (dz < 0) v = dz + div - 1;
        dz = v >> 8;
        len = SquareRoot0(dx * dx + dy * dy + dz * dz);
        len = len * div;
    }
    else
    {
        len = SquareRoot0(dx * dx + dy * dy + dz * dz);
    }
    return len;
}
