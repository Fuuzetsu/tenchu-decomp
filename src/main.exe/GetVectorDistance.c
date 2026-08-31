#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int GetVectorDistance(struct VECTOR *v1, struct VECTOR *v2);
 *     EFFECT.C:509, 19 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * v1
 *     param $a1       struct VECTOR * v2
 * END PSX.SYM */

/*
 * GetVectorDistance (0x80039808, 0x13c bytes) — same clamp-then-scale
 * magnitude idiom as the sibling GetVectorLength.c (this TU), but takes the
 * delta between two VECTOR points instead of three raw components. See
 * GetVectorLength.c's header for the three idioms this shares:
 *  - `abs()` must be declared `long abs(long)`, not `int abs(int)` — the
 *    latter is cc1's recognized builtin and inlines to branch+negate (no
 *    `jal`) even though this build's `-fno-builtin` never reaches cc1
 *    (Build.hs only feeds it to the separate `cpp` step). The target's 3
 *    `jal 0x80076074` calls (tools/xref.py) need the non-builtin spelling.
 *  - Ghidra's `bVar1 = false; if (OR-chain) bVar1 = true; if (bVar1) ...`
 *    is literal source shape (not SSA noise): the flag surviving all three
 *    abs() calls is what puts it in a callee-saved register.
 *  - Each `if (v < 0) v = v + 0xff;` clamp is a default-then-override temp
 *    (`t = v; if (v < 0) t = v + 0xff; v = t >> 8;`), matching the target's
 *    delay-slot-filled `bgez` (the unconditional copy sits in the branch's
 *    delay slot); reassigning `v` in place compiles 8 bytes short.
 */
extern long abs(long x);

int GetVectorDistance(VECTOR *v1, VECTOR *v2)
{
    enum
    {
        patch = 4096
    };
    enum
    {
        div = 256
    };
    long dx, dy, dz;
    long len;
    int big;
    long v;

    dx = v1->vx - v2->vx;
    dy = v1->vy - v2->vy;
    dz = v1->vz - v2->vz;

    big = 0;
    if (abs(dx) > patch || abs(dy) > patch || abs(dz) > patch)
    {
        big = 1;
    }
    if (big)
    {
        v = dx;
        if (dx < 0)
            v = dx + div - 1;
        dx = v >> 8;
        v = dy;
        if (dy < 0)
            v = dy + div - 1;
        dy = v >> 8;
        v = dz;
        if (dz < 0)
            v = dz + div - 1;
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
