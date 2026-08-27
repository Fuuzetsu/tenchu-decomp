#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct POLY_FT4 TelopP;
 * END PSX.SYM */

/*
 * FUN_800576e8 (0x800576e8, 0xa4 bytes) — computes the on-screen pixel
 * width of a SJIS string for the telop (on-screen caption/subtitle, per
 * DrawTelop/SetupTelop in this same TU) renderer: walks `str`, remapping the
 * single stray lead byte 0x92 to 0x27, folding the code into a 0-0x5F glyph
 * index (subtract 0x20, and an additional 0x40 for the upper half-width-kana
 * block >= 0xC0), and summing each character's width out of the per-glyph
 * table `FontWidth[]`. Short-circuits to a precomputed width
 * (`TelopP.u1 - TelopP.u0`, Ghidra's own field names for this
 * struct — "TelopP" per its Ghidra symbol) whenever either half of that
 * pair is nonzero, i.e. whenever a telop is already active/queued.
 *
 * `TelopP` is the complete POLY_FT4 declared by the demo's PSX.SYM.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - `code`/`idx` must be `s32`, not `u8` (Ghidra/m2c's own typing): a `u8`
 *    local re-narrowed after arithmetic (`idx -= 0x20;`) makes cc1 emit a
 *    defensive `andi 0xff` and then an UNSIGNED `sltiu` comparison; plain
 *    `s32` (the `lbu` load already zero-extends, and nothing here can drive
 *    it negative in a way that matters) keeps the raw value and the
 *    target's SIGNED `slti`.
 *  - The `idx -= 0x40` correction must be its own statement, not folded into
 *    `(idx - 0x40) + FontWidth` — fold-const combines the invariant
 *    `FontWidth - 0x40` into one loop-hoisted register (an extra temp the
 *    target doesn't have), same mechanism as GetArcData's `t + 4` split.
 *  - **The address must be computed ONCE, after BOTH conditional `idx`
 *    corrections, not per-branch.** Ghidra/m2c render `entry = idx +
 *    FontWidth;` separately in the "idx<0x20" fallthrough and again inside
 *    `if (code>=0xC0)` (matching the target's own asm, which does
 *    duplicate the `addu` in both blocks) — but writing it that way (a
 *    third, real cookbook-obvious attempt) leaves a 13-byte pure 4-register
 *    rotation (v0/a2/a3/t0 vs a2/a3/t0/t1) that no amount of statement
 *    reordering fixed; permuter-found the real shape in ~700 iterations.
 *    The target's TWO physical `addu`s are cc1 distributing ONE post-if
 *    address computation into both arms of the `idx`-adjustment ifs, not
 *    evidence of two source statements — don't trust the decompiler's
 *    per-branch duplication here.
 */

extern u8 FontWidth[];

s32 FUN_800576e8(u8 *str)
{
    s32 width;
    s32 code;
    s32 idx;
    u8 *entry;

    width = 0;
    if (TelopP.u0 != 0 || TelopP.u1 != 0)
    {
        width = TelopP.u1 - TelopP.u0;
    }
    else if (*str != 0)
    {
        do
        {
            code = *str;
            str++;
            if (code == 0x92)
            {
                code = 0x27;
            }
            idx = code;
            if (idx >= 0x20)
            {
                idx -= 0x20;
            }
            if (code >= 0xC0)
            {
                idx -= 0x40;
            }
            entry = idx + FontWidth;
            width += *entry;
        } while (*str != 0);
    }
    return width;
}
