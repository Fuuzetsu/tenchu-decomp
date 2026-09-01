#include "common.h"
#include "main.exe.h"
#include "images.h"
#include "tim.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupImageToPolyGT4(struct GsIMAGE *image, struct POLY_GT4 *ply, short x, short y);
 *     IMAGES.C:129, 25 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct GsIMAGE * image
 *     param $s0       struct POLY_GT4 * ply
 *     param $a2       short x
 *     param $a3       short y
 *     reg   $a1       short tx
 *     reg   $a3       short ty
 *     reg   $t0       short th
 * END PSX.SYM */

/*
 * SetupImageToPolyGT4 (0x8004ec10, 0x144 bytes) — the Gouraud twin of
 * SetupImageToPolyFT4 (IMAGES.C), byte-for-byte the same shape: the only
 * difference is POLY_GT4's per-vertex colour, so all twelve r/g/b bytes are
 * written 0x7F instead of FT4's three. Layout below is PSX.SYM's own POLY_GT4
 * (reference/psxsym-types.h), which confirms every offset in the .s.
 *
 * PSX.SYM's local list for this function is `tx`, `ty` and `th`, and all
 * three reproduce the bytes: `ty` in particular is ONE variable advanced in
 * place across the four v stores, which is how the target uses the register.
 * Retail does need more than the demo's three (the four grouped field reads
 * and `tx2` are load-bearing — see FT4's header for the measurements), but
 * the earlier note here claiming the demo names were unusable was wrong.
 */

void SetupImageToPolyGT4(GsIMAGE *image, POLY_GT4 *ply, short x, short y)
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

    SetPolyGT4(ply);
    tp = TIM_PIXEL_MODE(*(u16 *)&image->pmode);
    ply->tpage = GetTPage(tp, 1, image->px, image->py);
    ply->clut = GetClut(image->cx, image->cy);
    sh = 2 - tp;
    px = image->px;
    ty = (u8)image->py;
    pw = image->pw;
    th = image->ph;
    setRGB0(ply, 0x7F, 0x7F, 0x7F);
    setRGB1(ply, 0x7F, 0x7F, 0x7F);
    setRGB2(ply, 0x7F, 0x7F, 0x7F);
    setRGB3(ply, 0x7F, 0x7F, 0x7F);
    ply->x0 = x;
    ply->y0 = y;
    ply->y1 = y;
    ply->x2 = x;
    tx = (px << sh) & ((1 << (8 - tp)) - 1);
    tw = pw << sh;
    x += tw;
    y += th;
    /* One-shot fence: byte-required (collapse measured; see cookbook). */
    do
    {
    } while (0);
    tx2 = tx + tw;
    ply->v0 = ty;
    ply->v1 = ty;
    ty += th;
    ply->x1 = x;
    ply->y2 = y;
    ply->x3 = x;
    ply->y3 = y;
    ply->u0 = tx;
    ply->u1 = tx2;
    ply->u2 = tx;
    ply->v2 = ty;
    ply->u3 = tx2;
    ply->v3 = ty;
}
