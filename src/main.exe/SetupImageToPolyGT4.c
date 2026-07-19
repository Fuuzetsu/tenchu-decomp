#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupImageToPolyGT4(struct GsIMAGE *image, struct POLY_GT4 *ply, short x, short y);
 *     IMAGES.C:129, 25 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 * PSX.SYM's local list for this function (tx/ty/th) is from the DEMO build —
 * retail's version was rewritten to match FT4's shape, so the FT4 locals are
 * what reproduce the bytes here.
 */
extern void SetPolyGT4(POLY_GT4 *p);

void SetupImageToPolyGT4(GsIMAGE *image, POLY_GT4 *ply, short x, short y)
{
    s32 tp;
    s32 sh;
    s32 iVar7;
    u32 u0Val;
    u16 u1Val;
    u8 v2Val;
    s32 px;
    u8 pyByte;
    u32 pw;
    u32 ph;

    SetPolyGT4(ply);
    tp = *(u16 *)&image->pmode & 3;
    ply->tpage = GetTPage(tp, 1, image->px, image->py);
    ply->clut = GetClut(image->cx, image->cy);
    sh = 2 - tp;
    px = image->px;
    pyByte = (u8)image->py;
    pw = image->pw;
    ph = image->ph;
    ply->r0 = 0x7F;
    ply->g0 = 0x7F;
    ply->b0 = 0x7F;
    ply->r1 = 0x7F;
    ply->g1 = 0x7F;
    ply->b1 = 0x7F;
    ply->r2 = 0x7F;
    ply->g2 = 0x7F;
    ply->b2 = 0x7F;
    ply->r3 = 0x7F;
    ply->g3 = 0x7F;
    ply->b3 = 0x7F;
    ply->x0 = x;
    ply->y0 = y;
    ply->y1 = y;
    ply->x2 = x;
    u0Val = (px << sh) & ((1 << (8 - tp)) - 1);
    iVar7 = pw << sh;
    x = x + iVar7;
    y = y + ph;
    do
    {
    } while (0);
    u1Val = u0Val + iVar7;
    ply->v0 = pyByte;
    ply->v1 = pyByte;
    v2Val = pyByte + ph;
    ply->x1 = x;
    ply->y2 = y;
    ply->x3 = x;
    ply->y3 = y;
    ply->u0 = u0Val;
    ply->u1 = u1Val;
    ply->u2 = u0Val;
    ply->v2 = v2Val;
    ply->u3 = u1Val;
    ply->v3 = v2Val;
}
