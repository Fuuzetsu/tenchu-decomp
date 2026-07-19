#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawXG4(struct POLY_XG4 *ply);
 *     EFFECT.C:1808, 3 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $a0       struct POLY_XG4 * ply
 * END PSX.SYM */

/*
 * DrawXG4 (0x80038d10) — draws two GPU primitives back to back out of
 * one buffer: the primitive at +8 first, then the one at +0 (same
 * DrawPrim(u8 *prim) BIOS-linked call AdtSelect.c already declares locally
 * per this project's per-TU extern convention). arg0 is cached in a
 * callee-saved reg across both calls (a plain parameter read twice needs no
 * separate temp — see the cookbook's cached-pointer rule).
 */
void DrawXG4(u8 *arg0)
{
    DrawPrim(arg0 + 8);
    DrawPrim(arg0);
}
