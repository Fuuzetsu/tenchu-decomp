#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PutItemCursor(short x, short y, short size, short rotdif);
 *     INFOVIEW.C:358, 5 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     param $a0       short x
 *     param $a1       short y
 *     param $a2       short size
 *     param $a3       short rotdif
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE CursorImage;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * PutItemCursor (0x8004c094, 0x50 bytes) — position/scale the shop item
 * cursor sprite, bump its rotation by `rotdif`, and sort it into the GsOT.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - The caller's own extern (BriefingAndInventorySelectionScreen.c)
 *    declares all 4 params `int`; Ghidra guesses `short x,y,size,rotdif`
 *    for the defining TU here — but Ghidra's own rendering of the rotate
 *    update (`CONCAT22(in_register_0000001e,rotdif)`) is the decompiler's
 *    tell that its "short rotdif" guess is wrong: CONCAT22 reassembles a
 *    register Ghidra typed too narrow back to its real 32-bit width. The
 *    raw asm confirms: `rotdif` (a3) feeds `addu` against the full 32-bit
 *    `.rotate` with NO sign-extend, so it's `s32`; m2c's per-register
 *    typing (`s16,s16,s16,s32`) already had this right — x/y/size stay
 *    `short` (only ever truncating-stored via `sh`, which needs no widen
 *    either way, unlike a short forwarded as a callee's register argument
 *    — see SetLightning.c).
 *  - OTablePt is ABSOLUTE in this TU (lui+lw), unlike DrawBG.c's %gp_rel —
 *    gp-vs-absolute for the same extern is a per-TU property (gpsyms.py
 *    found no %gp_rel operand here, so this file is NOT added to the gp
 *    lists).
 */
extern GsSPRITE CursorImage;
extern GsOT *OTablePt;

void PutItemCursor(short x, short y, short size, s32 rotdif)
{
    CursorImage.x = x;
    CursorImage.scalex = size;
    CursorImage.scaley = size;
    CursorImage.y = y;
    CursorImage.rotate = CursorImage.rotate + rotdif;
    GsSortSprite(&CursorImage, OTablePt, 1);
}
