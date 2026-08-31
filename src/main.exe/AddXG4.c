#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AddXG4(void *ot, struct POLY_XG4 *ply);
 *     EFFECT.C:1803, 3 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       void * ot
 *     param $a1       struct POLY_XG4 * ply
 * END PSX.SYM */

/*
 * AddXG4 (0x80038d40) — adds the Gouraud quad and its draw-mode command to an
 * ordering table. Both `ot` and `ply` are cached in callee-saved regs across
 * the two calls (plain parameters read twice need no separate temps —
 * cookbook's cached-pointer rule).
 */
void AddXG4(void *ot, POLY_XG4 *ply)
{
    AddPrim(ot, &ply->ply);
    AddPrim(ot, &ply->tpage);
}
