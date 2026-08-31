#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateCoordinate(struct ModelType *dim);
 *     3DCTRL.C:203, 4 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelType * dim
 * END PSX.SYM */

/*
 * UpdateCoordinate (0x80018248) — recompute a model's local->world matrix from
 * its Euler rotation, then clear the GsCOORDINATE2 dirty flag. `dim` survives
 * the RotMatrixYXZ call (callee-saved $s0) for the trailing flg store.
 */

void UpdateCoordinate(ModelType *dim)
{
    RotMatrixYXZ(&dim->rotate, &dim->locate.coord);
    dim->locate.flg = 0;
}
