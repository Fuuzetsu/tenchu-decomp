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
    ply->tpage.code[0] = ((attrib & 3) << 5) | 0xE1000200;
}
