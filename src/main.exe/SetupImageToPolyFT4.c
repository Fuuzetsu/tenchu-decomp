#include "common.h"
#include "main.exe.h"
#include "images.h"
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

/*
 * SetupImageToPolyFT4 (0x8004eaf0, 0x120 bytes) — POLY_FT4 analogue of
 * InitSprite.c: builds a textured GPU quad primitive from the SAME GsIMAGE
 * source struct InitSprite reads (identical field offsets: pmode-narrow-cast
 * @0, px/py signed @4/6, pw/ph unsigned @8/A, cx/cy signed @0x10/0x12 — same
 * "narrow lhu view of pmode" and "byte-narrowed py" idioms), placing four
 * (x,y)/(u,v) vertices offset by the scaled pw/ph instead of one x/y/scale
 * triple. POLY_FT4 (u0/v0/clut/x1/y1/u1/v1/tpage/x2/y2/u2/v2/pad1/x3/y3/u3/
 * v3/pad2) is the real PsyQ SDK layout from libgpu.h.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `image->px`/`image->py`/`image->pw`/`image->ph` are read into NAMED
 *    TEMPS right after `sh` is computed, ALL FOUR BEFORE the r0/g0/b0/x0/
 *    y0/y1/x2 stores — Ghidra's own decompilation shows this same grouping
 *    (uVar1/uVar8/uVar2/uVar3 assigned before the stores); inlining the
 *    field reads at their later use sites instead reorders the schedule
 *    (loads no longer hoist ahead of the stores) and mis-colors 3
 *    registers even though the instruction COUNT stays identical — a
 *    "N adjacent loads with no use between them are source temps" case.
 *  - `image->py`'s BYTE-narrowed read ((u8) cast, `lbu`) is a distinct
 *    load from the earlier full `lh` read for the GetTPage argument —
 *    different machine modes don't CSE (DeleteConflict's ConflictObjects).
 *  - u0/u1 byte values are each stored to TWO fields (tx to u0 and u2;
 *    tx2 to u1 and u3) — named locals for exactly the values reused
 *    across those non-adjacent stores.
 *  - `ty` spans all four v stores: the target reuses one register ($7 in
 *    FT4) for the py-derived byte at v0/v1 and then for `ty + th` at
 *    v2/v3, so this is ONE source variable advanced in place, not two.
 *    Splitting it (we had `pyByte` and `v2Val`) is equally exact but
 *    invents a local; PSX.SYM records `tx`, `ty` and `th` and no others,
 *    which is where these three names come from.
 *  - `tx`/`tx2` must stay UNCAST/WIDE (u32/u16, no `(u8)` truncation
 *    on the assignment): an explicit `(u8)` on `tx`'s assignment forces
 *    a redundant `andi 0xff` when it's later added into `tx2`, which the
 *    target doesn't have (the `& mask` already leaves it byte-range; a
 *    second narrowing cast makes cc1 re-mask on reuse instead of trusting
 *    the first AND).
 *  - The empty `do { } while (0);` right after the `y += th;` update is a
 *    load-bearing REGALLOC LEVER (found by tools/permute.py, ~4600 iters,
 *    score 0): with no barrier, cc1's scheduler hoists `tx2 = tx + tw;`
 *    to float BEFORE the `x`/`y` updates (same instructions, wrong
 *    order); the loop-note barrier pins it after, matching the target.
 *  - `tx2` must stay a named local even though `ply->u1 = tx + tw;`
 *    twice would read better: inlined, the add sinks one slot past the
 *    v0 store (1 instruction out of place). Folding `px` into `tx` or
 *    `pw` into `tw` costs 32 lines — the four grouped field reads above
 *    are the reason.
 */

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
    tp = *(u16 *)&image->pmode & 3;
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
    /* Empty one-shot: a zero-code scheduling barrier (fence class; see cookbook). */
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
