#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/*
 * STATUS: MATCHING — 388 bytes / 97 instructions.
 *
 * draw_glyph_ (0x8005778c, 0x184 bytes) — draws a bitmap-font glyph: grabs
 * a POLY_GT4 from the work base (and advances it, like draw_shade_quad_'s
 * siblings), remaps the raw character code `code` to a cell index in the
 * FONT_IMAGE_ sheet (0x92 is special-cased to 0x27, then codes >=0x20/>=0xc0
 * fold down by 0x20/0x40 — a half-width-kana-style remap), copies the whole
 * FONT_IMAGE_ GsIMAGE descriptor onto the stack (one struct assignment —
 * see load_font_image_into_global.c for the identical 7-word unroll), slides
 * its px/py by the cell's (col,row) within the sheet (3 px wide, 0x10 px
 * tall cells), computes an extra y-nudge `nudge` for a handful of special
 * codes (0xc7/0xe7 get -4/-2/3, everything else in [0xc0,0xdf) or
 * [0xe0,0xff) gets 0), then calls SetupImageToPolyGT4/AddPrim exactly like
 * draw_shade_quad_'s neighbours.
 *
 * Matching notes:
 *  - `img = FONT_IMAGE_;` (a plain GsIMAGE struct assignment) is the
 *    proven load_font_image_into_global.c idiom, reused here in the other
 *    direction (global -> stack).
 *  - The `nudge` nudge is Ghidra's literal `if ((0x1f < (uint)(code -
 *    0xc0)) || (nudge = -4, code == 199)) {...}` — an `||` whose SECOND
 *    operand is a comma expression that unconditionally sets `nudge = -4`
 *    before testing `== 199`. Writing it as C's own short-circuit `||`
 *    reproduces the control flow directly: transcribe literally, don't
 *    "simplify" the comma away (the -4 must survive to the join point even
 *    on the branch where the equality test fails).
 *  - Preserve three full-width identities through the two-stage fold:
 *    `c0` is the unadjusted code, `t1` is the arithmetic working copy,
 *    and `t2` is the final result. That gives the target its visible
 *    v1 -> a0 -> a1 copy chain instead of letting CSE test the original
 *    register and fill the branch delay slot with the first copy.
 *  - Capture `code` into the `u8 narrow` before either work-base call,
 *    then widen it into `raw` afterwards. This is what keeps the raw a3 ->
 *    s0 copy in the target prologue while placing `andi s0,s0,0xff` after
 *    the calls; an in-place mask either rotates the saved-argument stores
 *    or folds the copy and mask into one instruction.
 *  - Update px before the signed row adjustment and let py's assignment
 *    perform the final narrowing. The source order and full-width shift
 *    reproduce the target's independent column arithmetic before its
 *    `bgez`, followed by `sra`/`sll` for the row.
 *  - The local `short y` truncates `y0 + nudge` after the addition.
 *    This old caller has no prototype in scope, so its `short` coordinates
 *    receive C's default integer promotions. The typed IMAGES.C API lives in
 *    images.h; keeping this legacy boundary unprototyped avoids inventing an
 *    incompatible ANSI signature merely to reproduce the promotion schedule.
 */
extern GsIMAGE FONT_IMAGE_;
/* Deleting this also gates ASM-IDENTICAL, and must not be done: this
 * file does not include images.h, so the call would fall back to an
 * implicit declaration rather than the real prototype. */
extern void SetupImageToPolyGT4();

void draw_glyph_(void *ot, short x, short y0, u32 code)
{
    u16 cell;
    POLY_GT4 *ply;
    s32 nudge;
    s32 c0;
    s32 t1;
    s32 t2;
    u32 raw;
    u8 narrow;
    GsIMAGE img;

    narrow = code;
    ply = (POLY_GT4 *)GsGetWorkBase();
    GsSetWorkBase(ply + 1);
    raw = narrow;
    c0 = raw;
    if (raw == 0x92)
    {
        c0 = 0x27;
    }
    t1 = c0;
    if (t1 > 0x1f)
    {
        t1 -= 0x20;
    }
    if (c0 > 0xbf)
    {
        t1 -= 0x40;
    }
    t2 = t1;
    cell = (u16)t2;
    img = FONT_IMAGE_;
    img.px += (cell & 0xf) * 3;
    if (t2 < 0)
    {
        t2 += 0xf;
    }
    img.pw = 3;
    img.ph = 0x10;
    img.py += (t2 >> 4) * 0x10;
    if ((0x1f < raw - 0xc0) || (nudge = -4, raw == 199))
    {
        if (raw - 0xe0 < 0x20)
        {
            nudge = -2;
            if (raw == 0xe7)
            {
                nudge = 3;
            }
        }
        else
        {
            nudge = 0;
        }
    }
    {
        short y = y0 + nudge;
        s32 y_arg = y;
        s32 x_arg = x;
        SetupImageToPolyGT4(&img, ply, x_arg, y_arg);
    }
    AddPrim(ot, ply);
}
