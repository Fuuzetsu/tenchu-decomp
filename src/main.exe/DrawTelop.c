#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawTelop(void);
 *     CHRANIM.C:420, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct POLY_F4 TelopbgP;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * DrawTelop (0x8005141c, 0xb4 bytes) - draws the telop (on-screen caption)
 * background quad in two halves (top strip y in [0x5a,0x78], bottom strip y
 * in [-0x78,-0x5a]) via GsSortPoly, then computes the caption text's pixel
 * width (FUN_800576e8, matched — same TU) and hands its centered X position
 * to the text-draw helper FUN_800570b8 along with the ordering-table's `org`
 * pointer.
 *
 * TelopbgP is a canonical PsyQ POLY_F4, and the text helper receives the
 * canonical GsOT ordering table's `org` pointer.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - The second GsSortPoly's field stores address TelopbgP through $a0 (the
 *    register just computed for that call's own first argument), not the
 *    $s0 the first block's stores use — a plain repeated `TelopbgP.yN = ...;`
 *    with no local pointer variable reproduces both addressing choices; no
 *    manual pointer temp needed.
 *  - `w = FUN_800576e8(...);` as its own statement (not inlined into the
 *    later call's argument list) matches the asm's evaluation order, where
 *    the width-derived arg2 is computed before arg1 (OTablePt->org, a fresh
 *    load right before the call).
 */
extern POLY_F4 TelopbgP;
extern u8 D_800C2C50[];

extern s32 FUN_800576e8(u8 *str);
extern void FUN_800570b8(GsOT_TAG *org, s32 x, s32 y, u8 *str);

void DrawTelop(void)
{
    s32 w;

    TelopbgP.y1 = 0x5a;
    TelopbgP.y0 = 0x5a;
    TelopbgP.y3 = 0x78;
    TelopbgP.y2 = 0x78;
    GsSortPoly(&TelopbgP, OTablePt, 1);
    TelopbgP.y1 = -0x78;
    TelopbgP.y0 = -0x78;
    TelopbgP.y3 = -0x5a;
    TelopbgP.y2 = -0x5a;
    GsSortPoly(&TelopbgP, OTablePt, 1);
    w = FUN_800576e8(D_800C2C50);
    FUN_800570b8(OTablePt->org, -(w / 2), 0x5c, D_800C2C50);
}
