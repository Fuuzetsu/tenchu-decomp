#include "common.h"
#include "main.exe.h"
#include "effect.h"

/*
 * MATCH.
 *
 * SetSnow (0x80039160, 0x134 bytes) — EFFECT.C effect-pool allocator:
 * the same EffectSlot[200] round-robin search as SetSplash/SetFrame/
 * SetBleed/set_fade_/set_impact_ex_ (see SetSplash.c for the shared idiom
 * writeup), filling the slot straight from its 4 caller-supplied parameters
 * (a raw "spawn exactly as told" setter, no randomization) and handing it
 * to DrawSnow — a DIFFERENT draw callback from DrawBlood/DrawImpact,
 * (`SetSnow(&pos, &vel, FIXED_ONE, SNOW_SPRITE_DEFAULT);`) — a
 * falling-snowflake spawner, not blood.
 *
 * SetSnow and DrawSnow jointly prove the shared SnowParticleType fields:
 * position, ground/sample height, draw size, velocity, and sprite selector.
 *
 * The name is high-confidence semantic recovery rather than a transplanted
 * demo symbol: ProcMiscSnowfall is the only caller, this function creates one
 * snow particle, and it installs DrawSnow, whose producer/consumer fields and
 * lifetime agree exactly. Both names were unused.
 *
 * No candidate name in reference/psxsym-candidates.tsv for either this
 * function or DrawSnow; not in the demo's PSX.SYM at all (EFFECT.C
 * functions this deep were apparently a retail-only addition, or the demo
 * spawned snow through a different, simpler path — ProcMiscSnowfall.c
 * itself IS in the demo per its own header, so only this helper is new).
 */
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
