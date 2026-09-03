#include "common.h"
#include "main.exe.h"
#include "adt.h"
#include <psxsdk/libgpu.h>

void AdtFntLoad(int tx, int ty)
{
    FntLoad(tx, ty);
    AdtFnt.tx = tx;
    AdtFnt.ty = ty;
}
