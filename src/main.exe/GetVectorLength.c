#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetVectorLength(long dx, long dy, long dz);
 *     EFFECT.C:493, 12 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
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
 *  - `abs()` MUST be declared taking/returning `long`, not `int`/`s32`.
 *    cc1 recognizes `int abs(int)` as a builtin and inlines it to
 *    branch+negate (no `jal`) even though Build.hs's `-fno-builtin` never
 *    reaches the cc1 step (it's only in `cppFlags`, fed to the separate
 *    `cpp` invocation — verified with a standalone cc1-281 run: plain
 *    `int abs(int)` inlines, `-fno-builtin int abs(int)` and plain
 *    `long abs(long)` both emit a real `jal abs` with byte-identical
 *    `.frame`/`.mask`. The target's 3 `jal 0x80076074` calls (confirmed by
 *    `tools/xref.py`) need the latter spelling — no Build.hs/symbols.txt
 *    change needed once the prototype is right.
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
extern s32 SquareRoot0(s32 x);
extern long abs(long x);

long GetVectorLength(long dx, long dy, long dz)
{
    long len;
    int big;
    long v;

    big = 0;
    if (abs(dx) > 0x1000 || abs(dy) > 0x1000 || abs(dz) > 0x1000)
    {
        big = 1;
    }
    if (big)
    {
        v = dx;
        if (dx < 0) v = dx + 0xff;
        dx = v >> 8;
        v = dy;
        if (dy < 0) v = dy + 0xff;
        dy = v >> 8;
        v = dz;
        if (dz < 0) v = dz + 0xff;
        dz = v >> 8;
        len = SquareRoot0(dx * dx + dy * dy + dz * dz);
        len = len << 8;
    }
    else
    {
        len = SquareRoot0(dx * dx + dy * dy + dz * dz);
    }
    return len;
}
