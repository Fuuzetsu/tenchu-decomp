#include "common.h"
#include "text_entry.h"
#include <psxsdk/libapi.h>
#include <psxsdk/libgpu.h>

/* Convert the BIOS's one-bit, big-endian glyph to a 16-bit texture. Empty
 * interior corners get a second shade; an absent glyph is distinct from space. */
short UploadTrialGlyph(u16 code, RECT *area)
{
    u16 pixels[16][16];
    u16 *glyph;
    u16 bits;
    int x, y;

    glyph = (u16 *)Krom2RawAdd2(code);
    if (glyph == TrialBlankGlyph && code != TRIAL_SJIS_SPACE)
        return 0;
    for (y = 0; y < 15; y++)
    {
        bits = (glyph[y] >> 8) | (glyph[y] << 8);
        for (x = 0; x < 16; x++)
        {
            pixels[y][15 - x] = ((bits >> x) & 1) ? TRIAL_GLYPH_INK : 0;
        }
    }

    y = x = 1;
    while (y < 14)
    {
        if (pixels[y][x] == 0 &&
            ((pixels[y - 1][x] == TRIAL_GLYPH_INK &&
              pixels[y][x - 1] == TRIAL_GLYPH_INK) ||
             (pixels[y][x - 1] == TRIAL_GLYPH_INK &&
              pixels[y + 1][x] == TRIAL_GLYPH_INK) ||
             (pixels[y + 1][x] == TRIAL_GLYPH_INK &&
              pixels[y][x + 1] == TRIAL_GLYPH_INK) ||
             (pixels[y][x + 1] == TRIAL_GLYPH_INK &&
              pixels[y - 1][x] == TRIAL_GLYPH_INK)))
            pixels[y][x] = TRIAL_GLYPH_EDGE;
        x++;
        if (x >= 15)
        {
            x = 1;
            y++;
        }
    }
    LoadImage2(area, (u_long *)pixels);
    return 1;
}
