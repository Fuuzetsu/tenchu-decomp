#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetVectorLength(long dx, long dy, long dz);
 *     EFFECT.C:493, 12 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long dx
 *     param $a1       long dy
 *     param $a2       long dz
 * END PSX.SYM */

extern long abs(long x);

long GetVectorLength(long dx, long dy, long dz)
{
    enum
    {
        div = 256
    };
    long len;
    int big;
    long v;

    big = 0;
    if (abs(dx) > FIXED_ONE || abs(dy) > FIXED_ONE ||
        abs(dz) > FIXED_ONE)
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
        len = len * div;
    }
    else
    {
        len = SquareRoot0(dx * dx + dy * dy + dz * dz);
    }
    return len;
}
