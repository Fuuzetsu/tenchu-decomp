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

extern long abs(long x);

int GetVectorDistance(VECTOR *v1, VECTOR *v2)
{
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
        len = len << 8;
    }
    else
    {
        len = SquareRoot0(dx * dx + dy * dy + dz * dz);
    }
    return len;
}
