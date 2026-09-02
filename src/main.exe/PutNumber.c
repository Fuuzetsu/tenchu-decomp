#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PutNumber(int x, int y, int cols, int n);
 *     INFOVIEW.C:197, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int x
 *     param $a1       int y
 *     param $a2       int cols
 *     param $a3       int n
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE NumberImage;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/*
 * PutNumber (0x8004c138, 0xB0 bytes) — draws a right-to-left digit strip
 * using NumberImage as a shared "digit cell" sprite: each iteration derives
 * one base-10 digit of `cols`, offsets NumberImage.u into the digit-glyph
 * strip (base `u` + digit*4), sorts the sprite, then steps NumberImage.x
 * left by 6px for the next (more significant) digit. `n` (the digit-count
 * 4th parameter PSX.SYM records) is never referenced by the body — a dead
 * parameter, so $a3 is simply never touched or saved.
 *
 * MATCH. Matching notes (all verified against the original bytes):
 *  - Ghidra renders a do-while, but the target RE-MATERIALIZES the /10 magic
 *    constant (lui/ori 0x66666667) every iteration instead of hoisting it to
 *    a preheader — a real do-while's loop notes let loop.c hoist that
 *    constant (cookbook: "division magic constants moved to the
 *    preheader"). A hand-rolled goto loop has no loop notes, so nothing is
 *    hoisted; written that way. The digit is plain `cols % 10` int
 *    arithmetic (cc1's mod-after-div reuses the quotient) rather than
 *    Ghidra's `(char)` casts: the store to
 *    NumberImage.u (a uchar field) truncates to one byte regardless, so
 *    GCC's standard mod-after-div lowering (reusing the quotient `q` for the
 *    remainder) falls out with no extra casts needed.
 *  - Configure the shared atlas's cell width before binding `img`, then read
 *    the base U coordinate through that drawing cursor. These are the two
 *    real roles behind the target's global-address materialization and saved
 *    pointer copy; the x/y stores need no scheduling wrapper.
 *  - The loop-exit test reads `cols` (just assigned from `q`), not `q`
 *    itself, even though they hold the same value — this is the register
 *    that ties out correctly (a bare 2-byte residual otherwise: `bnez a2` in
 *    target vs `bnez s0` in the `q`-tested draft).
 */

void PutNumber(int x, int y, int cols, int n)
{
    enum
    {
        NW = 4
    };
    enum
    {
        GAP = 6
    };
    int base;
    GsSPRITE *img;
    int q;

    NumberImage.w = NW;
    img = &NumberImage;
    base = img->u;
    img->x = (s16)x;
    img->y = (s16)y;
loop:
    q = cols / 10;
    img->u = base + (cols % 10) * NW;
    GsSortSprite(img, OTablePt, 0);
    img->x -= GAP;
    cols = q;
    if (cols != 0)
        goto loop;
    img->u = base;
}
