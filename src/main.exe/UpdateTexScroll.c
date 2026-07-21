#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateTexScroll(struct TexScroll *tscr);
 *     EFFECT.C:1884, 24 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *     param $s1       struct TexScroll * tscr
 *     reg   $s0       struct DR_MOVE * prim
 * END PSX.SYM */

/*
 * UpdateTexScroll (0x80032610, 0x110 bytes) — advances a scrolling texture
 * region's (vx,vy) accumulators by their per-frame deltas, wraps each modulo
 * its region's own width/height (<<4 for the 4-bit fixed-point scroll
 * unit), derives the wrapped whole-texel (x,y) offset added to a fixed
 * base, then re-issues a DR_MOVE primitive positioned there.
 *
 * Matching notes:
 *  - `tscr->x`/`tscr->image.w` are DIFFERENT fields at DIFFERENT offsets
 *    (0xC vs 0x18) even though both read as the divisor's/SetDrawMove's
 *    "width" — Ghidra's `param_1 + 0x18` (div) and `param_1 + 0xc`
 *    (SetDrawMove's w arg) are genuinely separate struct members, not the
 *    same field twice; `image` is a real embedded RECT{x,y,w,h} whose w/h
 *    (0x18/0x1A) are set by some OTHER function (not this one — this one
 *    only ever writes image.x/image.y) to the scroll region's extent, reused
 *    here as the wrap divisor.
 *  - The `(uint)(...) % divisor` cast is load-bearing: the divisor and sum
 *    are both promoted `short`s, which would pick signed `div` by default;
 *    only the explicit unsigned cast (Ghidra shows it) gets the real
 *    `divu`/`break 7` guard the target has (the divide-by-zero `break 7`
 *    guard is automatic codegen for any variable divisor, not written by
 *    hand). Needs `--expand-div` in this file's Build.hs entry (ASPSX's
 *    `break 7`/`break 6` guard shape, cookbook's Loops section).
 *  - `x = tscr->vx; if (x < 0) x += 0xf; ... x >> 4` is plain `x / 16`: the
 *    round-toward-zero correction for signed division by a power of two is
 *    automatic codegen, not a hand-written idiom.
 *  - The final `image.x`/`image.y` stores read `tscr->vx`/`tscr->vy` back FRESH
 *    from memory (a new load) rather than reusing the just-computed
 *    remainder in a register — matches every reload-heavy sibling in this
 *    TU (DrawHinoko.c).
 */
typedef struct TexScroll
{
    s16 px;      /* +0x00 */
    s16 py;      /* +0x02 */
    s16 vx;      /* +0x04 */
    s16 vy;      /* +0x06 */
    s16 time;    /* +0x08 */
    s16 count;   /* +0x0A */
    s16 x;       /* +0x0C */
    s16 y;       /* +0x0E */
    s16 sx;      /* +0x10 */
    s16 sy;      /* +0x12 */
    RECT image;  /* +0x14 */
} TexScroll;

/* Typed view from the first field this routine updates. It keeps the
 * compiler's retail +4 base without hiding the recovered member names. */
typedef struct
{
    s16 vx;      /* +0x00 (TexScroll +0x04) */
    s16 vy;      /* +0x02 */
    s16 time;    /* +0x04 */
    s16 count;   /* +0x06 */
    s16 x;       /* +0x08 */
    s16 y;       /* +0x0A */
    s16 sx;      /* +0x0C */
    s16 sy;      /* +0x0E */
    RECT image;  /* +0x10 */
} TexScrollTail;

extern GsOT *OTablePt;

void UpdateTexScroll(TexScroll *tscr)
{
    TexScrollTail *scroll;
    DR_MOVE *prim;
    s32 x, y;

    scroll = (TexScrollTail *)&tscr->vx;
    tscr->vx = (u16)((u32)(tscr->vx + scroll->time) %
                     (u32)(scroll->image.w << 4));
    scroll->vy = (u16)((u32)(scroll->vy + scroll->count) %
                       (u32)(scroll->image.h << 4));

    x = tscr->vx;
    if (x < 0) x = x + 0xf;
    scroll->image.x = (u16)scroll->sx + (x >> 4);

    y = scroll->vy;
    if (y < 0) y = y + 0xf;
    scroll->image.y = (u16)scroll->sy + (y >> 4);

    prim = (DR_MOVE *)GsGetWorkBase();
    GsSetWorkBase(prim + 1);
    SetDrawMove(prim, &tscr->image, scroll->x, scroll->y);
    AddPrim((u8 *)OTablePt->org, (u8 *)prim);
}
