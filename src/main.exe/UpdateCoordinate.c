#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * UpdateCoordinate (0x80018248) — recompute a model's local->world matrix from
 * its Euler rotation, then clear the GsCOORDINATE2 dirty flag. `dim` survives
 * the RotMatrixYXZ call (callee-saved $s0) for the trailing flg store.
 */
extern MATRIX *RotMatrixYXZ(SVECTOR *r, MATRIX *m);

void UpdateCoordinate(ModelType *m)
{
    RotMatrixYXZ(&m->rotate, &m->locate.coord);
    m->locate.flg = 0;
}
