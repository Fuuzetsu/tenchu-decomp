#include "common.h"
#include "main.exe.h"

void draw_sprite_pair_(GsSPRITE *sp1, GsSPRITE *sp2, s32 x, s32 y, s32 z, s32 size, long rotate, s32 color)
{
    SVECTOR out;
    s32 otz;
    s16 sx;
    s16 sy;
    s32 t;
    s32 pri;

    GetScreenPosition(x, y, z, &out);
    otz = out.vz;
    if (otz > NEAR_DEPTH)
    {
        sp1->scalex = sp1->scaley = sp2->scalex = sp2->scaley =
            (s16)((size * PROJECTION_DISTANCE) / otz) + 1;
        sp2->rotate = rotate;
        sp1->rotate = rotate;
        sx = out.vx;
        sp2->x = sx;
        sp1->x = sx;
        sy = out.vy;
        sp2->y = sy;
        sp1->y = sy;
        sp2->b = sp2->g = sp2->r = (u8)color;
        sp1->b = sp1->g = sp1->r = (u8)(color / 2);

        t = (s16)(u16)out.vz >> 2;
        CLAMP_SORT_DEPTH(pri, t);
        GsSortSprite(sp2, OTablePt, (u16)pri);

        t = (s16)(u16)out.vz >> 2;
        CLAMP_SORT_DEPTH(pri, t);
        GsSortSprite(sp1, OTablePt, (u16)pri);
    }
}
