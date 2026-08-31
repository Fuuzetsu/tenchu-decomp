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

/*
 * DrawOrnament (0x800186d4, 0x44 bytes) — like UpdateCoordinate/DrawBG's
 * sibling in this same TU: builds the local screen matrix for an ornament's
 * GsCOORDINATE2 (GsGetLs + GsSetLsMatrix, same pair GetAbsolutePosition uses
 * for the world variant GsGetLw), then draws its GsDOBJ2 via DrawTMD into the
 * global ordering table OTablePt, and always reports success (1).
 *
 * OrnamentType is the shared PSX.SYM record: locate@0, object@0x50.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - m2c undercounts GsGetLs's call: `objp` (a0) is carried in live from the
 *    caller and never reassigned before the jal, so m2c's basic-block-local
 *    view misses it — Ghidra's 2-arg rendering (&objp->locate, &mat) is the
 *    real call (same undercount pattern as DrawBG's FUN_80063b94).
 *  - OTablePt is %gp_rel in this TU, same as DrawBG (tools/gpsyms.py
 *    --write; Build.hs maspsxGpExterns + permute.py GP_EXTERNS both list
 *    DrawOrnament now).
 */
extern void DrawTMD(GsDOBJ2 *obj, GsOT *ot, s32 mode);

short DrawOrnament(OrnamentType *objp)
{
    MATRIX mat;

    GsGetLs(&objp->locate, &mat);
    GsSetLsMatrix(&mat);
    DrawTMD(&objp->object, OTablePt, 0);
    return 1;
}
