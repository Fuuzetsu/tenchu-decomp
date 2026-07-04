#include "common.h"
#include "main.exe.h"
#include "item.h"

extern MATRIX *RotMatrix(SVECTOR *r, MATRIX *m);

void FUN_80018278(ModelType *m)
{
    RotMatrix(&m->rotate, &m->locate.coord);
    m->locate.flg = 0;
}
