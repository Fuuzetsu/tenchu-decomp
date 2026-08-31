#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "effect.h"

typedef struct
{
    VECTOR world_velocity;
    VECTOR impact_position;
} SetGoreScratch;

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

/*
 * MATCH. This is the retail form of EFFECT.C's SetGore. It converts a
 * model-space position and velocity into a blood/gore effect, then emits a
 * larger impact particle every fourth frame. Its ABI and allocation logic
 * are a retail redesign; the source identity is established by adjacency and
 * by installing DrawGore as the effect callback.
 *
 * The two EffectSlot searches intentionally use distinct scoped locals.  The
 * first cursor coalesces with the BloodType pointer in $s0; keeping one cursor
 * variable live through both searches rotates nearly every scan register.
 * `scratch` is genuinely a two-VECTOR workspace: `world_velocity` receives
 * the rotated gore velocity, while `impact_position` holds the three signed
 * position captures at sp+0x58..0x60 during the second pool search.
 * Independent scalar captures stay in registers, shorten the function, and
 * lose the target's 0x88-byte frame.  Finally, naming `impact_pz` immediately
 * after the py store preserves the target's early load and late pz store.
 */
void SetGore(GsCOORDINATE2 *coord, SVECTOR *local_position,
             SVECTOR *local_velocity)
{
    enum
    {
        AIRBORNE_GORE_SPRITE_COUNT = 2,
        GORE_INITIAL_SCALE = 2 * FIXED_ONE,
        GORE_TIME_SPREAD = 15,
        GORE_TIME_MIN = 10,
        GORE_INITIAL_BRIGHTNESS = 0x80,
        GORE_MODE_AIRBORNE = 0,
        GORE_IMPACT_INTERVAL = 4,
        GORE_IMPACT_ROTATE_SPEED = 80,
        GORE_IMPACT_SIZE = 2 * FIXED_ONE,
        GORE_IMPACT_DURATION = 3,
        GORE_IMPACT_SPRITE = 2
    };
    VECTOR world_position;
    MATRIX local_to_world;
    SVECTOR velocity_copy;
    SetGoreScratch scratch;
    long transform_flags[2];
    u32 impact_phase;

    GsGetLw(coord, &local_to_world);
    GsSetLsMatrix(&local_to_world);
    RotTrans(local_position, &world_position, transform_flags);

    {
        int gore_index;
        TEffectSlot *gore_pool;
        TEffectSlot *gore_slot;
        int gore_slots_searched;
        TEffectSlot *gore_effect;
        BloodType *gore;

        gore_slots_searched = 0;
        gore_pool = EffectSlot;
        gore_index = EFFECT_CURSOR_;
        gore_slot = gore_pool + gore_index;
        do
        {
            gore_index++;
            gore_slot++;
            if (gore_index > N_EFFECT_SLOTS - 1)
            {
                gore_slot = gore_pool;
                gore_index = 0;
            }
            if (gore_slot->proc == 0)
            {
                EFFECT_CURSOR_ = gore_index + 1;
                if (N_EFFECT_SLOTS - 1 < gore_index + 1)
                {
                    EFFECT_CURSOR_ = 0;
                }
                gore_effect = gore_slot;
                goto gore_found;
            }
            gore_slots_searched++;
        } while (gore_slots_searched < N_EFFECT_SLOTS);
        gore_effect = &dmy;
    gore_found:
        gore = &gore_effect->param.blood;
        gore->sprite = rand() % AIRBORNE_GORE_SPRITE_COUNT;
        gore->scale = GORE_INITIAL_SCALE;
        gore->rotate = (rand() % 360) * FIXED_ONE;
        gore->px = world_position.vx;
        gore->py = world_position.vy;
        gore->pz = world_position.vz;
        velocity_copy = *local_velocity;
        ApplyRotMatrix(&velocity_copy, &scratch.world_velocity);
        gore->vx = (short)scratch.world_velocity.vx;
        gore->vy = (short)scratch.world_velocity.vy;
        gore->vz = (short)scratch.world_velocity.vz;
        gore->time = rand() % GORE_TIME_SPREAD + GORE_TIME_MIN;
        gore->hint = 0;
        gore->brightness = GORE_INITIAL_BRIGHTNESS;
        gore->mode = GORE_MODE_AIRBORNE;
        impact_phase = GameClock & (GORE_IMPACT_INTERVAL - 1);
        gore_effect->proc = (void (*)())DrawGore;
    }

    if (impact_phase == 0)
    {
        int impact_index;
        TEffectSlot *impact_pool;
        TEffectSlot *impact_slot;
        TEffectSlot *impact_effect;
        int impact_slots_searched;
        ImpactType *impact;
        long impact_pz;
        long start_color;
        long end_color;

        start_color = COLOR_GRAY;
        end_color = COLOR_GRAY;
        scratch.impact_position.vx = local_position->vx;
        scratch.impact_position.vy = local_position->vy;
        impact_slots_searched = 0;
        scratch.impact_position.vz = local_position->vz;
        impact_pool = EffectSlot;
        impact_index = EFFECT_CURSOR_;
        impact_slot = impact_pool + impact_index;
        do
        {
            impact_index++;
            impact_slot++;
            if (impact_index > N_EFFECT_SLOTS - 1)
            {
                impact_slot = impact_pool;
                impact_index = 0;
            }
            if (impact_slot->proc == 0)
            {
                EFFECT_CURSOR_ = impact_index + 1;
                if (N_EFFECT_SLOTS - 1 < impact_index + 1)
                {
                    EFFECT_CURSOR_ = 0;
                }
                impact_effect = impact_slot;
                goto impact_found;
            }
            impact_slots_searched++;
        } while (impact_slots_searched < N_EFFECT_SLOTS);
        impact_effect = &dmy;
    impact_found:
        impact_effect->proc = (void (*)())DrawImpact;
        impact_effect->param.impact.px = scratch.impact_position.vx;
        impact = &impact_effect->param.impact;
        impact->py = scratch.impact_position.vy;
        impact_pz = scratch.impact_position.vz;
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
