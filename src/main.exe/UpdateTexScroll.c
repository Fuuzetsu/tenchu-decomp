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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 *    (SetDrawMove's destination-x arg) are genuinely separate struct members, not the
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

/* PSX.SYM's parameter here is `struct TexScroll *tscr`, and that is a
 * genuine demo/retail divergence rather than a name we got wrong:
 * retail installs this as an effect-slot proc (`ef->proc =
 * UpdateTexScroll`), so it must take the slot and derive the
 * TexScroll from it. `tools/symnote.py --params` will keep
 * reporting the difference. */
void UpdateTexScroll(TEffectSlot *ef)
{
    TexScroll *tscr;
    DR_MOVE *prim;

    tscr = &ef->param.texscroll;
    tscr->px = (u32)(tscr->px + tscr->vx) %
               (u32)(tscr->image.w << TEXSCROLL_SUBPIXEL_BITS);
    tscr->py = (u32)(tscr->py + tscr->vy) %
               (u32)(tscr->image.h << TEXSCROLL_SUBPIXEL_BITS);

    tscr->image.x = tscr->sx + tscr->px / TEXSCROLL_SUBPIXEL_SCALE;
    tscr->image.y = tscr->sy + tscr->py / TEXSCROLL_SUBPIXEL_SCALE;

    prim = (DR_MOVE *)GsGetWorkBase();
    GsSetWorkBase(prim + 1);
    SetDrawMove(prim, &tscr->image, tscr->x, tscr->y);
    AddPrim(OTablePt->org, prim);
}
