#include "common.h"
#include "main.exe.h"
#include "tim.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct BackGround * SetupBG(struct GsIMAGE *image, short w, short h);
 *     3DCTRL.C:622, 60 src lines, frame 96 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param stack+0   struct GsIMAGE * image
 *     param $a1       short w
 *     param $a2       short h
 *     reg   $s4       struct BackGround * bg
 *     reg   $s2       struct GsCELL * cell
 *     reg   $s5       short x
 *     stack sp+16     short y
 *     stack sp+24     short sy
 *     reg   $a1       short n
 *     reg   $s1       short pmode
 *     stack sp+32     short size
 * END PSX.SYM */

/* Exact retail reconstruction. Keeping the unmasked image mode in its own
 * unsigned capture preserves h in $s1 until the later mask. Reading w/h back
 * through their stored fields makes cc1 emit the target's independent
 * midpoint narrowing. The cell-width capture and source-level u-before-v
 * stores are also load-bearing for the inner-loop allocation and schedule. */

extern char msg_no_background_image_data[]; /* NO BACKGROUND IMAGE DATA */
extern void *valloc(u32 size);

BackGround *SetupBG(GsIMAGE *image, short w, short h)
{
    BackGround *bg;
    GsCELL *cell;
    short x;
    short y;
    short sy;
    short n;
    short pmode;
    short size;
    u16 raw_pmode;

    if (image == 0)
        SystemOut((u8 *)msg_no_background_image_data);

    bg = (BackGround *)valloc(sizeof(BackGround));
    bg->id = CONFLICT_NONE;
    bg->attribute = 0;
    memset(bg, 0, sizeof(GsBG));

    raw_pmode = image->pmode;
    bg->hundle.r = bg->hundle.g = bg->hundle.b = 0x80;
    bg->hundle.scalex = bg->hundle.scaley = 0x1000;
    bg->hundle.w = w;
    bg->hundle.h = h;
    bg->hundle.map = &bg->map;
    bg->map.cellw = bg->map.cellh = 0x10;
    bg->map.ncellw = w / bg->map.cellw;
    pmode = TIM_PIXEL_MODE(raw_pmode);
    bg->hundle.attribute = pmode << 24;
    bg->hundle.mx = bg->hundle.w >> 1;
    bg->hundle.my = bg->hundle.h >> 1;
    bg->map.ncellh = h / bg->map.cellh;

    size = (short)(bg->map.ncellw * bg->map.ncellh);
    bg->map.index = bg->index = (u16 *)valloc(size << 1);
    n = 0;
    if (size > 0)
    {
        do
        {
            bg->index[n++] = 0xffff;
        } while (n < size);
    }

    n = ((u16)image->pw << (2 - pmode)) / bg->map.cellw;
    sy = (short)((u16)image->ph / bg->map.cellh);
    bg->map.base = bg->cell =
        (GsCELL *)valloc(n * sy * sizeof(GsCELL));

    size = (1 << (8 - pmode)) - 1;
    for (y = 0; y < sy; y++)
    {
        for (x = 0; x < n; x++)
        {
            u8 cellw;
            short basepx;
            short py;
            short px;

            cellw = bg->map.cellw;
            py = image->py;
            basepx = image->px;
            px = basepx;
            px += x * (cellw >> (2 - pmode));
            py += y * bg->map.cellh;
            cell = &bg->cell[y * n + x];
            cell->u = ((basepx << (2 - pmode)) +
                       x * cellw) &
                      size;
            cell->v = py;
            cell->cba = GetClut(image->cx, image->cy);
            cell->flag = 0;
            cell->tpage = GetTPage(pmode, 0, px, py);
        }
    }

    bg->work = (u32 *)valloc(
        (s16)((bg->map.ncellw + 1) * (bg->map.ncellh + 2) * 12 + 10) * 4);
    GsInitFixBg16(&bg->hundle, bg->work);
    bg->sz = 0;
    return bg;
}
