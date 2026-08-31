#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AddXF4(void *ot, struct POLY_XF4 *ply);
 *     EFFECT.C:1780, 3 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       void * ot
 *     param $a1       struct POLY_XF4 * ply
 * END PSX.SYM */

/*
 * AddXF4 (0x80038de4) — identical shape to AddXG4 two functions up in the
 * same 0x80038dxx TU: adds two GPU primitives out of one buffer to an order
 * table, `ply` then `tpage`, via AddPrim(ot, prim). `ot` and `ply` are cached in
 * callee-saved regs across both calls (cookbook's cached-pointer rule).
 */
void AddXF4(void *ot, POLY_XF4 *ply)
{
    AddPrim(ot, &ply->ply);
    AddPrim(ot, &ply->tpage);
}
