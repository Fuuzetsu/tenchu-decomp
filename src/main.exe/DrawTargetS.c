#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawTargetS(long x, long y, long z, long color);
 *     EFFECT.C:442, 23 src lines, frame 56 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s0       long x
 *     param $s1       long y
 *     param $s3       long z
 *     param $a3       long color
 *     stack sp+16     struct GsLINE line
 *     reg   $a2       int z
 * END PSX.SYM */

/*
 * DrawTargetS (0x8003250c, 0x104 bytes) — draws a target-lock crosshair (an
 * X of two GsSortLine diagonals) centered at (x,y): a 40px cross when
 * `color`'s sign bit is set, a 4px cross otherwise. `z` is scaled to an OT
 * depth (>>2, clamped to [0,0x4E1]) for both line's sort priority.
 *
 * STATUS: MATCHING — exact 260-byte / 65-instruction pure C with the target
 * 0x38 frame and line at sp+0x10.
 *
 * Matching notes:
 *  - The clamp is the reversed nested conditional expression below. Compared
 *    with a three-arm if/else ladder, `z >= 0x4e2` preserves the target's
 *    explicit in-range jump and negative-zero island instead of folding zero
 *    into the first branch's delay slot.
 *  - `color`'s r/g byte extraction uses a SIGNED shift (`sra`, matching a
 *    plain `long color >> N`), not Ghidra's `(uint)param_4 >> N` (which
 *    would compile to `srl`). Both truncate to the same byte once stored,
 *    but only the signed spelling reproduces the actual opcode.
 *  - `p`, `ot`, and `callpri` are branch-local call-argument carriers. They
 *    put `&line`, the in-place u16 narrowing, and OTablePt into a0/a2/a1 in
 *    both color arms while leaving the four line-coordinate stores and first
 *    jal shared after the join. Long nearx/neary locals avoid the short
 *    addiu-then-move hops; x/y still update in place as PSX.SYM suggests.
 *  - The nested one-shot loops in the small arm emit no runtime branches.
 *    Their loop notes give old cc1 the exact global-allocation priority
 *    window: x=14222, y=14166, neary=10909, otz=7317, nearx=4000, producing
 *    target homes s0/s1/s2/s3/s4 respectively.
 *  - The call-site declaration takes a full-width priority because both arms
 *    already narrow otz in place. This preserves the target's plain `move
 *    a2,s3` at the second call instead of inserting a redundant mask.
 */
extern GsOT *OTablePt;
extern void GsSortLine(GsLINE *p, GsOT *ot, long pri);

void DrawTargetS(long x, long y, long z, long color)
{
    GsLINE line;
    GsLINE *p;
    GsOT *ot;
    long otz;
    long nearx, neary;
    long callpri;

    z = z >> 2;
    otz = z < 0 ? 0 : (z >= 0x4e2 ? 0x4e1 : z);

    line.r = (u8)(color >> 16);
    line.attribute = 0;
    line.g = (u8)(color >> 8);
    line.b = (u8)color;
    if (color < 0)
    {
        p = &line;
        otz = (u16)otz;
        callpri = otz;
        nearx = x - 0x14;
        neary = y - 0x14;
        x = x + 0x14;
        ot = OTablePt;
        y = y + 0x14;
    }
    else
    {
        p = &line;
        otz = (u16)otz;
        callpri = otz;
        do
        {
            nearx = x - 2;
            do
            {
                do
                {
                    do
                    {
                        neary = y - 2;
                    } while (0);
                } while (0);
            } while (0);
            do
            {
                do
                {
                    x = x + 2;
                } while (0);
            } while (0);
        } while (0);
        ot = OTablePt;
        do
        {
            do
            {
                y = y + 2;
            } while (0);
        } while (0);
    }
    line.x0 = nearx;
    line.y0 = neary;
    line.x1 = x;
    line.y1 = y;
    GsSortLine(p, ot, callpri);
    line.x0 = x;
    line.y0 = neary;
    line.x1 = nearx;
    line.y1 = y;
    GsSortLine(&line, OTablePt, otz);
}
