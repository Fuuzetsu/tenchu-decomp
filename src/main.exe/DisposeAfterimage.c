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
