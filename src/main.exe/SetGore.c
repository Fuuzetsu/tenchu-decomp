#include "common.h"
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

/*
 * MATCH. This is the retail form of EFFECT.C's SetGore. It converts a
 * model-space position and direction into a blood/gore effect, then emits a
 * larger impact particle every fourth frame. Its ABI and allocation logic
 * are a retail redesign; the source identity is established by adjacency and
 * by installing DrawGore as the effect callback.
 *
 * The two EffectSlot searches intentionally use distinct scoped locals.  The
 * first cursor coalesces with the BloodType pointer in $s0; keeping one cursor
 * variable live through both searches rotates nearly every scan register.
 * `rotated` is genuinely a two-VECTOR workspace: its second element holds the
 * three signed position captures at sp+0x58..0x60 while the second pool search
 * runs.  Independent scalar captures stay in registers, shorten the function,
 * and lose the target's 0x88-byte frame.  Finally, naming `final_pz` immediately
 * after the py store preserves the target's early load and late pz store.
 */
void SetGore(GsCOORDINATE2 *coord, SVECTOR *position, SVECTOR *vector)
{
    VECTOR world;
    MATRIX mat;
    SVECTOR local_vector;
    VECTOR rotated[2];
    long flag[2];
    u32 clock;

    GsGetLw(coord, &mat);
    GsSetLsMatrix(&mat);
    RotTrans(position, &world, flag);

    {
        int idx;
        TEffectSlot *base;
        TEffectSlot *slot;
        int count;
        TEffectSlot *ef;
        BloodType *param;

        count = 0;
        base = EffectSlot;
        idx = EFFECT_CURSOR_;
        slot = base + idx;
        do
        {
            idx++;
            slot++;
            if (idx > N_EFFECT_SLOTS - 1)
            {
                slot = base;
                idx = 0;
            }
            if (slot->proc == 0)
            {
                EFFECT_CURSOR_ = idx + 1;
                if (N_EFFECT_SLOTS - 1 < idx + 1)
                {
                    EFFECT_CURSOR_ = 0;
                }
                ef = slot;
                goto found;
            }
            count++;
        } while (count < N_EFFECT_SLOTS);
        ef = &dmy;
    found:
        param = &ef->param.blood;
        param->sprite = rand() % 2;
        param->scale = 0x2000;
        param->rotate = (rand() % 360) * 4096;
        param->px = world.vx;
        param->py = world.vy;
        param->pz = world.vz;
        local_vector = *vector;
        ApplyRotMatrix(&local_vector, rotated);
        param->vx = (short)rotated[0].vx;
        param->vy = (short)rotated[0].vy;
        param->vz = (short)rotated[0].vz;
        param->time = rand() % 15 + 10;
        param->hint = 0;
        param->brightness = 0x80;
        param->mode = 0;
        clock = GameClock & 3;
        ef->proc = (void (*)())DrawGore;
    }

    if (clock == 0)
    {
        int idx;
        TEffectSlot *base;
        TEffectSlot *slot;
        TEffectSlot *ef;
        int count;
        ImpactType *impact;
        long final_pz;
        long start_color;
        long end_color;

        start_color = COLOR_GRAY;
        end_color = COLOR_GRAY;
        rotated[1].vx = position->vx;
        rotated[1].vy = position->vy;
        count = 0;
        rotated[1].vz = position->vz;
        base = EffectSlot;
        idx = EFFECT_CURSOR_;
        slot = base + idx;
        do
        {
            idx++;
            slot++;
            if (idx > N_EFFECT_SLOTS - 1)
            {
                slot = base;
                idx = 0;
            }
            if (slot->proc == 0)
            {
                EFFECT_CURSOR_ = idx + 1;
                if (N_EFFECT_SLOTS - 1 < idx + 1)
                {
                    EFFECT_CURSOR_ = 0;
                }
                ef = slot;
                goto impact_found;
            }
            count++;
        } while (count < N_EFFECT_SLOTS);
        ef = &dmy;
    impact_found:
        ef->proc = (void (*)())DrawImpact;
        ef->param.impact.px = rotated[1].vx;
        impact = &ef->param.impact;
        impact->py = rotated[1].vy;
        final_pz = rotated[1].vz;
        impact->rotate_speed = 80;
        impact->start_size = 0x2000;
        impact->end_size = 0x2000;
        impact->time = 3;
        impact->super = coord;
        impact->rotate = 0;
        impact->start_color.word = start_color;
        impact->end_color.word = end_color;
        impact->count = 0;
        impact->type = 2;
        impact->pz = final_pz;
    }
}
