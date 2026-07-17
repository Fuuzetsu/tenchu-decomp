#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* SetSnow's writes and this callback's reads prove a snow-particle
 * view of EffectParam.  It shares BloodType's storage, but not its field
 * meanings: the trailing shorts are velocity and the byte at +0x1e is a
 * sprite selector. */
typedef struct
{
    s32 x;          /* +0x00 */
    s32 y;          /* +0x04 */
    s32 z;          /* +0x08 */
    s32 ground;     /* +0x0c */
    s32 sample_y;   /* +0x10 */
    s32 size;       /* +0x14 */
    s16 velocity[3]; /* +0x18 */
    u8 sprite;       /* +0x1e */
} SnowParticleType;

typedef struct
{
    GsCOORDINATE2 locate; /* +0x00 */
    SVECTOR rotate;       /* +0x50 */
    s16 id;               /* +0x58 */
    s16 attribute;        /* +0x5a */
    SVECTOR clip;         /* +0x5c */
    s32 scale;            /* +0x64 */
    GsSPRITE sprite;      /* +0x68 */
} EffectSprite3D;

typedef struct
{
    s32 vpx;
    s32 vpy;
    s32 vpz;
    s32 vrx;
    s32 vry;
    s32 vrz;
} EffectViewInfo;

extern EffectViewInfo ViewInfo;
extern u_long *GlobalAreaMap;
extern GsOT *OTablePt;
extern EffectSprite3D *D_80097F2C[];
extern s32 abs(s32 value);
extern s32 GetAreaMapLevel(u_long *area, s32 x, s32 y, s32 z, s32 mode);
extern void GetScreenPosition(s32 x, s32 y, s32 z, SVECTOR *screen);
extern void GsSortSprite(GsSPRITE *sprite, GsOT *ot, s32 priority);

/*
 * Naming: retail added this pair after the demo build. SetSnow is called only
 * by ProcMiscSnowfall and installs this callback; this body advances exactly
 * those snow-particle fields and draws the dedicated sprite. The unused
 * SetSnow/DrawSnow pair follows every surrounding EffectSlot setter/callback.
 */
void DrawSnow(TEffectSlot *effect)
{
    SnowParticleType *particle;
    EffectSprite3D *model;
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
    s32 tbl_offset;
    s32 state;
    s32 size;
    s16 scale;
    s16 depth;
    s32 priority;

    particle = (SnowParticleType *)&effect->param;
    view_x = ViewInfo.vrx;
    view_y = ViewInfo.vry;
    view_z = ViewInfo.vrz;
    {
        s16 velocity_x;
        s16 velocity_z;
        s16 velocity_y;

        x = particle->x;
        y = particle->y;
        velocity_x = particle->velocity[0];
        z = particle->z;
        velocity_y = particle->velocity[1];
        x += velocity_x;
        y += velocity_y;
        velocity_z = particle->velocity[2];
        ground = particle->ground;
        z += velocity_z;
    }
    state = 0;

    if (ground < y)
    {
        effect->proc = 0;
        return;
    }

    delta_y = y - view_y;
    delta = x - view_x;
    if (3000 < (s32)delta_y)
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
        state = GetAreaMapLevel(GlobalAreaMap, x, particle->sample_y, z, 8);
        if (state < y)
        {
            effect->proc = 0;
            return;
        }
        particle->ground = state;
    }

    particle->x = x;
    particle->y = y;
    particle->z = z;
    tbl_offset = particle->sprite * (s32)sizeof(EffectSprite3D *);
    model = *(EffectSprite3D **)(tbl_offset + (s32)D_80097F2C);
    sprite = &model->sprite;
    size = particle->size;
    GetScreenPosition(x, y, z, &screen);
    depth = screen.vz;
    if (0x24 < depth)
    {
        scale = (s16)((size * 300) / depth) + 1;
        sprite->scaley = scale;
        sprite->scalex = scale;
        sprite->x = screen.vx;
        sprite->y = screen.vy;
        depth = (s32)((u32)(u16)screen.vz << 16) >> 18;
        if (depth < 0)
        {
            goto zero;
        }
        priority = 0x4e1;
        if (depth < 0x4e2)
        {
            priority = depth;
        }
        goto draw;
    zero:
        priority = 0;
    draw:
        GsSortSprite(sprite, OTablePt, (u16)priority);
    }
}
