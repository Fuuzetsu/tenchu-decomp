#include "common.h"
#include "main.exe.h"
#include "item.h"

/* Rebuild a model's coordinate matrix from its rotate vector and mark the
 * GsCOORDINATE2 dirty. */
/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateCoordinate2(struct ModelType *dim);
 *     3DCTRL.C:212, 4 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelType * dim
 * END PSX.SYM */

void UpdateCoordinate2(ModelType *dim)
{
    RotMatrix(&dim->rotate, &dim->locate.coord);
    dim->locate.flg = 0;
}
