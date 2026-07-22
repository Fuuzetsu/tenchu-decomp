#include "common.h"
#include "main.exe.h"
#include "effect.h"
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
 * region's (px,py) accumulators by its (vx,vy) per-frame deltas, wraps each
 * modulo its region's own width/height (<<4 for the 4-bit fixed-point scroll
 * unit), derives the wrapped whole-texel (x,y) offset added to a fixed base,
 * then re-issues a DR_MOVE primitive positioned there.
 *
 * Matching notes:
 *  - The demo accepted a standalone TexScroll pointer. Retail installs this
 *    routine as an EffectSlot callback instead and embeds a shortened record
 *    without the demo's time/count fields. SetupTexScroll is the matching typed
 *    producer.
 *  - `tscr->x`/`tscr->image.w` are DIFFERENT fields at DIFFERENT offsets
 *    (0xC vs 0x18) even though both read as the divisor's/SetDrawMove's
 *    "width" — Ghidra's `param_1 + 0x18` (div) and `param_1 + 0xc`
 *    (SetDrawMove's w arg) are genuinely separate struct members, not the
 *    same field twice; `image` is a real embedded RECT{x,y,w,h} whose w/h
 *    (0x18/0x1A in the composite slot) are set by SetupTexScroll to the scroll
 *    region's extent and reused here as the wrap divisor; this routine only
 *    changes image.x/image.y.
 *  - The `(uint)(...) % divisor` cast is load-bearing: the divisor and sum
 *    are both promoted `short`s, which would pick signed `div` by default;
 *    only the explicit unsigned cast (Ghidra shows it) gets the real
 *    `divu`/`break 7` guard the target has (the divide-by-zero `break 7`
 *    guard is automatic codegen for any variable divisor, not written by
 *    hand). Needs `--expand-div` in this file's Build.hs entry (ASPSX's
 *    `break 7`/`break 6` guard shape, cookbook's Loops section).
 *  - `x = tscr->px; if (x < 0) x += 0xf; ... x >> 4` is plain `x / 16`: the
 *    round-toward-zero correction for signed division by a power of two is
 *    automatic codegen, not a hand-written idiom.
 *  - The final `image.x`/`image.y` stores read `tscr->px`/`tscr->py` back FRESH
 *    from memory (a new load) rather than reusing the just-computed
 *    remainder in a register — matches every reload-heavy sibling in this
 *    TU (DrawHinoko.c).
 */
extern GsOT *OTablePt;

void UpdateTexScroll(TEffectSlot *ef)
{
    TexScroll *tscr;
    DR_MOVE *prim;
    s32 x, y;

    tscr = &ef->param.texscroll;
    tscr->px = (u16)((u32)(tscr->px + tscr->vx) %
                     (u32)(tscr->image.w << 4));
    tscr->py = (u16)((u32)(tscr->py + tscr->vy) %
                     (u32)(tscr->image.h << 4));

    x = tscr->px;
    if (x < 0) x = x + 0xf;
    tscr->image.x = (u16)tscr->sx + (x >> 4);

    y = tscr->py;
    if (y < 0) y = y + 0xf;
    tscr->image.y = (u16)tscr->sy + (y >> 4);

    prim = (DR_MOVE *)GsGetWorkBase();
    GsSetWorkBase(prim + 1);
    SetDrawMove(prim, &tscr->image, tscr->x, tscr->y);
    AddPrim((u8 *)OTablePt->org, (u8 *)prim);
}
