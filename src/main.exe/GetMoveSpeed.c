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

/*
 * GetMoveSpeed (0x80029660, 0xcc bytes) — same "Humanoid control" TU as
 * MoveHumanoid.c/GetHumanoid.c (HUMAN.C). Fills *vect with the (vx,vz)
 * velocity for a given facing angle `ry` and an (ordr,side) speed pair —
 * MoveHumanoid computes the same rotation from human->rotate->vy instead of
 * a plain parameter; this looks like its shared helper.
 *
 * Matching notes (see MoveHumanoid's identical idioms):
 *  - `s = -rsin(...); c = -rcos(...);` negates AT THE ASSIGNMENT — reorg
 *    steals the first negate into the second call's delay slot (the
 *    "x = -f(...)" rule), so -sin is live across the rcos call.
 *  - `(short)s`/`(short)c` casts are written INLINE at each of their two
 *    uses in the vx/vz expressions; cc1 CSEs the repeated cast into a
 *    single truncation per variable (no re-truncation on reuse).
 *  - `ordr`/`side` are the plain `short` parameters (no MoveHumanoid-style
 *    io/o resign locals — this function does no -0x100 byte-resign); their
 *    promotion to int is the ordinary implicit one (the decompiler's
 *    explicit (int) wrappers were measured byte-free and removed).
 */

void GetMoveSpeed(SVECTOR *vect, short ry, short ordr, short side)
{
    int s, c;

    s = -rsin(ry);
    c = -rcos(ry);
    vect->vy = 0;
    vect->vx = (short)(((short)s * ordr - (short)c * side) >> 0xc);
    vect->vz = (short)(((short)c * ordr + (short)s * side) >> 0xc);
}
