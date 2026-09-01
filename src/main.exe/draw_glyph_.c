#include "common.h"
#include "main.exe.h"
#include "font.h"
#include <psxsdk/libgpu.h>

/*
 * STATUS: MATCHING — 388 bytes / 97 instructions.
 *
 * draw_glyph_ (0x8005778c, 0x184 bytes) — draws a bitmap-font glyph: grabs
 * a POLY_GT4 from the work base (and advances it, like draw_shade_quad_'s
 * siblings), remaps the raw character code `code` to a cell index in the
 * FONT_IMAGE_ sheet (FONT_REMAP_CODE is translated first, then the printable
 * and upper blocks fold down to contiguous indices), copies the whole
 * FONT_IMAGE_ GsIMAGE descriptor onto the stack (one struct assignment —
 * see load_font_image_into_global.c for the identical 7-word unroll), slides
 * its px/py by the cell's (col,row) within the sheet, computes the glyph's
 * vertical nudge, then calls SetupImageToPolyGT4/AddPrim exactly like
 * draw_shade_quad_'s neighbours.
 *
 * Matching notes:
 *  - `img = FONT_IMAGE_;` (a plain GsIMAGE struct assignment) is the
 *    proven load_font_image_into_global.c idiom, reused here in the other
 *    direction (global -> stack).
 *  - The upper-band nudge assigns -4 before testing its exempt code, then
 *    jumps to the shared join. This keeps the assignment in the target
 *    branch delay slot without relying on a decompiler-style comma operand.
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
 *  - Compute the signed atlas row before writing pw/ph, then let py's
 *    assignment perform the final narrowing. The scheduler separates the
 *    division's sign bias from its shift around those independent stores,
 *    reproducing the target's `bgez`/`sra`/`sll` sequence.
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
    s32 row;
    u32 raw;
    u8 narrow;
    GsIMAGE img;

    narrow = code;
    ply = (POLY_GT4 *)GsGetWorkBase();
    GsSetWorkBase(ply + 1);
    raw = narrow;
    c0 = raw;
    if (raw == FONT_REMAP_CODE)
    {
        c0 = FONT_REMAP_TARGET;
    }
    t1 = c0;
    if (t1 >= FONT_PRINTABLE_FIRST)
    {
        t1 -= FONT_PRINTABLE_FIRST;
    }
    if (c0 >= FONT_UPPER_BLOCK_FIRST)
    {
        t1 -= FONT_UPPER_BLOCK_OFFSET;
    }
    t2 = t1;
    cell = (u16)t2;
    img = FONT_IMAGE_;
    img.px += (cell & (FONT_ATLAS_COLUMNS - 1)) * FONT_GLYPH_WIDTH;
    row = t2 / FONT_ATLAS_COLUMNS;
    img.pw = FONT_GLYPH_WIDTH;
    img.ph = FONT_GLYPH_HEIGHT;
    img.py += row * FONT_GLYPH_HEIGHT;
    if (raw - FONT_UPPER_BLOCK_FIRST < FONT_CODE_BLOCK_SIZE)
    {
        nudge = FONT_NUDGE_UPPER;
        if (raw != FONT_NUDGE_EXEMPT)
        {
            goto nudge_done;
        }
    }
    if (raw - FONT_EXTENDED_BLOCK_FIRST < FONT_CODE_BLOCK_SIZE)
    {
        nudge = FONT_NUDGE_EXTENDED;
        if (raw == FONT_RAISED_CODE)
        {
            nudge = FONT_NUDGE_RAISED;
        }
    }
    else
    {
        nudge = FONT_NUDGE_NONE;
    }
nudge_done:
    {
        short y = y0 + nudge;
        s32 y_arg = y;
        s32 x_arg = x;
        SetupImageToPolyGT4(&img, ply, x_arg, y_arg);
    }
    AddPrim(ot, ply);
}
