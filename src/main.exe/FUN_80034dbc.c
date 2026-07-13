#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

/* FUN_80039160's writes and this callback's reads prove a snow-particle
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
 * STATUS: NON_MATCHING — exact retail length (182 instructions / 728 bytes)
 * with 14 differing bytes.  The C is semantically complete; the residual is
 * two isolated old-GCC allocation/scheduling choices:
 *
 *  - At entry, retail keeps velocity-z in $v1 and then velocity-y in $v0;
 *    cc1 gives those two halfword loads the opposite registers and schedules
 *    their independent adds in the corresponding opposite order (8 bytes).
 *  - In the sprite-table lookup, retail builds D_80097F2C in $v0 and the
 *    byte index in $v1; cc1 assigns the address/index chain to $v1/$v0
 *    instead (6 bytes).
 *
 * `rtlguide` classifies these as local allocation plus one combine/scheduler
 * consequence; `.greg` already has the correct callee-saved coordinate map.
 * Flat or worse trials included coordinate-expression folding, statement and
 * declaration reordering, scalar/member-array aliases, block/comma/assignment
 * fences, pointer/index respellings, and disjoint-lifetime donor reuse.  The
 * latter proved the two blocks are coupled but adds a move or recolors the
 * otherwise-exact callee-saved map.  A near-final permuter run (~1,500
 * candidates) retained no full-link candidate close to this 14-byte base.
 * Keep the checkpoint pure C; no inline assembly is needed or wanted.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80034dbc", FUN_80034dbc);
#else
void FUN_80034dbc(TEffectSlot *effect)
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
        velocity_z = particle->velocity[2];
        x += velocity_x;
        z += velocity_z;
        velocity_y = particle->velocity[1];
        ground = particle->ground;
        y += velocity_y;
    }
    state = 0;

    if (ground < y)
    {
        do
        {
            effect->proc = 0;
        } while (0);
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
    model = D_80097F2C[particle->sprite];
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
#endif
