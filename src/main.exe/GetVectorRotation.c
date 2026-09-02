#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetVectorRotation(struct VECTOR *start, struct VECTOR *end, int *rx, int *ry);
 *     EFFECT.C:532, 7 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * start
 *     param $a1       struct VECTOR * end
 *     param $a2       int * rx
 *     param $a3       int * ry
 * END PSX.SYM */

void GetVectorRotation(VECTOR *start, VECTOR *end, int *rx, int *ry)
{
    s32 dx;
    s32 dy;
    s32 dz;

    dx = end->vx - start->vx;
    dz = end->vz - start->vz;
    dy = end->vy - start->vy;
    *ry = ratan2(-dx, -dz);
    *rx = ratan2(dy, SquareRoot0((dx * dx) + (dz * dz)));
}
