#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetPolyXG4(struct POLY_XG4 *ply, short attrib);
 *     EFFECT.C:1793, 5 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
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
 *     param $a1       short attrib
 * END PSX.SYM */

/*
 * SetPolyXG4 (0x80038d80) — pokes 4 fields of an unnamed (likely GPU
 * primitive / OT-tag) struct: byte 0x3 = 1 (top/length byte of the 4-byte
 * tag word at offset 0 — only the length byte is set, matching the OT-tag
 * convention of a single following word), word @0x4 = a DR_TPAGE-style
 * 0xE1000000 draw-mode command with the semi-transparency rate (arg1 & 3)
 * placed in bits 5-6, and two more bytes at 0xB/0xF (likely a second
 * primitive's fields) set to fixed constants 8 and 0x3A. No proven struct
 * name exists for this layout, so plain offset casts (per the cookbook's
 * "no proven view" rule) — same shape as the Ghidra/m2c reference.
 */
void SetPolyXG4(void *arg0, s32 arg1)
{
    *((u8 *)arg0 + 0xB) = 8;
    *((u8 *)arg0 + 0xF) = 0x3A;
    *((u8 *)arg0 + 3) = 1;
    *(u32 *)((u8 *)arg0 + 4) = (arg1 & 3) << 5 | 0xE1000200;
}
