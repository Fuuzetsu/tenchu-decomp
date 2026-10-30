#include "common.h"
#include "telop.h"
#include "../main.exe/main.exe.h"
#include "../main.exe/font.h"
#include "../main.exe/tim.h"
#include <psxsdk/libgpu.h>

/* TRIAL reads double-byte Japanese text directly from the BIOS font. */
void SetupTelop(u8 *telop, short line)
{
    s16 bitmap[TELOP_BITMAP_SIZE][TELOP_BITMAP_SIZE];
    RECT rect;
    s16 n;
    s16 u;
    s16 v;
    s16 *font;
    s16 bits;
    u16 raw_bits;
    s32 line_y;
    s32 signed_v;
    s32 final_v;

    TelopP.u1 = 0;
    TelopP.u0 = 0;
    {
        setRECT(&rect, TELOP_VRAM_X,
                TELOP_VRAM_BASE_Y - line * TELOP_BITMAP_SIZE,
                TELOP_VRAM_STRIP_WIDTH, TELOP_GLYPH_ROWS);
        line_y = line * TELOP_BITMAP_SIZE;
        ClearImage(&rect, TELOP_PIXEL_TRANSPARENT,
                   TELOP_PIXEL_TRANSPARENT, TELOP_PIXEL_TRANSPARENT);
        DrawSync(0);
        rect.w = TELOP_BITMAP_SIZE;
        rect.h = TELOP_GLYPH_ROWS;

        if (*telop == 0)
        {
            TelopP.u1 = 0;
            TelopP.u0 = 0;
            return;
        }

        n = 0;
        while (1)
        {
            if (n >= TELOP_MAX_SJIS_BYTES)
            {
                break;
            }

            font = (s16 *)Krom2RawAdd((telop + n)[1] | (telop[n] << 8));

            if (font != (s16 *)-1)
            {
                v = 0;
                do
                {
                    raw_bits = font[v];
                    bits = raw_bits >> 8;
                    bits |= raw_bits << 8;
                    u = 0;
                    do
                    {
                        bitmap[v][TELOP_BITMAP_SIZE - 1 - u] =
                            ((bits >> u) & 1) ? TELOP_PIXEL_WHITE
                                             : TELOP_PIXEL_TRANSPARENT;
                        u++;
                    } while (u < TELOP_BITMAP_SIZE);
                    v++;
                } while (v < TELOP_GLYPH_ROWS);
                u = TELOP_OUTLINE_FIRST_PIXEL;
                v = TELOP_OUTLINE_FIRST_PIXEL;
                do
                {
                    if (bitmap[v][u] == TELOP_PIXEL_TRANSPARENT)
                    {
                        if ((bitmap[v - 1][u] == TELOP_PIXEL_WHITE &&
                             bitmap[v][u - 1] == TELOP_PIXEL_WHITE) ||
                            (bitmap[v][u - 1] == TELOP_PIXEL_WHITE &&
                             bitmap[v + 1][u] == TELOP_PIXEL_WHITE) ||
                            (bitmap[v + 1][u] == TELOP_PIXEL_WHITE &&
                             bitmap[v][u + 1] == TELOP_PIXEL_WHITE) ||
                            (bitmap[v][u + 1] == TELOP_PIXEL_WHITE &&
                             bitmap[v - 1][u] == TELOP_PIXEL_WHITE))
                        {
                            bitmap[v][u] = TELOP_PIXEL_OUTLINE;
                        }
                    }
                    u++;
                    signed_v = v;
                    if (u >= TELOP_OUTLINE_X_LIMIT)
                    {
                        u = TELOP_OUTLINE_FIRST_PIXEL;
                        v = signed_v + 1;
                    }
                    else
                    {
                        v = signed_v;
                    }
                } while (v < TELOP_OUTLINE_Y_LIMIT);

                LoadImage(&rect, (u_long *)bitmap);
                DrawSync(0);
                rect.x += rect.w;
            }

            n += TELOP_BYTES_PER_SJIS_GLYPH;
            if (telop[n] == 0)
            {
                break;
            }
        }

        memset(&TelopP, 0xff, sizeof(TelopP));
        final_v = SCREEN_H - line_y;
        u = (u16)rect.x - TELOP_TEXTURE_U_ORIGIN;
        setPolyFT4(&TelopP);
        TelopP.u2 = 0;
        TelopP.u0 = 0;
        TelopP.v1 = final_v;
        TelopP.v0 = final_v;
        TelopP.u3 = u;
        TelopP.u1 = u;
        TelopP.v3 = (u8)rect.h + final_v;
        TelopP.v2 = (u8)rect.h + final_v;
        TelopP.tpage = GetTPage(TIM_PIXEL_MODE_16BPP, GPU_BLEND_AVERAGE,
                                TELOP_VRAM_X,
                                TELOP_VRAM_BASE_Y - (s16)line_y);
    }
}

