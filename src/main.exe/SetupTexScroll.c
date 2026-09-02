#include "common.h"
#include "main.exe.h"
#include "effect.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct TexScroll * SetupTexScroll(struct GsIMAGE *img, short x, short y, short mode);
 *     EFFECT.C:1853, 27 src lines, frame 64 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct GsIMAGE * img
 *     param $a1       short x
 *     param $a2       short y
 *     param $a3       short mode
 * END PSX.SYM */

extern s16 TexScrollX;
extern s16 TexScrollY;

void SetupTexScroll(GsIMAGE *img, short vx, short vy)
{
    int idx;
    TEffectSlot *slot;
    int count;
    TexScroll *tscr;
    s16 scrollX;
    short scrollY;
    short j;
    short i;

    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
{
    u32 scrollYShifted;
    int sx;
    short mask;

    tscr = &slot->param.texscroll;
    slot->param.texscroll.px = tscr->py = 0;

    scrollX = TexScrollX;
    scrollY = TexScrollY;
    tscr->sx = scrollX;
    tscr->sy = scrollY;

    tscr->image.x = tscr->x = img->px;
    tscr->image.y = tscr->y = img->py;
    tscr->image.w = img->pw;
    tscr->image.h = img->ph;
    sx = scrollX;
    scrollYShifted = (u32)(u16)scrollY << 16;

    j = 0;
    while (1)
    {
        for (i = 0; i < TEXSCROLL_GRID_COLUMNS; i++)
        {
            mask = TEXSCROLL_COPY_ALL;
            if ((mask >> (j * TEXSCROLL_GRID_COLUMNS + i)) & 1)
            {
                MoveImage(&tscr->image, sx + img->pw * i,
                          ((s32)scrollYShifted >> 16) + img->ph * j);
            }
        }
        j++;
        if (j >= TEXSCROLL_GRID_ROWS)
        {
            scrollYShifted = 0;
            break;
        }
    }

    TexScrollY += TEXSCROLL_VRAM_SLOT_STRIDE;
    tscr->vx = vx;
    tscr->vy = vy;
    slot->proc = UpdateTexScroll;
    if (TexScrollY > TEXSCROLL_VRAM_Y_LIMIT)
    {
        TexScrollY = TEXSCROLL_VRAM_ORIGIN_Y;
        TexScrollX += TEXSCROLL_VRAM_SLOT_STRIDE;
    }
}
}
