#include "common.h"
#include "trial.exe.h"

/* Clear the requested VRAM rectangle and reset the 3D transform state. */
void ClearTrialDisplay(short width, short height)
{
    RECT rect = {0, 0, width, height};
    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);
    GsInit3D();
}

