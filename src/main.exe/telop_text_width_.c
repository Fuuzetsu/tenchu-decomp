#include "common.h"
#include "main.exe.h"
#include "font.h"

extern u8 FontWidth[];

s32 telop_text_width_(u8 *str)
{
    s32 width;
    s32 code;
    s32 idx;

    width = 0;
    if (TelopP.u0 != 0 || TelopP.u1 != 0)
    {
        width = TelopP.u1 - TelopP.u0;
    }
    else if (*str != 0)
    {
        do
        {
            code = *str;
            str++;
            if (code == FONT_REMAP_CODE)
            {
                code = FONT_REMAP_TARGET;
            }
            idx = code;
            if (idx >= FONT_PRINTABLE_FIRST)
            {
                idx -= FONT_PRINTABLE_FIRST;
            }
            if (code >= FONT_UPPER_BLOCK_FIRST)
            {
                idx -= FONT_UPPER_BLOCK_OFFSET;
            }
            width += FontWidth[idx];
        } while (*str != 0);
    }
    return width;
}
