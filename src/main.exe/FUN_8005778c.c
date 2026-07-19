#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

/*
 * STATUS: MATCHING — 388 bytes / 97 instructions.
 *
 * FUN_8005778c (0x8005778c, 0x184 bytes) — draws a bitmap-font glyph: grabs
 * a POLY_GT4 from the work base (and advances it, like FUN_80038c0c's
 * siblings), remaps the raw character code `param_4` to a cell index in the
 * FONT_IMAGE_ sheet (0x92 is special-cased to 0x27, then codes >=0x20/>=0xc0
 * fold down by 0x20/0x40 — a half-width-kana-style remap), copies the whole
 * FONT_IMAGE_ GsIMAGE descriptor onto the stack (one struct assignment —
 * see load_font_image_into_global.c for the identical 7-word unroll), slides
 * its px/py by the cell's (col,row) within the sheet (3 px wide, 0x10 px
 * tall cells), computes an extra x-nudge `sVar3` for a handful of special
 * codes (0xc7/0xe7 get -4/-2/3, everything else in [0xc0,0xdf) or
 * [0xe0,0xff) gets 0), then calls SetupImageToPolyGT4/AddPrim exactly like
 * FUN_80038c0c's neighbours.
 *
 * Matching notes:
 *  - `local_38 = FONT_IMAGE_;` (a plain GsIMAGE struct assignment) is the
 *    proven load_font_image_into_global.c idiom, reused here in the other
 *    direction (global -> stack).
 *  - The `sVar3` nudge is Ghidra's literal `if ((0x1f < (uint)(param_4 -
 *    0xc0)) || (sVar3 = -4, param_4 == 199)) {...}` — an `||` whose SECOND
 *    operand is a comma expression that unconditionally sets `sVar3 = -4`
 *    before testing `== 199`. Writing it as C's own short-circuit `||`
 *    reproduces the control flow directly: transcribe literally, don't
 *    "simplify" the comma away (the -4 must survive to the join point even
 *    on the branch where the equality test fails).
 *  - Preserve three full-width identities through the two-stage fold:
 *    `uVar4` is the unadjusted code, `t1` is the arithmetic working copy,
 *    and `t2` is the final result. That gives the target its visible
 *    v1 -> a0 -> a1 copy chain instead of letting CSE test the original
 *    register and fill the branch delay slot with the first copy.
 *  - Capture `param_4` into the `u8 narrow` before either work-base call,
 *    then widen it into `raw` afterwards. This is what keeps the raw a3 ->
 *    s0 copy in the target prologue while placing `andi s0,s0,0xff` after
 *    the calls; an in-place mask either rotates the saved-argument stores
 *    or folds the copy and mask into one instruction.
 *  - Update px before the signed row adjustment and let py's assignment
 *    perform the final narrowing. The source order and full-width shift
 *    reproduce the target's independent column arithmetic before its
 *    `bgez`, followed by `sra`/`sll` for the row.
 *  - The local `short h` truncates `param_3 + sVar3` after the addition.
 *    This TU intentionally promotes it through `h_arg` and declares the
 *    fourth extern slot `s32`; the callee's PSX.SYM definition calls that
 *    slot `short`, but old C TUs may carry a different caller declaration.
 *    The explicit short keeps the passed value identical while the wider
 *    call slot fixes the adjacent a3/a2 sign-extension schedule.
 */
extern GsIMAGE FONT_IMAGE_;
extern void *GsGetWorkBase(void);
extern void GsSetWorkBase(void *workBase);
extern void SetupImageToPolyGT4(GsIMAGE *image, void *quad, short w, s32 h);
extern void AddPrim(u8 *ot, u8 *prim);

void FUN_8005778c(void *param_1, short param_2, short param_3, u32 param_4)
{
    u16 uVar2;
    POLY_GT4 *ply;
    s32 sVar3;
    s32 uVar4;
    s32 t1;
    s32 t2;
    u32 raw;
    u8 narrow;
    GsIMAGE local_38;

    narrow = param_4;
    ply = (POLY_GT4 *)GsGetWorkBase();
    GsSetWorkBase(ply + 1);
    raw = narrow;
    uVar4 = raw;
    if (raw == 0x92)
    {
        uVar4 = 0x27;
    }
    t1 = uVar4;
    if (0x1f < t1)
    {
        t1 = t1 - 0x20;
    }
    if (0xbf < uVar4)
    {
        t1 = t1 - 0x40;
    }
    t2 = t1;
    uVar2 = (u16)t2;
    local_38 = FONT_IMAGE_;
    local_38.px = local_38.px + (uVar2 & 0xf) * 3;
    if (t2 < 0)
    {
        t2 = t2 + 0xf;
    }
    local_38.pw = 3;
    local_38.ph = 0x10;
    local_38.py = local_38.py + (t2 >> 4) * 0x10;
    if ((0x1f < raw - 0xc0) || (sVar3 = -4, raw == 199))
    {
        if (raw - 0xe0 < 0x20)
        {
            sVar3 = -2;
            if (raw == 0xe7)
            {
                sVar3 = 3;
            }
        }
        else
        {
            sVar3 = 0;
        }
    }
    {
        short h = param_3 + sVar3;
        s32 w_arg = param_2;
        s32 h_arg = h;
        SetupImageToPolyGT4(&local_38, ply, w_arg, h_arg);
    }
    AddPrim(param_1, ply);
}
