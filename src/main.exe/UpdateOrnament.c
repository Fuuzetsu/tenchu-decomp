#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateOrnament(struct OrnamentType *objp, short ry);
 *     3DCTRL.C:532, 8 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct OrnamentType * objp
 *     param $a1       short ry
 *     stack sp+16     struct SVECTOR rotv
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR UnitVector;
 * END PSX.SYM */

/*
 * UpdateOrnament (0x800187bc) — like UpdateCoordinate, but OrnamentType has no
 * own `rotate` SVECTOR: build one from UnitVector (the identity SVECTOR, see
 * InsertConflict.c) with vy overridden to `ry`, then recompute the local
 * matrix and clear the GsCOORDINATE2 dirty flag.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `rotv = UnitVector;` is the align-2 SVECTOR struct-copy idiom
 *    (lwl/lwr + swl/swr pairs, see InsertConflict's `.offset`/`.size`);
 *    the following `rotv.vy = ry;` is a plain `sh` overwriting the copied
 *    half, executed AFTER the copy (matches asm order).
 *  - OrnamentType is the complete shared PSX.SYM record; this function only
 *    touches its leading `locate` member.
 */

void UpdateOrnament(OrnamentType *objp, short ry)
{
    SVECTOR rotv;

    rotv = UnitVector;
    rotv.vy = ry;
    RotMatrixYXZ(&rotv, &objp->locate.coord);
    objp->locate.flg = 0;
}
