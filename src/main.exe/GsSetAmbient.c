#include "common.h"
#include "main.exe.h"

/* Ambient light: forward to the GTE background color, 12.4 scaled. */
void GsSetAmbient(long r, long g, long b)
{
    SetBackColor(r >> 4, g >> 4, b >> 4);
}
