#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DrawTargetS(long x, long y, long z, long color);
 *     EFFECT.C:442, 23 src lines, frame 56 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
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
 * STATUS: NON_MATCHING — 3 of 65 instructions short (structural, not a pure
 * register tie; see below).
 *
 * DrawTargetS (0x8003250c, 0x104 bytes) — draws a target-lock crosshair (an
 * X of two GsSortLine diagonals) centered at (x,y): a 40px cross when
 * `color`'s sign bit is set, a 4px cross otherwise. `z` is scaled to an OT
 * depth (>>2, clamped to [0,0x4E1]) for both line's sort priority.
 *
 * Matching notes (read the raw `.s`, not Ghidra — its rendering of the
 * clamp is misleading here):
 *  - The clamp is a plain 3-way `if/else if/else`, NOT Ghidra's nested
 *    "default 0x4e1, override if < 0x4e2": the asm's `bltz` guards a
 *    negative-goes-to-0 arm, then `slti a2,0x4e2` + `beqz` picks between
 *    "z" (in range) and "0x4e1" (too far) — three disjoint assignments,
 *    not a default+conditional-override.
 *  - `color`'s r/g byte extraction uses a SIGNED shift (`sra`, matching a
 *    plain `long color >> N`), not Ghidra's `(uint)param_4 >> N` (which
 *    would compile to `srl`). Both truncate to the same byte once stored,
 *    but only the signed spelling reproduces the actual opcode.
 *  - The "big"/"small" corner offsets reuse x/y IN PLACE for one corner
 *    (matching PSX.SYM: x/y are the only named params, no separate
 *    "big"/"far" locals) and assign the OTHER corner into two fresh
 *    locals — never reassign x/y a second time.
 *
 * RESIDUAL: the target duplicates the FIRST GsSortLine call's argument
 * setup (`a0 = &line`, `a2 = otz & 0xffff`, `a1 = OTablePt` via lui/lw)
 * into BOTH branches of the `color < 0` if/else, interleaved with each
 * branch's own nearx/neary/x/y arithmetic — even though all three values
 * are IDENTICAL in both branches. The shared "sh" stores + `jal` after the
 * join stay merged (one copy). Writing the call once after a merged
 * if/else (this draft) OR duplicating the whole call statement in each
 * branch both get cc1 to MERGE that setup back into one shared copy
 * (cross-jump/tail-merging is insensitive to source-level duplication when
 * the generated code is byte-identical) — the target's non-merge implies
 * something in the ORIGINAL scheduling/allocation prevented the merge that
 * this draft's simpler live-range shape doesn't reproduce. Tried: single
 * merged call (shape wrong, ~127-byte diff); call duplicated per branch
 * with `short nearx,neary` (closest: 62/65 instructions, but a spurious
 * addiu-then-move "hop" through $v0/$v1 appears for nearx/neary instead of
 * the target's direct addiu into the final register); `long nearx,neary`
 * (removes the hop but drops to 58/65, worse). `tools/regalloc.py` shows a
 * clean 5-of-5 s0-s4 assignment (x,y,otz,nearx,neary) once the hop is
 * accepted — this is a structural/scheduling gap, not a same-length
 * register swap, so the permuter was not run (cookbook: it's reliable at
 * matched length, not for filling in missing instructions). Left at the
 * closest (62/65) draft below.
 */
/* GsLINE isn't in include/psxsdk/libgs.h (only the Gs types other TUs
 * needed are there); declared locally from reference/psxsym-types.h's
 * proven layout rather than editing the shared SDK header. */
typedef struct
{
    u32 attribute;  /* +0x0 */
    short x0;       /* +0x4 */
    short y0;       /* +0x6 */
    short x1;       /* +0x8 */
    short y1;       /* +0xa */
    u8 r;           /* +0xc */
    u8 g;           /* +0xd */
    u8 b;           /* +0xe */
} GsLINE;

extern GsOT *OTablePt;
extern void GsSortLine(GsLINE *p, GsOT *ot, u16 pri);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DrawTargetS", DrawTargetS);
#else /* NON_MATCHING */

void DrawTargetS(long x, long y, long z, long color)
{
    GsLINE line;
    long otz;
    short nearx, neary;

    z = z >> 2;
    if (z < 0)
    {
        otz = 0;
    }
    else if (z < 0x4e2)
    {
        otz = z;
    }
    else
    {
        otz = 0x4e1;
    }

    line.r = (u8)(color >> 16);
    line.attribute = 0;
    line.g = (u8)(color >> 8);
    line.b = (u8)color;
    if (color < 0)
    {
        nearx = x - 0x14;
        neary = y - 0x14;
        x = x + 0x14;
        y = y + 0x14;
        line.x0 = nearx;
        line.y0 = neary;
        line.x1 = x;
        line.y1 = y;
        GsSortLine(&line, OTablePt, (u16)otz);
    }
    else
    {
        nearx = x - 2;
        neary = y - 2;
        x = x + 2;
        y = y + 2;
        line.x0 = nearx;
        line.y0 = neary;
        line.x1 = x;
        line.y1 = y;
        GsSortLine(&line, OTablePt, (u16)otz);
    }
    line.x0 = x;
    line.y0 = neary;
    line.x1 = nearx;
    line.y1 = y;
    GsSortLine(&line, OTablePt, (u16)otz);
}

#endif /* NON_MATCHING */
