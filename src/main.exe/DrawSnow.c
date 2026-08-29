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
    u32 delta_y;
    u32 delta;
    u32 offset;
    s32 state;
    s32 size;
    s16 scale;
    s16 depth;
    s16 otz;
    s32 priority;

    param = &ef->param.snow;
    view_x = ViewInfo.vrx;
    view_y = ViewInfo.vry;
    view_z = ViewInfo.vrz;
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
    state = 0;

    if (ground < y)
    {
        ef->proc = 0;
        return;
    }

    delta_y = y - view_y;
    delta = x - view_x;
    if ((s32)delta_y > 3000)
    {
        state = 1;
        offset = delta_y % 6000 - 3000;
        y = view_y + offset;
    }
    if (3000 < abs(delta))
    {
        state = 1;
        offset = delta % 6000 - 3000;
        x = view_x + offset;
    }
    delta = z - view_z;
    if (3000 < abs(delta))
    {
        state = 1;
        offset = delta % 6000 - 3000;
        z = view_z + offset;
    }

    if (state != 0)
    {
        /* Retail reuses the rewrap flag's register for the ground query
         * (a fresh local, or reusing `ground`, re-colors a pseudo --
         * measured). */
        state = GetAreaMapLevel(GlobalAreaMap, x, param->sample_y, z, 8);
        if (state < y)
        {
            ef->proc = 0;
            return;
        }
        param->ground = state;
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
        scale = (s16)((size * 300) / depth) + 1;
        sprite->scaley = scale;
        sprite->scalex = scale;
        sprite->x = screen.vx;
        sprite->y = screen.vy;
        otz = (s16)(u16)screen.vz >> 2;
        if (otz >= 0)
        {
            priority = DEPTH_LIMIT - 1;
            if (otz < DEPTH_LIMIT)
            {
                priority = otz;
            }
        }
        else
        {
            priority = 0;
        }
        GsSortSprite(sprite, OTablePt, (u16)priority);
    }
}
