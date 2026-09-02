#include "common.h"
#include "main.exe.h"
#include "humanoid.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetMoveSpeed(struct SVECTOR *vect, short ry, short ordr, short side);
 *     HUMAN.C:370, 7 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct SVECTOR * vect
 *     param $a1       short ry
 *     param $a2       short ordr
 *     param $a3       short side
 * END PSX.SYM */

void GetMoveSpeed(SVECTOR *vect, short ry, short ordr, short side)
{
    int s, c;

    s = -rsin(ry);
    c = -rcos(ry);
    vect->vy = 0;
    vect->vx = (short)(((short)s * ordr - (short)c * side) >> FIXED_SHIFT);
    vect->vz = (short)(((short)c * ordr + (short)s * side) >> FIXED_SHIFT);
}
