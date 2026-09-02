#include "common.h"
#include "main.exe.h"

extern MATRIX _LC;

void GS_107_OBJ_4B8(MATRIX *m)
{
    _LC = *m;
    SetColorMatrix(m);
}
