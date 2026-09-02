#include "common.h"
#include "main.exe.h"

void draw_sprite_coord_(GsSPRITE *sp, s32 x, s32 y, s32 z, s32 size, GsCOORDINATE2 *coord, short zbias)
{
    SVECTOR scr;
    s32 otz;
    s32 t;
    s32 pri;

    if (coord != 0)
    {
        SVECTOR *sv = SCREEN_PROJECTION_POINT;
        setVector(sv, x, y, z);
        GsGetLs(coord, SCREEN_PROJECTION_MATRIX);
        GsSetLsMatrix(SCREEN_PROJECTION_MATRIX);
        scr.vz = (s16)RotTransPers(
            sv, (s32 *)&scr, SCREEN_PROJECTION_PERSPECTIVE,
            SCREEN_PROJECTION_FLAG);
    }
    else
    {
        GetScreenPosition(x, y, z, &scr);
    }
    otz = scr.vz;
    if (otz > NEAR_DEPTH)
    {
        sp->scalex = sp->scaley =
            (s16)((size * PROJECTION_DISTANCE) / otz) + 1;
        sp->x = scr.vx;
        sp->y = scr.vy;
        t = (scr.vz + (s32)zbias) >> 2;
        CLAMP_SORT_DEPTH(pri, t);
        GsSortSprite(sp, OTablePt, (u16)pri);
    }
}
