#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DestroyTraceLine(struct TraceLine *t);
 *     WORLD.C:1186, 7 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct TraceLine * t
 * END PSX.SYM */

/*
 * DestroyTraceLine (0x8003cb44) — free a trace-line's point array, then the
 * TraceLine itself (called from CreateStage). Same shape as
 * DisposeMotionManager (null-check, free a nested pointer field, then free
 * self; `t` stays live across the first vfree in a callee-saved register).
 * PSX.SYM's shared TraceLine is { short index; short count;
 * struct TracePoint *point; }; only `point` is touched here.
 */
extern void vfree(void *p);

void DestroyTraceLine(TraceLine *t)
{
    if (t != 0)
    {
        vfree(t->point);
        vfree(t);
    }
}
