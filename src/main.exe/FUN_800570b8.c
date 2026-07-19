#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct POLY_FT4 TelopP;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * FUN_800570b8 (0x800570b8, 0x160 bytes) — the telop (on-screen caption)
 * text-line renderer, called by DrawTelop (matched, same TU) with
 * `org = OTablePt->org`, `x = -(width/2)`, `y = 0x5c`, `str` the caption
 * text. If a telop is already "active" (TelopP.u0 or TelopP.u1 nonzero —
 * FUN_800576e8's same proven TelopType view, this TU's shorthand for
 * "already queued/fading"), instead draws TelopP itself as a textured quad
 * via GsSortPoly (the real PSYQ POLY_FT4, proven in full by
 * SetupImageToPolyFT4.c — x0/x2 = x, y0/y1 = y, x1/x3 = x + (u1-u0)
 * [i.e. the queued caption's own pixel width], y2/y3 = y+15). Otherwise
 * walks `str`, drawing one glyph per byte via FUN_8005778c (still asm;
 * takes the same org/x/y/char signature) at a cursor that resets to the
 * start-of-line X on '\n' and otherwise advances by each glyph's width out
 * of the per-glyph table D_8008EF98[] (the SAME SJIS-code-to-glyph-index
 * remap as FUN_800576e8: 0x92->0x27, -0x20 if >=0x20, an extra -0x40 for
 * the upper half-width-kana block >=0xC0). No confirmed original name.
 *
 * STATUS: MATCHING — exact 352-byte / 88-instruction pure-C body.  Separate
 * cursor, Y-position, and text-pointer aliases recover the original prologue
 * copy order.  Four chained quad-field assignments recover the store schedule,
 * and distinct character/index roles preserve the pre-adjustment character for
 * the upper-kana test.
 */

typedef struct
{
    u_long tag;
    u_char r0, g0, b0, code;
    s16 x0, y0;
    u8 u0, v0;
    u16 clut;
    s16 x1, y1;
    u8 u1, v1;
    u16 tpage;
    s16 x2, y2;
    u8 u2, v2;
    u16 pad1;
    s16 x3, y3;
    u8 u3, v3;
    u16 pad2;
} TelopPolyType;

struct GsOT_TAG;

extern TelopPolyType TelopP;
extern GsOT *OTablePt;
extern u8 D_8008EF98[];
extern void FUN_8005778c(struct GsOT_TAG *org, s32 x, s32 y, u32 ch);

void FUN_800570b8(struct GsOT_TAG *org, s32 x, s32 y, u8 *str)
{
    s32 cursor;
    s32 ypos;
    u8 *text;
    s32 ch;
    s32 index;

    cursor = x;
    text = str;
    ypos = y;
    if (*text != 0) {
        if (TelopP.u0 == 0) {
            if (TelopP.u1 == 0) {
                goto charloop;
            }
        }
        TelopP.x0 = TelopP.x2 = cursor;
        TelopP.y0 = TelopP.y1 = ypos;
        TelopP.y2 = TelopP.y3 = ypos + 0xf;
        TelopP.x1 = TelopP.x3 = cursor + (TelopP.u1 - TelopP.u0);
        GsSortPoly(&TelopP, OTablePt, 0);
        goto end;
    charloop:
        do {
            if (*text == 10) {
                cursor = x;
                ypos = ypos + 0x10;
            } else {
                FUN_8005778c(org, cursor, ypos, *text);
                ch = *text;
                if (ch == 0x92) {
                    ch = 0x27;
                }
                index = ch;
                if (index > 0x1f) {
                    index = index - 0x20;
                }
                if (ch > 0xbf) {
                    index = index - 0x40;
                }
                cursor = cursor + D_8008EF98[index];
            }
            text++;
        } while (*text != 0);
    end:;
    }
}
