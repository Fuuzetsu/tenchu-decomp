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
