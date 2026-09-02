#include "common.h"
#include "main.exe.h"
#include "images.h"
#include <psxsdk/libgpu.h>

extern char path_demo_loading_tim[];  /* K:\\WORK\\CDIMAGE\\DEMO\\loading.tim */
extern char path_demo_load_ten_tim[]; /* K:\\WORK\\CDIMAGE\\DEMO\\load_ten.tim */

void draw_loading_splash_(void)
{
    u_long *tim;
    GsIMAGE img;
    POLY_FT4 poly1;
    POLY_FT4 poly2;
    DISPENV disp;
    DRAWENV draw;
    DRAWENV draw2;

    tim = FileRead(path_demo_loading_tim);
    GetTIMInfo(tim, &img);
    LoadTIMAndFree(tim);
    SetupImageToPolyFT4(&img, &poly1, 0xD4, 0xDE);
    tim = FileRead(path_demo_load_ten_tim);
    GetTIMInfo(tim, &img);
    LoadTIMAndFree(tim);
    SetupImageToPolyFT4(&img, &poly2, 0xD4, 0xC0);
    GetDrawEnv(&draw);
    GetDispEnv(&disp);
    draw2 = draw;
    draw2.clip = disp.disp;
    draw2.ofs[0] = disp.disp.x;
    draw2.ofs[1] = disp.disp.y;
    PutDrawEnv(&draw2);
    DrawPrim((u8 *)&poly1);
    DrawPrim((u8 *)&poly2);
    DrawSync(0);
    PutDrawEnv(&draw);
}