/* Same glyph rasterizer, with the editor's six foreground/outline pairs. */
void SetupTrialColoredTelop(u8 *telop, short line, TrialTelopStyle style)
{
    s16 bitmap[TELOP_BITMAP_SIZE][TELOP_BITMAP_SIZE];
    RECT rect;
    s16 colors[TRIAL_TELOP_STYLE_COUNT][2] = {
        {TELOP_PIXEL_WHITE, TELOP_PIXEL_OUTLINE},
        {0x0421, 0x2108},
        {0x7000, TELOP_PIXEL_TRANSPARENT},
        {0x03e0, TELOP_PIXEL_TRANSPARENT},
        {0x001f, TELOP_PIXEL_TRANSPARENT},
        {0x03ff, TELOP_PIXEL_TRANSPARENT}
    };
    s16 n;
    s16 u;
    s16 v;
    s16 *font;
    s16 bits;
    u16 raw_bits;
    s32 line_y;
    s32 signed_v;
    s32 final_v;

    TelopP.u1 = 0;
    TelopP.u0 = 0;
    {
        setRECT(&rect, TELOP_VRAM_X,
                TELOP_VRAM_BASE_Y - line * TELOP_BITMAP_SIZE,
                TELOP_VRAM_STRIP_WIDTH, TELOP_GLYPH_ROWS);
        line_y = line * TELOP_BITMAP_SIZE;
        ClearImage(&rect, TELOP_PIXEL_TRANSPARENT,
                   TELOP_PIXEL_TRANSPARENT, TELOP_PIXEL_TRANSPARENT);
        DrawSync(0);
        rect.w = TELOP_BITMAP_SIZE;
        rect.h = TELOP_GLYPH_ROWS;

        if (*telop == 0)
        {
            TelopP.u1 = 0;
            TelopP.u0 = 0;
            return;
        }

        for (n = 0; telop[n] != 0; n += TELOP_BYTES_PER_SJIS_GLYPH)
        {
            if (n >= TELOP_MAX_SJIS_BYTES)
                break;

            font = (s16 *)Krom2RawAdd((telop + n)[1] | (telop[n] << 8));

            if (font != (s16 *)-1)
            {
                for (v = 0; v < TELOP_GLYPH_ROWS; v++)
                {
                    raw_bits = font[v];
                    bits = raw_bits >> 8;
                    bits |= raw_bits << 8;
                    for (u = 0; u < TELOP_BITMAP_SIZE; u++)
                    {
                        bitmap[v][TELOP_BITMAP_SIZE - 1 - u] =
                            ((bits >> u) & 1) ? colors[style][0]
                                             : TELOP_PIXEL_TRANSPARENT;
                    }
                }
                u = TELOP_OUTLINE_FIRST_PIXEL;
                v = TELOP_OUTLINE_FIRST_PIXEL;
                do
                {
                    if (bitmap[v][u] == TELOP_PIXEL_TRANSPARENT)
                    {
                        if ((bitmap[v - 1][u] == colors[style][0] &&
                             bitmap[v][u - 1] == colors[style][0]) ||
                            (bitmap[v][u - 1] == colors[style][0] &&
                             bitmap[v + 1][u] == colors[style][0]) ||
                            (bitmap[v + 1][u] == colors[style][0] &&
                             bitmap[v][u + 1] == colors[style][0]) ||
                            (bitmap[v][u + 1] == colors[style][0] &&
                             bitmap[v - 1][u] == colors[style][0]))
                        {
                            bitmap[v][u] = colors[style][1];
                        }
                    }
                    u++;
                    signed_v = v;
                    if (u >= TELOP_OUTLINE_X_LIMIT)
                    {
                        u = TELOP_OUTLINE_FIRST_PIXEL;
                        v = signed_v + 1;
                    }
                    else
                    {
                        v = signed_v;
                    }
                } while (v < TELOP_OUTLINE_Y_LIMIT);

                LoadImage(&rect, (u_long *)bitmap);
                DrawSync(0);
                rect.x += rect.w;
            }
        }

        memset(&TelopP, 0xff, sizeof(TelopP));
        final_v = SCREEN_H - line_y;
        u = (u16)rect.x - TELOP_TEXTURE_U_ORIGIN;
        setPolyFT4(&TelopP);
        TelopP.u2 = 0;
        TelopP.u0 = 0;
        TelopP.v1 = final_v;
        TelopP.v0 = final_v;
        TelopP.u3 = u;
        TelopP.u1 = u;
        TelopP.v3 = (u8)rect.h + final_v;
        TelopP.v2 = (u8)rect.h + final_v;
        TelopP.tpage = GetTPage(TIM_PIXEL_MODE_16BPP, GPU_BLEND_AVERAGE,
                                TELOP_VRAM_X,
                                TELOP_VRAM_BASE_Y - (s16)line_y);
    }
}
