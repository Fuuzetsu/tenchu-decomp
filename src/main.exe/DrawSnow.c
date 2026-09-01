#include "common.h"
#include "main.exe.h"
#include "effect.h"

extern s32 abs(s32 value);

/*
 * Naming: retail added this pair after the demo build. SetSnow is called only
 * by ProcMiscSnowfall and installs this callback; this body advances exactly
 * those snow-param fields and draws the dedicated sprite. The unused
 * SetSnow/DrawSnow pair follows every surrounding EffectSlot setter/callback.
 */
void DrawSnow(TEffectSlot *ef)
{
    SnowParticleType *param;
    Sprite3D *model;
    GsSPRITE *sprite;
    SVECTOR screen;
    s32 view_z;
    s32 view_x;
    s32 view_y;
    s32 x;
    s32 y;
    s32 z;
    s32 ground;
    /* delta widths are u32: the unsigned % SNOW_SPAN is in the bytes. */
    u32 delta_y;
    u32 delta;
    u32 offset;
    s32 wrapped;
    s32 size;
    s16 depth;
    s16 otz;
    s32 priority;

    param = &ef->param.snow;
    view_x = ViewInfo.vrx;
    view_y = ViewInfo.vry;
    view_z = ViewInfo.vrz;
    /* Field loads in this machine order: byte-required (the plain
     * x = param->x + param->velocity[0] spelling reorders; measured). */
    {
        s16 velocity_x;
        s16 velocity_z;
        s16 velocity_y;

        x = param->x;
        y = param->y;
        velocity_x = param->velocity[0];
        z = param->z;
        velocity_y = param->velocity[1];
        x += velocity_x;
        y += velocity_y;
        velocity_z = param->velocity[2];
        ground = param->ground;
        z += velocity_z;
    }
    wrapped = 0;

    if (ground < y)
    {
        ef->proc = 0;
        return;
    }

    delta_y = y - view_y;
    delta = x - view_x;
    if ((s32)delta_y > SNOW_RANGE)
    {
        wrapped = 1;
        offset = delta_y % SNOW_SPAN - SNOW_RANGE;
        y = view_y + offset;
    }
    if (SNOW_RANGE < abs(delta))
    {
        wrapped = 1;
        offset = delta % SNOW_SPAN - SNOW_RANGE;
        x = view_x + offset;
    }
    delta = z - view_z;
    if (SNOW_RANGE < abs(delta))
    {
        wrapped = 1;
        offset = delta % SNOW_SPAN - SNOW_RANGE;
        z = view_z + offset;
    }

    if (wrapped != 0)
    {
        /* Retail reuses the rewrap flag's register for the ground query
         * (a fresh local, or reusing `ground`, re-colors a pseudo --
         * measured). */
        wrapped = GetAreaMapLevel(GlobalAreaMap, x, param->sample_y, z,
                                  AREA_LEVEL_FIRST_HIT);
        if (wrapped < y)
        {
            ef->proc = 0;
            return;
        }
        param->ground = wrapped;
    }

    param->x = x;
    param->y = y;
    param->z = z;
    model = SpriteSnow[param->sprite];
    sprite = &model->sprite;
    size = param->size;
    GetScreenPosition(x, y, z, &screen);
    depth = screen.vz;
    if (depth > NEAR_DEPTH)
    {
        sprite->scalex = sprite->scaley =
            (s16)((size * PROJECTION_DISTANCE) / depth) + 1;
        sprite->x = screen.vx;
        sprite->y = screen.vy;
        otz = (s16)(u16)screen.vz >> 2;
        CLAMP_SORT_DEPTH(priority, otz);
        GsSortSprite(sprite, OTablePt, (u16)priority);
    }
}
