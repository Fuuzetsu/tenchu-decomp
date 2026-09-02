#include "common.h"
#include "main.exe.h"
#include <psxsdk/libgpu.h>

void draw_shade_quad_(void *ot, s8 r, s8 g, s8 b)
{
    POLY_XF4 *ply;

    ply = (POLY_XF4 *)GsGetWorkBase();
    GsSetWorkBase(ply + 6);
    setPolyF4(&ply->ply);
    setSemiTrans(&ply->ply, 1);
    setlen(&ply->tpage, GPU_PACKET_LENGTH(DR_TPAGE));
    ply->ply.x0 = -SCREEN_W / 2;
    ply->tpage.code[0] =
        GPU_DRAWMODE_BLEND(GPU_BLEND_SUBTRACT) | GPU_DRAWMODE_DITHER;
    ply->ply.y0 = -SCREEN_H / 2;
    ply->ply.y1 = -SCREEN_H / 2;
    ply->ply.x1 = SCREEN_W / 2;
    ply->ply.x2 = -SCREEN_W / 2;
    ply->ply.y2 = SCREEN_H / 2;
    ply->ply.x3 = SCREEN_W / 2;
    ply->ply.y3 = SCREEN_H / 2;
    ply->ply.r0 = r;
    ply->ply.g0 = g;
    ply->ply.b0 = b;
    AddPrim(ot, &ply->ply);
    AddPrim(ot, &ply->tpage);
}
