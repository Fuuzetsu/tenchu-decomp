#ifndef TENCHU_ADT_H
#define TENCHU_ADT_H

#include <psxsdk/libgpu.h>

/* ADT.C's display snapshot, as recorded in the demo's PSX.SYM. */
typedef struct TAdtDisp TAdtDisp;
struct TAdtDisp
{
    DRAWENV draw;
    DISPENV disp;
    RECT rect;
    u_char backup[32768];
    POLY_F4 bg;
};

void AdtGetDisp(TAdtDisp *disp);
void AdtReleaseDisp(TAdtDisp *disp);
AdtQuietMode AdtQuiet(AdtQuietMode quiet);
extern long (*AdtPadRead)(int port);
long AdtDmyPadRead(int port);

#endif
