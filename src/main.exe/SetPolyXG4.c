#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetPolyXG4(struct POLY_XG4 *ply, short attrib);
 *     EFFECT.C:1793, 5 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct POLY_XG4 * ply
 *     param $a1       short attrib
 * END PSX.SYM */

/*
 * SetPolyXG4 (0x80038d80) — initializes EFFECT.C's recovered POLY_XG4:
 * one DR_TPAGE command followed by a semi-transparent Gouraud quad. The
 * attribute's low two bits select the semi-transparency rate in the draw-mode
 * command.
 */
void SetPolyXG4(POLY_XG4 *ply, short attrib)
{
    setlen(&ply->ply, 8);
    setcode(&ply->ply, 0x3A);
    setlen(&ply->tpage, 1);
    ply->tpage.code[0] = (attrib & 3) << 5 | GPU_DRAWMODE_DITHER;
}
