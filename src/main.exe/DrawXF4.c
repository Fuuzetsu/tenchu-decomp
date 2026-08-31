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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
