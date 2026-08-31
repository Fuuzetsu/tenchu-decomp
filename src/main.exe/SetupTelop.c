#include "common.h"
#include "main.exe.h"

/* A lit telop pixel: white in the 15-bit BGR the bitmap holds. */
#define TELOP_WHITE 0x7fff
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

/* STATUS: MATCHED — exact 1076 bytes / 269 instructions.
 *
 * Matching constraints:
 *  - Keep the glyph fill as one coherent source package: PSX.SYM's u is the
 *    inner pixel counter, bitmap[v][15 - u] is one ternary assignment, and
 *    font selection is ordinary if/else control flow. Synthetic one-shot
 *    fences, carrier variables, and loop-weighting nests are not required.
 *  - Write v = 0 before entering the fill loop. The independent zero
 *    initialization fills the font guard's delay slot and preserves the
 *    target allocation.
 *  - Keep the byte swap in two statements: bits = raw_bits >> 8 followed by
 *    bits |= raw_bits << 8. A fused expression reverses the operand emission
 *    order even though loop.c still hoists it.
 *  - The fill constant may remain literal in the bitmap assignment; CSE
 *    retains the target's single TELOP_WHITE value.
 *
 * The rounds 1–4 allocation floor was a property of the scaffolded draft, not
 * the recovered decomposition. Its superseded autopsy remains in
 * docs/matching-archive.md.
 */
extern s16 TelopFont[];

extern s16 *Krom2RawAdd(u32 code);
extern void *memset(void *dst, int value, u32 size);

void SetupTelop(u8 *telop, short line)
{
    s16 bitmap[16][16];
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
        setRECT(&rect, 0x300, 0x1f0 - line * 16, 0x100, 0xf);
        line_y = line * 16;
        ClearImage(&rect, 0, 0, 0);
        DrawSync(0);
        rect.w = 0x10;
        rect.h = 0xf;

        if (*telop == 0)
        {
            TelopP.u1 = 0;
            TelopP.u0 = 0;
            return;
        }

        n = 0;
        while (1)
        {
            if (n >= 0x20)
            {
                break;
            }

            if (telop[n] == 0x81 && telop[n + 1] == 0x99)
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
                    /* Two pseudos, deliberately: fusing them into one
                     * `bits = font[v]; bits = bits >> 8 | bits << 8;`
                     * costs 31 lines. Reading `(u16)font[v]` twice
                     * instead of naming `raw_bits` is also exact, but
                     * repeats the load with two added casts and PSX.SYM
                     * records NEITHER local, so there is no fidelity
                     * argument either way -- this spelling reads as the
                     * byte swap it is. */
                    raw_bits = font[v];
                    bits = raw_bits >> 8;
                    bits |= raw_bits << 8;
                    u = 0;
                    do
                    {
                        bitmap[v][15 - u] = ((bits >> u) & 1) ? TELOP_WHITE : 0;
                        u++;
                    } while (u < 16);
                    v++;
                } while (v < 15);
                u = 1;
                v = 1;
                do
                {
                    if (bitmap[v][u] == 0)
                    {
                        if ((bitmap[v - 1][u] == TELOP_WHITE && bitmap[v][u - 1] == TELOP_WHITE) ||
                            (bitmap[v][u - 1] == TELOP_WHITE && bitmap[v + 1][u] == TELOP_WHITE) ||
                            (bitmap[v + 1][u] == TELOP_WHITE && bitmap[v][u + 1] == TELOP_WHITE) ||
                            (bitmap[v][u + 1] == TELOP_WHITE && bitmap[v - 1][u] == TELOP_WHITE))
                        {
                            bitmap[v][u] = 0x1ce7;
                        }
                    }
                    u++;
                    signed_v = v;
                    if (u >= 15)
                    {
                        u = 1;
                        v = signed_v + 1;
                    }
                    else
                    {
                        v = signed_v;
                    }
                } while (v < 14);

                LoadImage(&rect, (u_long *)bitmap);
                DrawSync(0);
                rect.x += rect.w;
            }

            n += 2;
            if (telop[n] == 0)
            {
                break;
            }
        }

        memset(&TelopP, 0xff, sizeof(TelopP));
        final_v = SCREEN_H - line_y;
        u = (u16)rect.x - 0x301;
        setlen(&TelopP, 9);
        TelopP.code = 0x2c;
        TelopP.u2 = 0;
        TelopP.u0 = 0;
        TelopP.v1 = final_v;
        TelopP.v0 = final_v;
        TelopP.u3 = u;
        TelopP.u1 = u;
        TelopP.v3 = (u8)rect.h + final_v;
        TelopP.v2 = (u8)rect.h + final_v;
        TelopP.tpage = GetTPage(2, 0, 0x300,
                                0x1f0 - (s16)line_y);
    }
}
