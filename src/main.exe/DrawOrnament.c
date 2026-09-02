#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawOrnament(struct OrnamentType *objp);
 *     3DCTRL.C:496, 10 src lines, frame 56 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct OrnamentType * objp
 *     stack sp+16     struct MATRIX mat
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

extern void DrawTMD(GsDOBJ2 *obj, GsOT *ot, s32 mode);

short DrawOrnament(OrnamentType *objp)
{
    MATRIX mat;

    GsGetLs(&objp->locate, &mat);
    GsSetLsMatrix(&mat);
    DrawTMD(&objp->object, OTablePt, 0);
    return 1;
}
