#ifndef TENCHU_TRIAL_DISPLAY_H
#define TENCHU_TRIAL_DISPLAY_H

#include "trial.exe.h"

/* Results pages inline the same display reset that other screens call. */
#ifndef TRIAL_DEFINE_CLEAR_DISPLAY
extern __inline__
#endif
void ClearTrialDisplay(short width, short height)
{
    RECT rect = {0, 0, width, height};
    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);
    GsInit3D();
}

#endif
