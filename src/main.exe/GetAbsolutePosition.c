#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * GetAbsolutePosition (0x800182a8) — world-space position of a local offset
 * (x, y, z) on a model. Pulls the model's local->world matrix (GsGetLw), makes
 * it the current local screen matrix (GsSetLsMatrix), then RotTrans's the local
 * offset into a shared static VECTOR (D_80097F90) whose address is returned.
 * &model->locate is model itself (locate is GsCOORDINATE2 at offset 0).
 */
extern void GsGetLw(GsCOORDINATE2 *coord, MATRIX *m);
extern void GsSetLsMatrix(MATRIX *m);
extern void RotTrans(SVECTOR *v0, VECTOR *sxy, long *flag);
extern VECTOR D_80097F90;

VECTOR *GetAbsolutePosition(ModelType *model, int x, int y, int z)
{
    MATRIX m;
    SVECTOR v;

    GsGetLw(&model->locate, &m);
    GsSetLsMatrix(&m);
    v.vx = x;
    v.vy = y;
    v.vz = z;
    RotTrans(&v, &D_80097F90, (long *)0);
    return &D_80097F90;
}
