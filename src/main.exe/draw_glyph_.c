#include "common.h"
#include "main.exe.h"
#include "font.h"
#include "images.h"
#include <psxsdk/libgpu.h>

void draw_glyph_(GsOT_TAG *ot, s32 x, s32 y0, u32 code)
{
    u16 cell;
    POLY_GT4 *ply;
    s32 nudge;
    s32 c0;
    s32 t1;
    s32 t2;
    s32 row;
    u32 raw;
    u8 narrow;
    GsIMAGE img;

    narrow = code;
    ply = (POLY_GT4 *)GsGetWorkBase();
    GsSetWorkBase(ply + 1);
    raw = narrow;
    c0 = raw;
    if (raw == FONT_REMAP_CODE)
    {
        c0 = FONT_REMAP_TARGET;
    }
    t1 = c0;
    if (t1 >= FONT_PRINTABLE_FIRST)
    {
        t1 -= FONT_PRINTABLE_FIRST;
    }
    if (c0 >= FONT_UPPER_BLOCK_FIRST)
    {
        t1 -= FONT_UPPER_BLOCK_OFFSET;
    }
    t2 = t1;
    cell = (u16)t2;
    img = FONT_IMAGE_;
    img.px += (cell & (FONT_ATLAS_COLUMNS - 1)) * FONT_GLYPH_WIDTH;
    row = t2 / FONT_ATLAS_COLUMNS;
    img.pw = FONT_GLYPH_WIDTH;
    img.ph = FONT_GLYPH_HEIGHT;
    img.py += row * FONT_GLYPH_HEIGHT;
    if (raw - FONT_UPPER_BLOCK_FIRST < FONT_CODE_BLOCK_SIZE &&
        raw != FONT_NUDGE_EXEMPT)
    {
        nudge = FONT_NUDGE_UPPER;
    }
    else if (raw - FONT_EXTENDED_BLOCK_FIRST < FONT_CODE_BLOCK_SIZE)
    {
        nudge = FONT_NUDGE_EXTENDED;
        if (raw == FONT_RAISED_CODE)
        {
            nudge = FONT_NUDGE_RAISED;
        }
    }
    else
    {
        nudge = FONT_NUDGE_NONE;
    }
    {
        int y = (short)(y0 + nudge);
        SetupImageToPolyGT4(&img, ply, (short)x, y);
    }
    AddPrim(ot, ply);
}
