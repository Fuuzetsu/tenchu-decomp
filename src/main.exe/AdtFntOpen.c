#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

extern AdtFntState AdtFnt;

void AdtFntOpen(int x, int y, int w, int h, int isbg, int n)
{
    FntOpen(x, y, w, h, isbg, n);
    AdtFnt.x = x;
    AdtFnt.y = y;
    AdtFnt.w = w;
    AdtFnt.h = h;
    AdtFnt.isbg = isbg;
    AdtFnt.n = n;
}
