#include "common.h"
#include "main.exe.h"

extern BackGround *SetupBG(GsIMAGE *image, s16 w, s16 h);

BackGround *load_background_(u_long *tim)
{
    BackGround *bg;
    s32 i;
    GsIMAGE im;

    GetTIMInfo(tim, &im);
    bg = SetupBG(&im, SCREEN_W, SCREEN_H);
    bg->sz = 100;
    LoadTIM(tim);
    i = 0;
    if (0 < bg->map.ncellw * bg->map.ncellh)
    {
        do
        {
            bg->index[i] = (u16)i;
            i++;
        } while (i < bg->map.ncellw * bg->map.ncellh);
    }
    return bg;
}
