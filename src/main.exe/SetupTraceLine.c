#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct TraceLine * SetupTraceLine(struct Humanoid *human, struct TracePoint *point);
 *     HUMAN.C:488, 18 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *     param $a0       struct Humanoid * human
 *     param $a1       struct TracePoint * point
 * END PSX.SYM */

/*
 * SetupTraceLine (0x800299d0) — allocate a TraceLine, seed it from the
 * (sentinel-terminated, `pad == -1`) TracePoint array `point`: walk to the
 * last point, stamp its world position (from human->locate) and its range
 * (locate->vy / 100), then install the new TraceLine on human->trace.
 * SystemOut("NO TRACE POINT") (does not return) if point is null.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - The scan is the cookbook's "wrap-around search loop with increment in
 *    the backjump's delay slot" shape (minus the wraparound): entry-
 *    duplicated guard (`if (point->pad == -1) skip`), then a bottom-tested
 *    while-loop whose `point++` is the loop body's FIRST statement (reorg
 *    steals it into the backjump's delay slot, retargeting the branch) —
 *    `while (pad != -1) { point = point + 1; pad = point->pad; }`. The
 *    tempting Ghidra-literal rewrite (`pad = point[1].pad; point = point +
 *    1;`, reading one-ahead-then-advancing) instead compiles the read at a
 *    fixed `+0xC`-larger offset every iteration with NO net effect (same
 *    value, just phase-shifted) but costs 2 extra instructions (a
 *    `+12`/`-12` compensation pair at the loop's entry/exit) that the
 *    increment-first shape avoids entirely — the exact "increment as the
 *    FIRST body statement" idiom, just verified again on a plain (non-
 *    wraparound) scan.
 *  - `point->x`/`point->z`/`point->range` are each reloaded from
 *    `human->locate` FRESH (three separate `lw human->locate`), not cached
 *    in one pointer local — matches the raw asm's three reloads.
 *  - `point->range = (s16)(human->locate->vy / 100);` is the magic-multiply
 *    constant division (automatic from plain `/100`).
 */
extern void *valloc(u32 size);
extern char D_800116B8[]; /* "NO TRACE POINT" */

TraceLine *SetupTraceLine(Humanoid *human, TracePoint *point)
{
    TraceLine *trcl;
    s16 pad;

    if (point == 0)
    {
        SystemOut(D_800116B8);
    }
    trcl = (TraceLine *)valloc(8);
    trcl->count = 0;
    trcl->index = 0;
    trcl->point = point;
    pad = point->pad;
    while (pad != -1)
    {
        point = point + 1;
        pad = point->pad;
    }
    point->x = human->locate->vx;
    point->z = human->locate->vz;
    point->range = (s16)(human->locate->vy / 100);
    human->trace = trcl;
    return trcl;
}
