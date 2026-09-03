#include "common.h"
#include "main.exe.h"
#include "font.h"

extern u8 FontWidth[];

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
