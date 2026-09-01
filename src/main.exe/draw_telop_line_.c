#include "common.h"
#include "main.exe.h"
#include "font.h"

/*
 * draw_telop_line_ (0x800570b8, 0x160 bytes) — the telop (on-screen caption)
 * text-line renderer, called by DrawTelop (matched, same TU) with
 * `org = OTablePt->org`, `x = -(width/2)`, `y = 0x5c`, `str` the caption
 * text. If a telop is already "active" (TelopP.u0 or TelopP.u1 nonzero),
 * instead draws TelopP itself as a textured quad
 * via GsSortPoly (the real PSYQ POLY_FT4, proven in full by
 * SetupImageToPolyFT4.c — x0/x2 = x, y0/y1 = y, x1/x3 = x + (u1-u0)
 * [i.e. the queued caption's own pixel width], y2/y3 = y+15). Otherwise
 * walks `str`, drawing one glyph per byte via draw_glyph_ (still asm;
 * takes the same org/x/y/char signature) at a cursor that resets to the
 * start-of-line X on '\n' and otherwise advances by each glyph's width out
 * of the per-glyph table FontWidth[] (the same font-block remap as
 * telop_text_width_). No confirmed original name.
 *
 * STATUS: MATCHING — exact 352-byte / 88-instruction pure-C body.  Separate
 * cursor, Y-position, and text-pointer aliases recover the original prologue
 * copy order.  Four chained quad-field assignments recover the store schedule,
 * and distinct character/index roles preserve the pre-adjustment character for
 * the upper-kana test.
 */

extern u8 FontWidth[];
extern void draw_glyph_(GsOT_TAG *org, s32 x, s32 y, u32 ch);

void draw_telop_line_(GsOT_TAG *org, s32 x, s32 y, u8 *str)
{
    s32 cursor;
    s32 ypos;
    u8 *text;
    s32 ch;
    s32 index;

    cursor = x;
    text = str;
    ypos = y;
    if (*text != 0)
    {
        if (TelopP.u0 == 0)
        {
            if (TelopP.u1 == 0)
            {
                goto charloop;
            }
        }
        TelopP.x0 = TelopP.x2 = cursor;
        TelopP.y0 = TelopP.y1 = ypos;
        TelopP.y2 = TelopP.y3 = ypos + FONT_GLYPH_HEIGHT - 1;
        TelopP.x1 = TelopP.x3 = cursor + (TelopP.u1 - TelopP.u0);
        GsSortPoly(&TelopP, OTablePt, 0);
        goto end;
    charloop:
        do
        {
            if (*text == '\n')
            {
                cursor = x;
                ypos += FONT_GLYPH_HEIGHT;
            }
            else
            {
                draw_glyph_(org, cursor, ypos, *text);
                ch = *text;
                if (ch == FONT_REMAP_CODE)
                {
                    ch = FONT_REMAP_TARGET;
                }
                index = ch;
                if (index >= FONT_PRINTABLE_FIRST)
                {
                    index -= FONT_PRINTABLE_FIRST;
                }
                if (ch >= FONT_UPPER_BLOCK_FIRST)
                {
                    index -= FONT_UPPER_BLOCK_OFFSET;
                }
                cursor += FontWidth[index];
            }
            text++;
        } while (*text != 0);
    end:;
    }
}
