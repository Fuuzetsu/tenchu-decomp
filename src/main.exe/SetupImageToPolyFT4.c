#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupImageToPolyFT4(struct GsIMAGE *image, struct POLY_FT4 *ply, short x, short y);
 *     IMAGES.C:106, 21 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 * v3/pad2) is the real PsyQ SDK layout (reference/ghidra_types.h), declared
 * locally per the SetPolyXF4.c precedent (no shared header owns it yet).
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
 *  - u0/u1 byte values are each stored to TWO fields (u0Val to u0 and u2;
 *    u1Val to u1 and u3) and the py-derived byte to v0/v1 then v2Val to
 *    v2/v3 — named locals for exactly the values reused across those
 *    non-adjacent stores, matching Ghidra's own bVar4/uVar6/uVar8 temps.
 *  - `u0Val`/`u1Val` must stay UNCAST/WIDE (u32/u16, no `(u8)` truncation
 *    on the assignment): an explicit `(u8)` on `u0Val`'s assignment forces
 *    a redundant `andi 0xff` when it's later added into `u1Val`, which the
 *    target doesn't have (the `& mask` already leaves it byte-range; a
 *    second narrowing cast makes cc1 re-mask on reuse instead of trusting
 *    the first AND).
 *  - The empty `do { } while (0);` right after the `y += ph;` update is a
 *    load-bearing REGALLOC LEVER (found by tools/permute.py, ~4600 iters,
 *    score 0): with no barrier, cc1's scheduler hoists `u1Val = u0Val +
 *    iVar7;` to float BEFORE the `x`/`y` updates (same instructions, wrong
 *    order); the loop-note barrier pins it after, matching the target.
 */
typedef struct
{
    u_long tag;
    u_char r0, g0, b0, code;
    short x0, y0;
    u_char u0, v0;
    u_short clut;
    short x1, y1;
    u_char u1, v1;
    u_short tpage;
    short x2, y2;
    u_char u2, v2;
    u_short pad1;
    short x3, y3;
    u_char u3, v3;
    u_short pad2;
} POLY_FT4;

extern void SetPolyFT4(POLY_FT4 *p);
extern u16 GetTPage(s32 tp, s32 abr, s16 x, s16 y);
extern u16 GetClut(s16 x, s16 y);

void SetupImageToPolyFT4(GsIMAGE *image, POLY_FT4 *ply, short x, short y)
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

    SetPolyFT4(ply);
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
