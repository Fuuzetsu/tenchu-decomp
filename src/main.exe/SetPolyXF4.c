#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetPolyXF4(struct POLY_XF4 *ply, short attrib);
 *     EFFECT.C:1770, 5 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct POLY_XF4 * ply
 *     param $a1       short attrib
 * END PSX.SYM */

/*
 * SetPolyXF4 (0x80038e24) — initializes EFFECT.C's recovered POLY_XF4: one
 * DR_TPAGE command followed by a semi-transparent flat quad.
 *
 *  - `ply->ply.tag`'s top byte (offset+3) is the PsyQ `setlen` length field,
 *    set to 5; `.code` is the primitive code byte, set to '*' (0x2A).
 *  - `ply->tpage.tag`'s top byte set to 1 (setlen), and `.code[0]` packed as
 *    `((attrib & 3) << 5) | 0xE1000200` — a DR_TPAGE-style mode word (0xE1
 *    = draw-mode GPU command, with the low tpage bits ORed in).
 */
void SetPolyXF4(POLY_XF4 *ply, short attrib)
{
    setlen(&ply->ply, 5);
    setcode(&ply->ply, 0x2A);
    setlen(&ply->tpage, 1);
    ply->tpage.code[0] = ((attrib & 3) << 5) | GPU_DRAWMODE_DITHER;
}
