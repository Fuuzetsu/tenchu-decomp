#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetGore(struct VECTOR *pos, struct SVECTOR *vec, int time, long col);
 *     EFFECT.C:1161, 12 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct VECTOR * pos
 *     param $a1       struct SVECTOR * vec
 *     param $a2       int time
 *     param $a3       long col
 * END PSX.SYM */

extern void DrawImpact(TEffectSlot *ef);

void SetGore(GsCOORDINATE2 *coord, SVECTOR *local_position,
             SVECTOR *local_velocity)
{
    enum
    {
        GORE_INITIAL_SCALE = 2 * FIXED_ONE,
        GORE_TIME_SPREAD = 15,
        GORE_TIME_MIN = 10,
        GORE_INITIAL_BRIGHTNESS = 0x80,
        GORE_IMPACT_INTERVAL = 4,
        GORE_IMPACT_ROTATE_SPEED = 80,
        GORE_IMPACT_SIZE = 2 * FIXED_ONE,
        GORE_IMPACT_DURATION = 3,
        GORE_IMPACT_SPRITE = 2
    };
    VECTOR world_position;
    MATRIX local_to_world;
    SVECTOR velocity_copy;
    VECTOR world_velocity;
    VECTOR impact_position;
    long transform_flags[2];
    u32 impact_phase;

    GsGetLw(coord, &local_to_world);
    GsSetLsMatrix(&local_to_world);
    RotTrans(local_position, &world_position, transform_flags);

    {
        int gore_index;
        TEffectSlot *gore_slot;
        int gore_slots_searched;
        BloodType *gore;

        FIND_EFFECT_SLOT(gore_index, gore_slots_searched, gore_slot, gore_found);
    gore_found:
        gore = &gore_slot->param.blood;
        gore->sprite = rand() % N_AIRBORNE_BLOOD_SPRITES;
        gore->scale = GORE_INITIAL_SCALE;
        gore->rotate = (rand() % 360) * FIXED_ONE;
        gore->px = world_position.vx;
        gore->py = world_position.vy;
        gore->pz = world_position.vz;
        velocity_copy = *local_velocity;
        ApplyRotMatrix(&velocity_copy, &world_velocity);
        gore->vx = (short)world_velocity.vx;
        gore->vy = (short)world_velocity.vy;
        gore->vz = (short)world_velocity.vz;
        gore->time = rand() % GORE_TIME_SPREAD + GORE_TIME_MIN;
        gore->hint = 0;
        gore->brightness = GORE_INITIAL_BRIGHTNESS;
        gore->mode = GORE_MODE_AIRBORNE;
        impact_phase = GameClock & (GORE_IMPACT_INTERVAL - 1);
        gore_slot->proc = DrawGore;
    }

    if (impact_phase == 0)
    {
        int impact_index;
        TEffectSlot *impact_slot;
        int impact_slots_searched;
        ImpactType *impact;
        long impact_pz;
        long start_color;
        long end_color;

        start_color = COLOR_GRAY;
        end_color = COLOR_GRAY;
        impact_position.vx = local_position->vx;
        impact_position.vy = local_position->vy;
        impact_position.vz = local_position->vz;
        FIND_EFFECT_SLOT(impact_index, impact_slots_searched,
                         impact_slot, impact_found);
    impact_found:
        impact_slot->proc = DrawImpact;
        impact_slot->param.impact.px = impact_position.vx;
        impact = &impact_slot->param.impact;
        impact->py = impact_position.vy;
        impact_pz = impact_position.vz;
        impact->rotate_speed = GORE_IMPACT_ROTATE_SPEED;
        impact->start_size = GORE_IMPACT_SIZE;
        impact->end_size = GORE_IMPACT_SIZE;
        impact->time = GORE_IMPACT_DURATION;
        impact->super = coord;
        impact->rotate = 0;
        impact->start_color.word = start_color;
        impact->end_color.word = end_color;
        impact->count = 0;
        impact->type = GORE_IMPACT_SPRITE;
        impact->pz = impact_pz;
    }
}
