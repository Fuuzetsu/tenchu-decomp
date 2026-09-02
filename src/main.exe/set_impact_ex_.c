#include "common.h"
#include "main.exe.h"
#include "effect.h"

extern void DrawImpact(TEffectSlot *ef);

void set_impact_ex_(VECTOR *pos, GsCOORDINATE2 *super,
                    short start_size, short end_size,
                    long start_color, long end_color,
                    s32 rotate, s32 rotate_speed, s32 time,
                    enum impact_sprite type)
{
    int idx;
    TEffectSlot *slot;
    int count;
    ImpactType *param;
    long pz;
    u16 stored_rotation = rotate;
    u16 stored_rotate_speed = rotate_speed;
    u16 stored_time = time;
    u16 stored_type = type;

    FIND_EFFECT_SLOT(idx, count, slot, found);
found:
    slot->proc = DrawImpact;
    slot->param.impact.px = pos->vx;
    param = &slot->param.impact;
    param->py = pos->vy;
    pz = pos->vz;
    param->super = super;
    param->rotate = stored_rotation;
    param->rotate_speed = stored_rotate_speed;
    param->start_color.word = start_color;
    param->end_color.word = end_color;
    param->start_size = start_size;
    param->end_size = end_size;
    param->time = stored_time;
    param->count = 0;
    param->type = stored_type;
    param->pz = pz;
}
