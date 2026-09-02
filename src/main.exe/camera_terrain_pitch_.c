#include "common.h"
#include "main.exe.h"
#include "item.h"

s32 camera_terrain_pitch_(Humanoid *human)
{
    enum
    {
        DIV = 16,
        ROTMAX = 341
    };
    AreaNodeType *node;
    s16 x;
    s16 z;
    s16 xspan;
    s16 zspan;
    s32 yy;
    s32 height0;
    s32 height1;
    s32 xshift;
    s32 zshift;
    s32 delta;
    s32 angle;

    if (human->map.area == 0)
        return 0;

    xshift = -rsin(human->rotate->vy) / DIV;
    zshift = -rcos(human->rotate->vy) / DIV;

    x = human->locate->vx / 10;
    z = human->locate->vz / 10;
    node = human->map.area;
    yy = (u16)node->y;
    x -= (u16)node->x1;
    xspan = (u16)node->x2 - (u16)node->x1 + 1;
    z -= (u16)node->z1;
    zspan = (u16)node->z2 - (u16)node->z1 + 1;

    switch (node->attribute & (MAP_SLOPE_X | MAP_SLOPE_Z))
    {
    case MAP_SLOPE_X:
        yy += x * node->dy / xspan;
        break;
    case MAP_SLOPE_Z:
        yy += z * node->dy / zspan;
        break;
    }
    height0 = (short)yy * 10;
    if (height0 == LEVEL_NONE)
        return 0;

    x = (human->locate->vx + xshift) / 10;
    z = (human->locate->vz + zshift) / 10;
    node = human->map.area;
    yy = (u16)node->y;
    x -= (u16)node->x1;
    xspan = (u16)node->x2 - (u16)node->x1 + 1;
    z -= (u16)node->z1;
    zspan = (u16)node->z2 - (u16)node->z1 + 1;

    switch (node->attribute & (MAP_SLOPE_X | MAP_SLOPE_Z))
    {
    case MAP_SLOPE_X:
        yy += x * node->dy / xspan;
        break;
    case MAP_SLOPE_Z:
        yy += z * node->dy / zspan;
        break;
    }
    height1 = (short)yy * 10;
    if (height1 == LEVEL_NONE)
        return 0;

    delta = height1 - height0;
    if (delta <= -700)
        return 0;

    angle = ratan2(delta, 256);
    if (angle > ROTMAX)
        angle = ROTMAX;
    else if (angle < -ROTMAX)
        angle = -ROTMAX;
    return angle;
}
