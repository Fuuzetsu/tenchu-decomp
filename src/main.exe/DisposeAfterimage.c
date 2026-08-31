#include "common.h"
#include "main.exe.h"
#include "afterimage.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeAfterimage(struct AfterimageType *afi);
 *     EFFECT.C:1706, 7 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct AfterimageType * afi
 * END PSX.SYM */

/*
 * DisposeAfterimage (0x80038e58) — free an afterimage effect's two dynamic
 * buffers (`p1`@0x18, `p2`@0x1C — trail-point arrays, sized by SetupAfterimage
 * per ReqItemHappou/ReqItemLaunch) then the AfterimageType itself. Same
 * null-check-then-free shape as DisposeMotionManager (afi survives all three
 * vfree calls in a callee-saved register).
 */
extern void vfree(void *p);

void DisposeAfterimage(AfterimageType *afi)
{
    if (afi != 0)
    {
        vfree(afi->p1);
        vfree(afi->p2);
        vfree(afi);
    }
}
