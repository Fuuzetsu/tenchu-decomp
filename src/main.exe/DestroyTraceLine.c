#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DestroyTraceLine(struct TraceLine *t);
 *     WORLD.C:1186, 7 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct TraceLine * t
 * END PSX.SYM */

/*
 * DestroyTraceLine (0x8003cb44) — free a trace-line's point array, then the
 * TraceLine itself (called from CreateStage). Same shape as
 * DisposeMotionManager (null-check, free a nested pointer field, then free
 * self; `t` stays live across the first vfree in a callee-saved register).
 * TraceLine (Ghidra: { short index; short count; struct TracePoint *point; })
 * isn't shared with any other matched function yet, so it's defined locally
 * here (same precedent as NodeIndexType/AreaNodeType being redefined
 * per-file rather than centralized) — only the `point` field (0x4) is
 * touched, so the pointee (TracePoint) stays opaque (void *).
 */
typedef struct
{
    s16 index;   /* 0x0 */
    s16 count;   /* 0x2 */
    void *point; /* 0x4 */
} TraceLine;

extern void vfree(void *p);

void DestroyTraceLine(TraceLine *t)
{
    if (t != 0)
    {
        vfree(t->point);
        vfree(t);
    }
}
