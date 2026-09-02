#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

extern AdtFntState AdtFnt;

void AdtFntLoad(int tx, int ty)
{
    FntLoad(tx, ty);
    AdtFnt.tx = tx;
    AdtFnt.ty = ty;
}
