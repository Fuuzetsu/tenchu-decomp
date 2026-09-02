#include "common.h"
#include "main.exe.h"
#include "font.h"
#include "tim.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupTelop(unsigned char *telop);
 *     CHRANIM.C:372, 44 src lines, frame 560 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s4       unsigned char * telop
 *     reg   $a2       short * font
 *     stack sp+16     short [16][16] bitmap
 *     reg   $s0       short n
 *     reg   $t0       short u
 *     reg   $t1       short v
 *     stack sp+528    struct RECT rect
 *
 * Globals it touches, as the original declared them:
 *     extern struct POLY_FT4 TelopP;
 * END PSX.SYM */

extern s16 TelopFont[];

extern s16 *Krom2RawAdd(u32 code);
extern void *memset(void *dst, int value, u32 size);

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
    /* Both the first and the third byte have their high bit set, so the
     * line is Shift-JIS double-byte text rather than ASCII, and needs the
     * kanji glyph path below instead of the font sprites. */
    if ((*telop & 0x80) != 0 && (telop[2] & 0x80) != 0)
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

            if (telop[n] == TELOP_CUSTOM_STAR_LEAD &&
                telop[n + 1] == TELOP_CUSTOM_STAR_TRAIL)
            {
                font = TelopFont;
            }
            else
            {
                font = Krom2RawAdd((telop[n] << 8) | telop[n + 1]);
            }

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
