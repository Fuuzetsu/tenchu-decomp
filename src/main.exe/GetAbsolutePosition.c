#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct VECTOR * GetAbsolutePosition(struct ModelType *model, short x, short y, short z);
 *     3DCTRL.C:221, 15 src lines, frame 80 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct ModelType * model
 *     param $a1       short x
 *     param $a2       short y
 *     param $a3       short z
 *     stack sp+16     struct MATRIX mat
 *     stack sp+48     struct SVECTOR offset
 * END PSX.SYM */

extern VECTOR vector;

VECTOR *GetAbsolutePosition(ModelType *model, short x, short y, short z)
{
    MATRIX mat;
    SVECTOR offset;

    GsGetLw(&model->locate, &mat);
    GsSetLsMatrix(&mat);
    offset.vx = x;
    offset.vy = y;
    offset.vz = z;
    RotTrans(&offset, &vector, (long *)0);
    return &vector;
}
