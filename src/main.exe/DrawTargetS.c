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
 *  - `line_ptr`, `ordering_table`, and `priority` are branch-local
 *    call-argument carriers. They put `&line`, the in-place u16 narrowing,
 *    and OTablePt into a0/a2/a1 in both color arms while leaving the four
 *    line-coordinate stores and first jal shared after the join. Long
 *    nearx/neary locals avoid the short addiu-then-move hops; x/y still
 *    update in place as PSX.SYM suggests.
 *  - The sign-staged edge forms in both arms (`x = -x; x -= r; x = -x;`
 *    and `neary = r - y; neary = -neary;`) are allocation staging, not
 *    recovered arithmetic: flow counts their mentions, combine folds each
 *    chain back to the single addiu, and the counted refs give old cc1 the
 *    exact global-allocation priority order (x 17/45=15111, y 17/48=14166,
 *    neary 8/22=10909, otz 10/41=7317, nearx 4/25=3200 -> s0/s1/s2/s3/s4).
 *    They replaced an equivalent set of do{}while(0) weight cages
 *    (2026-08-31, joint with Codex); value-identical for the on-screen
 *    coordinate domain.
 *  - The call-site declaration takes a full-width priority because both arms
 *    already narrow otz in place. This preserves the target's plain `move
 *    a2,s3` at the second call instead of inserting a redundant mask.
 */
extern void GsSortLine(GsLINE *p, GsOT *ot, long pri);

void DrawTargetS(long x, long y, long z, long color)
{
    GsLINE line;
    GsLINE *line_ptr;
    GsOT *ordering_table;
    long otz;
    long nearx, neary;
    long priority;

    z = z >> 2;
    otz = z < 0 ? 0 : (z >= DEPTH_LIMIT ? DEPTH_LIMIT - 1 : z);

    line.r = (u8)(color >> 16);
    line.attribute = 0;
    line.g = (u8)(color >> 8);
    line.b = (u8)color;
    /* Sign-staged edges: allocation staging that combine folds back to
     * plain adds -- see the header. */
    if (color < 0)
    {
        line_ptr = &line;
        otz = (u16)otz;
        priority = otz;
        nearx = x - 20;
        neary = 20 - y;
        neary = -neary;
        x = -x;
        x -= 20;
        x = -x;
        ordering_table = OTablePt;
        y = -y;
        y -= 20;
        y = -y;
    }
    else
    {
        line_ptr = &line;
        otz = (u16)otz;
        priority = otz;
        nearx = x - 2;
        neary = 2 - y;
        neary = -neary;
        x = -x;
        x -= 2;
        x = -x;
        ordering_table = OTablePt;
        y = -y;
        y -= 2;
        y = -y;
    }
    line.x0 = nearx;
    line.y0 = neary;
    line.x1 = x;
    line.y1 = y;
    GsSortLine(line_ptr, ordering_table, priority);
    line.x0 = x;
    line.y0 = neary;
    line.x1 = nearx;
    line.y1 = y;
    GsSortLine(&line, OTablePt, otz);
}
