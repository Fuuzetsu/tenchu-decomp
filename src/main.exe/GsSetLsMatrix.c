#include "common.h"
#include "main.exe.h"

void GsSetLsMatrix(MATRIX *mp)
{
    SetRotMatrix(mp);
    SetTransMatrix(mp);
}
