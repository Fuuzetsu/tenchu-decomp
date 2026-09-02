#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawTargetS(long x, long y, long z, long color);
 *     EFFECT.C:442, 23 src lines, frame 56 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 *  - Both radius arms submit their two diagonals directly. GCC cross-jumps
 *    their common tails, while CSE keeps the repeated center +/- radius
 *    values in the four saved registers seen in the target.
 *  - GsSortLine's real `unsigned short` priority parameter supplies the
 *    target's in-place narrowing before the first submission in each arm.
 *    The narrowed value is then reused by the second call, so no extra mask
 *    or full-width prototype exception is needed.
 */

void DrawTargetS(long x, long y, long z, long color)
{
    GsLINE line;
    long priority;

    z >>= 2;
    priority = z < 0 ? 0 :
        (z >= DEPTH_LIMIT ? DEPTH_LIMIT - 1 : z);

    line.r = (u8)(color >> 16);
    line.attribute = 0;
    line.g = (u8)(color >> 8);
    line.b = (u8)color;
    if (color < 0)
    {
        line.x0 = x - 20;
        line.y0 = y - 20;
        line.x1 = x + 20;
        line.y1 = y + 20;
        GsSortLine(&line, OTablePt, priority);
        line.x0 = x + 20;
        line.y0 = y - 20;
        line.x1 = x - 20;
        line.y1 = y + 20;
        GsSortLine(&line, OTablePt, priority);
    }
    else
    {
        line.x0 = x - 2;
        line.y0 = y - 2;
        line.x1 = x + 2;
        line.y1 = y + 2;
        GsSortLine(&line, OTablePt, priority);
        line.x0 = x + 2;
        line.y0 = y - 2;
        line.x1 = x - 2;
        line.y1 = y + 2;
        GsSortLine(&line, OTablePt, priority);
    }
}
