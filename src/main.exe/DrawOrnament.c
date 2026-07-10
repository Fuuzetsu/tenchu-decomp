#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawOrnament(struct OrnamentType *objp);
 *     3DCTRL.C:496, 10 src lines, frame 56 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 * OrnamentType (Ghidra: `{ GsCOORDINATE2 locate; GsDOBJ2 object; }`) isn't
 * shared via item.h yet; declared locally with just the two accessed fields,
 * same convention as UpdateOrnament.c (locate@0, object right after it@0x50
 * — GsCOORDINATE2 is proven 0x50 bytes: flg(4)+coord(0x20)+workm(0x20)+
 * param/super/sub(4 each) = 0x50).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - m2c undercounts GsGetLs's call: `objp` (a0) is carried in live from the
 *    caller and never reassigned before the jal, so m2c's basic-block-local
 *    view misses it — Ghidra's 2-arg rendering (&objp->locate, &m) is the
 *    real call (same undercount pattern as DrawBG's FUN_80063b94).
 *  - OTablePt is %gp_rel in this TU, same as DrawBG (tools/gpsyms.py
 *    --write; Build.hs maspsxGpExterns + permute.py GP_EXTERNS both list
 *    DrawOrnament now).
 */
typedef struct
{
    GsCOORDINATE2 locate; /* 0x00 */
    GsDOBJ2 object;       /* 0x50 */
} OrnamentType;

extern void GsGetLs(GsCOORDINATE2 *coord, MATRIX *m);
extern void GsSetLsMatrix(MATRIX *m);
extern void DrawTMD(GsDOBJ2 *obj, GsOT *ot, s32 mode);
extern GsOT *OTablePt;

short DrawOrnament(OrnamentType *objp)
{
    MATRIX m;

    GsGetLs(&objp->locate, &m);
    GsSetLsMatrix(&m);
    DrawTMD(&objp->object, OTablePt, 0);
    return 1;
}
