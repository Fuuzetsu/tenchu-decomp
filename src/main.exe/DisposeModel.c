#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DisposeModel(struct ModelType *objp);
 *     3DCTRL.C:312, 2 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelType * objp
 * END PSX.SYM */

/* DisposeModel (0x800184f8) — free a model object if non-null. Byte-identical
 * to DisposeOrnament (cloned via tools/clonematch.py). */

extern void vfree(void *p);

void DisposeModel(ModelType *objp)
{
    if (objp != 0)
    {
        vfree(objp);
    }
}
