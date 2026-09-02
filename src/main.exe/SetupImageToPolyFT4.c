#include "common.h"
#include "main.exe.h"
#include "images.h"
#include "tim.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupImageToPolyFT4(struct GsIMAGE *image, struct POLY_FT4 *ply, short x, short y);
 *     IMAGES.C:106, 21 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct GsIMAGE * image
 *     param $s0       struct POLY_FT4 * ply
 *     param $a2       short x
 *     param $a3       short y
 *     reg   $a1       short tx
 *     reg   $a3       short ty
 *     reg   $t0       short th
 * END PSX.SYM */

void SetupImageToPolyFT4(GsIMAGE *image, POLY_FT4 *ply, short x, short y)
{
    s32 tp;
    s32 sh;
    s32 tw;
    u32 tx;
    u16 tx2;
    s32 px;
    u8 ty;
    u32 pw;
    u32 th;

    SetPolyFT4(ply);
    tp = TIM_PIXEL_MODE((u16)image->pmode);
    ply->tpage = GetTPage(tp, 1, image->px, image->py);
    ply->clut = GetClut(image->cx, image->cy);
    sh = 2 - tp;
    px = image->px;
    ty = (u8)image->py;
    pw = image->pw;
    th = image->ph;
    setRGB0(ply, 0x7F, 0x7F, 0x7F);
    ply->x0 = x;
    ply->y0 = y;
    ply->y1 = y;
    ply->x2 = x;
    tx = (px << sh) & ((1 << (8 - tp)) - 1);
    tw = pw << sh;
    x += tw;
    y += th;
    /* Empty loop retained for code layout; its original source construct is unknown. */
    do
    {
    } while (0);
    tx2 = tx + tw;
    ply->x1 = x;
    ply->y2 = y;
    ply->x3 = x;
    ply->y3 = y;
    setUV4(ply, tx, ty, tx2, ty, tx, ty + th, tx2, ty + th);
}
