#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * Derive the camera's terrain-following pitch from the Humanoid's current
 * AreaNode and a point 0x100 units ahead along its facing direction.  Each
 * point is projected onto the node's flat/X-slope/Z-slope plane, the height
 * difference becomes ratan2(dy, 0x100), and the result is clamped to
 * +/-0x155.  A missing area, invalid height, or a drop below -699 returns 0.
 *
 * Matching notes:
 *  - x/z and their spans are s16 working values.  Keeping them wide and only
 *    casting at the divisions changes local allocation throughout both
 *    duplicated plane evaluations.
 *  - yy deliberately stays wide after the node's unsigned-y load.  Narrowing
 *    it only in `(short)yy * 10` lets both slope arms cross-jump into the same
 *    tail and keeps the target's a3 -> v0 handoff.
 *  - The final clamp assigns one shared `angle` and returns once; three direct
 *    returns produce a different final basic-block layout.
 */

typedef struct
{
    u16 y;
    s16 dy;
    u16 x1;
    u16 z1;
    u16 x2;
    u16 z2;
    s16 attribute;
    s16 division;
} CameraAreaNode;

extern s32 rsin(s32 angle);
extern s32 rcos(s32 angle);
extern s32 ratan2(s32 y, s32 x);

s32 FUN_8002fd9c(Humanoid *human)
{
    CameraAreaNode *node;
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

    if (*(CameraAreaNode **)((u8 *)human + 0x30) == 0)
        return 0;

    xshift = -rsin(human->rotate->vy) / 16;
    zshift = -rcos(human->rotate->vy) / 16;

    x = human->locate->vx / 10;
    z = human->locate->vz / 10;
    node = *(CameraAreaNode **)((u8 *)human + 0x30);
    yy = node->y;
    x = x - node->x1;
    xspan = node->x2 - node->x1 + 1;
    z = z - node->z1;
    zspan = node->z2 - node->z1 + 1;

    if ((node->attribute & 0xc000) == 0x4000)
        goto first_x_slope;
    if ((node->attribute & 0xc000) == 0x8000)
        goto first_z_slope;
    goto first_done;

first_x_slope:
    yy = yy + x * node->dy / xspan;
    goto first_done;
first_z_slope:
    yy = yy + z * node->dy / zspan;
first_done:
    height0 = (short)yy * 10;
    if (height0 == (s32)0x80000000)
        return 0;

    x = (human->locate->vx + xshift) / 10;
    z = (human->locate->vz + zshift) / 10;
    node = *(CameraAreaNode **)((u8 *)human + 0x30);
    yy = node->y;
    x = x - node->x1;
    xspan = node->x2 - node->x1 + 1;
    z = z - node->z1;
    zspan = node->z2 - node->z1 + 1;

    if ((node->attribute & 0xc000) == 0x4000)
        goto second_x_slope;
    if ((node->attribute & 0xc000) == 0x8000)
        goto second_z_slope;
    goto second_done;

second_x_slope:
    yy = yy + x * node->dy / xspan;
    goto second_done;
second_z_slope:
    yy = yy + z * node->dy / zspan;
second_done:
    height1 = (short)yy * 10;
    if (height1 == (s32)0x80000000)
        return 0;

    delta = height1 - height0;
    if (delta < -699)
        return 0;

    angle = ratan2(delta, 0x100);
    if (angle > 0x155)
        angle = 0x155;
    else if (angle < -0x155)
        angle = -0x155;
    return angle;
}
