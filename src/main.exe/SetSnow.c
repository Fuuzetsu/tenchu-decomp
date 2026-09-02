#include "common.h"
#include "main.exe.h"
#include "effect.h"

extern void DrawSnow(TEffectSlot *ef);

void SetSnow(VECTOR *pos, SVECTOR *velocity, s32 size, u8 sprite)
{
    int idx;
    int count;
    TEffectSlot *slot;
    SnowParticleType *particle;
    s16 vz;

    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
    slot->param.snow.x = pos->vx;
    particle = &slot->param.snow;
    particle->y = pos->vy;
    particle->z = pos->vz;
    particle->velocity[0] = velocity->vx;
    particle->velocity[1] = velocity->vy;
    vz = velocity->vz;
    particle->sprite = sprite;
    particle->size = size;
    particle->sample_y = particle->y - 8000;
    particle->velocity[2] = vz;
    particle->ground = GetAreaMapLevel(GlobalAreaMap, slot->param.snow.x,
                                       particle->sample_y, particle->z,
                                       AREA_LEVEL_FIRST_HIT);
    slot->proc = DrawSnow;
}
