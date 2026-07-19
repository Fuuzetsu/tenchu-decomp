#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void PutNumber(int x, int y, int cols, int n);
 *     INFOVIEW.C:197, 17 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *    hoisted; written that way. The digit is plain `cols - q*10` int
 *    arithmetic rather than Ghidra's `(char)` casts: the store to
 *    NumberImage.u (a uchar field) truncates to one byte regardless, so
 *    GCC's standard mod-after-div lowering (reusing the quotient `q` for the
 *    remainder) falls out with no extra casts needed.
 *  - `img = &NumberImage;` must be a SEPARATE statement written AFTER the
 *    very first field access (`base = NumberImage.u;`), not before: the
 *    first access alone materializes NumberImage's address into a fresh
 *    (caller-saved) pseudo, and only the SECOND, NAMED occurrence (`img =
 *    &...`) gets CSE'd into a copy — reproducing the target's `lui/addiu` +
 *    separate `move $s1,$v0` pair. Declaring/assigning `img` FIRST instead
 *    lets cc1 target its home register directly, fusing the two into one
 *    `addiu` and costing 4 bytes (cookbook "Register allocation steering":
 *    two registers holding the same value = an explicit source copy).
 *  - The `img->x=(s16)x; img->y=(s16)y;` pair needed a `do{}while(0)` wrapper
 *    around just those two stores (permuter-found, byte-neutral scheduling
 *    lever — cookbook's do{}while(0) family) to get the entry-block
 *    instruction order right relative to the loop label.
 *  - The loop-exit test reads `cols` (just assigned from `q`), not `q`
 *    itself, even though they hold the same value — this is the register
 *    that ties out correctly (a bare 2-byte residual otherwise: `bnez a2` in
 *    target vs `bnez s0` in the `q`-tested draft).
 */
extern GsSPRITE NumberImage;
extern struct GsOT *OTablePt;


void PutNumber(int x, int y, int cols, int n)
{
    int base;
    GsSPRITE *img;
    int q;

    base = NumberImage.u;
    img = &NumberImage;
    img->w = 4;
    do
    {
        img->x = (s16)x;
        img->y = (s16)y;
    } while (0);
loop:
    q = cols / 10;
    img->u = base + (cols - q * 10) * 4;
    GsSortSprite(img, OTablePt, 0);
    img->x = img->x - 6;
    cols = q;
    if (cols != 0)
        goto loop;
    img->u = base;
}
