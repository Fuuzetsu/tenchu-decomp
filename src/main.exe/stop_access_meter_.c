#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

extern void VSyncCallback(void (*f)(void));

void stop_access_meter_(void)
{
    DISPENV o_disp;
    DRAWENV o_draw;
    DRAWENV n_draw;

    VSyncCallback(0);
    if (AccessPower >= 0)
    {
        AccessImage.r0 = 0;
        AccessImage.g0 = 0;
        AccessImage.b0 = 0;
        AccessImage.r1 = 0;
        AccessImage.g1 = 0;
        AccessImage.b1 = 0;
        AccessImage.r2 = 0;
        AccessImage.g2 = 0;
        AccessImage.b2 = 0;
        AccessImage.r3 = 0;
        AccessImage.g3 = 0;
        AccessImage.b3 = 0;
        GetDrawEnv(&o_draw);
        if (AccessPower != 0)
            GetDispEnv(&o_disp);
        else
            GetDispEnv(&o_disp);
        n_draw = o_draw;
        n_draw.clip = o_disp.disp;
        n_draw.ofs[0] = o_disp.disp.x;
        n_draw.ofs[1] = o_disp.disp.y;
        PutDrawEnv(&n_draw);
        DrawPrim((u8 *)&AccessImage);
        PutDrawEnv(&o_draw);
    }
}
