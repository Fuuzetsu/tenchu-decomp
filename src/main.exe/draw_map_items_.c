#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"

extern void DrawTargetS(s32 x, s32 y, s32 z, s32 color);

void draw_map_items_(s32 x, s32 z, MapPlacementType *placement)
{
    s32 divisor;
    s32 sine;
    s32 first_draw_arg_x;
    s32 loop_draw_arg_x;
    s32 cosine;
    s32 draw_x;
    s32 draw_y;
    s32 i;

    divisor = placement->scale_divisor;
    if (divisor <= 0)
    {
        divisor = 1;
    }

    sine = rsin(placement->rotation);
    cosine = rcos(placement->rotation);

    draw_x = (x / divisor) * cosine + (z / divisor) * sine;
    if (draw_x < 0)
    {
        draw_x += FIXED_TRUNC_BIAS;
    }
    first_draw_arg_x = (draw_x >> FIXED_SHIFT) + placement->screen_x;
    draw_y = (x / divisor) * sine - (z / divisor) * cosine;
    if (draw_y < 0)
    {
        draw_y += FIXED_TRUNC_BIAS;
    }
    DrawTargetS(first_draw_arg_x,
                (draw_y >> FIXED_SHIFT) + placement->screen_y, 0,
                RGB24(200, 20, 20));

    i = 0;
    while (1)
    {
        if (i >= MAX_ITEMS)
        {
            break;
        }

        if (items[i].proc != 0 && items[i].type == ITEM_GOSHIKIMAI && items[i].owner == CamState.Owner)
        {
            draw_x = (items[i].locate->locate.coord.t[0] / divisor) * cosine +
                     (items[i].locate->locate.coord.t[2] / divisor) * sine;
            if (draw_x < 0)
            {
                draw_x += FIXED_TRUNC_BIAS;
            }
            draw_y = (items[i].locate->locate.coord.t[0] / divisor) * sine -
                     (items[i].locate->locate.coord.t[2] / divisor) * cosine;
            loop_draw_arg_x =
                (draw_x >> FIXED_SHIFT) + placement->screen_x;
            if (draw_y < 0)
            {
                draw_y += FIXED_TRUNC_BIAS;
            }
            DrawTargetS(loop_draw_arg_x,
                        (draw_y >> FIXED_SHIFT) + placement->screen_y, 0,
                        RGB24(20, 20, 200));
        }

        i++;
    }
}
