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
 *    io/o resign locals — this function does no -0x100 byte-resign), so
 *    their (int) promotion is deferred to each expression's first use.
 */

void GetMoveSpeed(SVECTOR *vect, short ry, short ordr, short side)
{
    int s, c;

    s = -rsin((int)ry);
    c = -rcos((int)ry);
    vect->vy = 0;
    vect->vx = (short)(((int)(short)s * (int)ordr - (int)(short)c * (int)side) >> 0xc);
    vect->vz = (short)(((int)(short)c * (int)ordr + (int)(short)s * (int)side) >> 0xc);
}
