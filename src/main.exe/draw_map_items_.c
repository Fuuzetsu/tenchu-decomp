#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"

/*
 * Draws the current map target and each live goshikimai owned by the camera
 * owner after rotating and scaling their X/Z coordinates through `area`.
 *
 * Matching notes:
 *  - The RequestItem candidate above is REFUTED: the demo dispatcher's
 *    (enum TRequestItem, void *) shape cannot produce this (x, z, area)
 *    signature, and retail replaced RequestItem with the per-item
 *    ReqItem* helper family (item.h) — every demo role is accounted for.
 *  - Indexing `items` with the loop counter gives cc1 the target's single
 *    natural 0x58-byte induction pointer; a separately incremented item
 *    pointer was biased to `items + 0x10` and made the function four
 *    instructions too long.
 *  - The two X call arguments need distinct temporaries even though their
 *    expressions are identical and their live ranges do not overlap. Each
 *    temporary pulls the area[2] load/shift ahead of the Y rounding branch;
 *    sharing one source variable coalesces the regions and recolors both
 *    division chains, while inlining either argument schedules that region
 *    too late.
 *  - `--expand-div` is required for the retail signed-division overflow and
 *    divide-by-zero guards.
 */

extern void DrawTargetS(s32 x, s32 y, s32 z, s32 color);

void draw_map_items_(s32 x, s32 z, s32 *area)
{
    s32 divisor;
    s32 sine;
    s32 first_draw_arg_x;
    s32 loop_draw_arg_x;
    s32 cosine;
    s32 draw_x;
    s32 draw_y;
    s32 i;

    divisor = area[0];
    if (divisor <= 0)
    {
        divisor = 1;
    }

    sine = rsin(area[1]);
    cosine = rcos(area[1]);

    draw_x = (x / divisor) * cosine + (z / divisor) * sine;
    if (draw_x < 0)
    {
        draw_x += 0xFFF;
    }
    first_draw_arg_x = (draw_x >> 12) + area[2];
    draw_y = (x / divisor) * sine - (z / divisor) * cosine;
    if (draw_y < 0)
    {
        draw_y += 0xFFF;
    }
    DrawTargetS(first_draw_arg_x, (draw_y >> 12) + area[3], 0, RGB24(200, 20, 20));

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
                draw_x += 0xFFF;
            }
            draw_y = (items[i].locate->locate.coord.t[0] / divisor) * sine -
                     (items[i].locate->locate.coord.t[2] / divisor) * cosine;
            loop_draw_arg_x = (draw_x >> 12) + area[2];
            if (draw_y < 0)
            {
                draw_y += 0xFFF;
            }
            DrawTargetS(loop_draw_arg_x, (draw_y >> 12) + area[3], 0, RGB24(20, 20, 200));
        }

        i++;
    }
}
