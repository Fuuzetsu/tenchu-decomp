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
 * region's (u,v) accumulators by their per-frame deltas, wraps each modulo
 * its region's own width/height (<<4 for the 4-bit fixed-point scroll
 * unit), derives the wrapped whole-texel (x,y) offset added to a fixed
 * base, then re-issues a DR_MOVE primitive positioned there.
 *
 * Matching notes:
 *  - `tscr->w`/`tscr->rect.w` are DIFFERENT fields at DIFFERENT offsets
 *    (0xC vs 0x18) even though both read as the divisor's/SetDrawMove's
 *    "width" — Ghidra's `param_1 + 0x18` (div) and `param_1 + 0xc`
 *    (SetDrawMove's w arg) are genuinely separate struct members, not the
 *    same field twice; `rect` is a real embedded RECT{x,y,w,h} whose w/h
 *    (0x18/0x1A) are set by some OTHER function (not this one — this one
 *    only ever writes rect.x/rect.y) to the scroll region's extent, reused
 *    here as the wrap divisor.
 *  - The `(uint)(...) % divisor` cast is load-bearing: the divisor and sum
 *    are both promoted `short`s, which would pick signed `div` by default;
 *    only the explicit unsigned cast (Ghidra shows it) gets the real
 *    `divu`/`break 7` guard the target has (the divide-by-zero `break 7`
 *    guard is automatic codegen for any variable divisor, not written by
 *    hand). Needs `--expand-div` in this file's Build.hs entry (ASPSX's
 *    `break 7`/`break 6` guard shape, cookbook's Loops section).
 *  - `x = tscr->u; if (x < 0) x += 0xf; ... x >> 4` is plain `x / 16`: the
 *    round-toward-zero correction for signed division by a power of two is
 *    automatic codegen, not a hand-written idiom.
 *  - The final `rect.x`/`rect.y` stores read `tscr->u`/`tscr->v` back FRESH
 *    from memory (a new load) rather than reusing the just-computed
 *    remainder in a register — matches every reload-heavy sibling in this
 *    TU (DrawHinoko.c).
 */
/* The sub-struct starting right after the untouched leading 4 bytes.
 * `u` sits at ITS offset 0 — a "zero-offset member" gets written through a
 * fresh outer recast (`tscr->info.u`, folding the whole +4 into ONE
 * displacement off `tscr`'s own register), while every nonzero-offset
 * member goes through the separately-materialized `info` pointer (`p`) —
 * same mechanical split as the item-TU param-union offset-0 rule
 * (cookbook's "A param-union write to OFFSET 0 routes through a fresh
 * recast, nonzero offsets through the live pointer"), just without an
 * actual union here. */
typedef struct
{
    s16 u;     /* +0x0 */
    s16 v;     /* +0x2 */
    s16 du;    /* +0x4 */
    s16 dv;    /* +0x6 */
    s16 w;     /* +0x8 */
    s16 h;     /* +0xa */
    u16 baseX; /* +0xc */
    u16 baseY; /* +0xe */
    RECT rect; /* +0x10: x,y written here; w,h (+0x14/+0x16, i.e. tscr's
                  0x18/0x1a) are the wrap divisors, set elsewhere */
} TexScrollInfo;

typedef struct
{
    s16 pad[2];        /* +0x0, untouched here */
    TexScrollInfo info; /* +0x4 */
} TexScroll;

extern GsOT *OTablePt;

void UpdateTexScroll(TexScroll *tscr)
{
    TexScrollInfo *p;
    DR_MOVE *prim;
    s32 x, y;

    p = &tscr->info;
    tscr->info.u = (u16)((u32)(tscr->info.u + p->du) % (u32)(p->rect.w << 4));
    p->v = (u16)((u32)(p->v + p->dv) % (u32)(p->rect.h << 4));

    x = tscr->info.u;
    if (x < 0) x = x + 0xf;
    p->rect.x = p->baseX + (x >> 4);

    y = p->v;
    if (y < 0) y = y + 0xf;
    p->rect.y = p->baseY + (y >> 4);

    prim = (DR_MOVE *)GsGetWorkBase();
    GsSetWorkBase(prim + 1);
    SetDrawMove(prim, &p->rect, p->w, p->h);
    AddPrim((u8 *)OTablePt->org, (u8 *)prim);
}
