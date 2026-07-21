#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawXF4(struct POLY_XF4 *ply);
 *     EFFECT.C:1785, 3 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $a0       struct POLY_XF4 * ply
 * END PSX.SYM */

/*
 * DrawXF4 (0x80038db4) — identical shape to DrawXG4 just above it
 * (same 0x80038dxx TU): draws the buffer's `ply` then its `tpage` via the
 * BIOS-linked DrawPrim (declared per-TU, as AdtSelect.c already does).
 * `ply` is cached across both calls and needs no
 * separate temp (cookbook's cached-pointer rule).
 */
void DrawXF4(POLY_XF4 *ply)
{
    DrawPrim(&ply->ply);
    DrawPrim(&ply->tpage);
}
