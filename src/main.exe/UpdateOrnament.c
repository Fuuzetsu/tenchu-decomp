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

void UpdateOrnament(OrnamentType *objp, short ry)
{
    SVECTOR rotv;

    rotv = UnitVector;
    rotv.vy = ry;
    RotMatrixYXZ(&rotv, &objp->locate.coord);
    objp->locate.flg = 0;
}
