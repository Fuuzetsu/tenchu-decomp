#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PutItemCursor(short x, short y, short size, short rotdif);
 *     INFOVIEW.C:358, 5 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 *  - The shared retail API retains the demo's recovered `x`, `y`, `size`,
 *    and `rotdif` names. Ghidra guesses all four as `short`, but its own
 *    rendering of the rotate update
 *    (`CONCAT22(in_register_0000001e,rotdif)`) is the decompiler's
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

void PutItemCursor(s16 x, s16 y, s16 size, s32 rotdif)
{
    CursorImage.x = x;
    CursorImage.y = y;
    CursorImage.scalex = size;
    CursorImage.scaley = size;
    CursorImage.rotate = CursorImage.rotate + rotdif;
    GsSortSprite(&CursorImage, OTablePt, 1);
}
