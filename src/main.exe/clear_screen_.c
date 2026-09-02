#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

extern RECT ScreenRect; /* {0,0,320,480}: both pages */

void clear_screen_(void)
{
    ClearImage(&ScreenRect, 0, 0, 0);
}
