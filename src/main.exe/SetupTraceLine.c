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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       struct TracePoint * point
 * END PSX.SYM */

/*
 * SetupTraceLine (0x800299d0) — allocate a TraceLine, seed it from the
 * (sentinel-terminated, `pad == TRACE_POINT_END`) TracePoint array `point`:
 * walk to the last point, stamp its world position (from human->locate) and
 * its range (locate->vy / 100), then install the new TraceLine on
 * human->trace.
 * SystemOut("NO TRACE POINT") (does not return) if point is null.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - The plain `while (point->pad != TRACE_POINT_END) point++;` scan emits
 *    the target's entry guard and bottom test, with the increment in the
 *    backjump delay slot. No copied `pad` value is needed.
 *  - `point->x`/`point->z`/`point->range` are each reloaded from
 *    `human->locate` FRESH (three separate `lw human->locate`), not cached
 *    in one pointer local — matches the raw asm's three reloads.
 *  - `point->range = (s16)(human->locate->vy / 100);` is the magic-multiply
 *    constant division (automatic from plain `/100`).
 */
extern void *valloc(u32 size);
extern char msg_no_trace_point[]; /* NO TRACE POINT */

TraceLine *SetupTraceLine(Humanoid *human, TracePoint *point)
{
    TraceLine *trcl;

    if (point == 0)
    {
        SystemOut(msg_no_trace_point);
    }
    trcl = (TraceLine *)valloc(8);
    trcl->count = 0;
    trcl->index = 0;
    trcl->point = point;
    while (point->pad != TRACE_POINT_END)
    {
        point++;
    }
    point->x = human->locate->vx;
    point->z = human->locate->vz;
    point->range = (s16)(human->locate->vy / 100);
    human->trace = trcl;
    return trcl;
}
