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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct POLY_XG4 * ply
 * END PSX.SYM */

void DrawXG4(POLY_XG4 *ply)
{
    DrawPrim(&ply->ply);
    DrawPrim(&ply->tpage);
}
