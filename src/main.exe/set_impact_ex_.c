#include "common.h"
#include "main.exe.h"
#include "effect.h"

/*
 * set_impact_ex_ (0x8003944c, 0xf8 bytes) — EFFECT.C effect-pool allocator:
 * same EffectSlot[200] round-robin search as SetSplash/SetFrame/SetBleed/
 * set_fade_ (see SetSplash.c for the full writeup of the shared idioms).
 * It fills the retail ImpactType directly from its ten arguments and hands
 * the slot to DrawImpact: position and optional parent coordinate, starting
 * and ending size/colour, angular state, duration, and sprite type. This is
 * the configurable counterpart to SetImpact's computed defaults. The name
 * is not recovered, so the address name remains rather than inventing one.
 *
 * The four trailing arguments use their promoted ABI widths. Narrow locals
 * preserve the callee's four upfront `lhu` reads while one shared prototype
 * keeps every caller's signed constants and the impact-sprite domain honest.
 *
 * The two `lw $v1,N($sp)` in the tail are `start_color`/`end_color`
 * rematerialized from their REG_EQUIV incoming slots: register pressure
 * across the search loop is high enough that neither pseudo gets a hard
 * register, so each is re-read at its single use. Both nops there are maspsx
 * load-delay artifacts of a load landing adjacent to its use, not scheduler
 * gaps.
 *
 * The store order below is plain source order, which is the whole trick. A
 * long-standing 16-byte checkpoint tried to explain retail's early
 * `lw $v1,0x14($sp)` as a *hoisted* rotate load and reached for a
 * `do{}while(0)` scheduler fence to move it. That is impossible: a load
 * carries a true dependence on EVERY preceding struct store (sched.c's
 * fixed-scalar/varying-struct dismissal does not fire for a plain sp-relative
 * `mem` against `mem/s` stores), so it can never hoist. Retail's load is early
 * because `param->end_color.word = end_color;` sits right here in source
 * order, and sched2 SANK the store — stores at distinct constant offsets
 * disambiguate freely — until reorg took it for the return delay slot.
 */
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
