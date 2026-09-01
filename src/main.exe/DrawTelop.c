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
 * background quad in two halves (top strip y in [90,120], bottom strip y
 * in [-120,-90]) via GsSortPoly, then computes the caption text's pixel
 * width (telop_text_width_, matched — same TU) and hands its centered X position
 * to the text-draw helper draw_telop_line_ along with the ordering-table's `org`
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
 *  - `w = telop_text_width_(...);` as its own statement (not inlined into the
 *    later call's argument list) matches the asm's evaluation order, where
 *    the width-derived arg2 is computed before arg1 (OTablePt->org, a fresh
 *    load right before the call).
 */
extern u8 TelopText[];

extern s32 telop_text_width_(u8 *str);
extern void draw_telop_line_(GsOT_TAG *org, s32 x, s32 y, u8 *str);

void DrawTelop(void)
{
    enum
    {
        TELOP_INNER_Y = 90
    };
    s32 w;

    TelopbgP.y1 = TELOP_INNER_Y;
    TelopbgP.y0 = TELOP_INNER_Y;
    TelopbgP.y3 = SCREEN_H / 2;
    TelopbgP.y2 = SCREEN_H / 2;
    GsSortPoly(&TelopbgP, OTablePt, 1);
    TelopbgP.y1 = -(SCREEN_H / 2);
    TelopbgP.y0 = -(SCREEN_H / 2);
    TelopbgP.y3 = -TELOP_INNER_Y;
    TelopbgP.y2 = -TELOP_INNER_Y;
    GsSortPoly(&TelopbgP, OTablePt, 1);
    w = telop_text_width_(TelopText);
    draw_telop_line_(OTablePt->org, -(w / 2), 92, TelopText);
}
