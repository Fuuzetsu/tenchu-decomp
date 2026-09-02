#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeOrnament(struct OrnamentType *objp);
 *     3DCTRL.C:510, 3 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct OrnamentType * objp
 * END PSX.SYM */

extern void vfree(void *p);

void DisposeOrnament(OrnamentType *objp)
{
    if (objp != 0)
    {
        vfree(objp);
    }
}
