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
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct VECTOR * pos
 *     param $a1       struct SVECTOR * vec
 *     param $a2       int time
 *     param $a3       long col
 * END PSX.SYM */

/*
 * SetGore (0x80035f44) — spawns the wound spray for a killing blow:
 * one DrawGore blood particle from a model-local hit point, plus a
 * DrawImpact flash on one frame in four. coord is the wounded object's
 * GsCOORDINATE2 (ActDEAD passes model->object[blood]->locate) and
 * position/vector are model-local, so GsGetLw + GsSetLsMatrix +
 * RotTrans turn the point into world space and ApplyRotMatrix turns
 * the direction into the particle's world velocity. The slot comes
 * from the shared 200-entry EffectSlot pool by the usual round-robin
 * scan: EFFECT_CURSOR_ advances from where it stopped, wraps past 199,
 * takes the first null proc, and after 200 misses writes the throwaway
 * dmy slot instead. The BloodType fill randomizes sprite = rand() % 2,
 * rotate to a whole degree (rand() % 360, degrees<<12) and
 * time = rand() % 15 + 10, with scale 0x2000, hint 0, brightness 0x80,
 * mode 0 and proc = DrawGore. Then, only when (GameClock & 3) == 0, a
 * second slot is claimed for an ImpactType that keeps the RAW
 * model-local px/py/pz and carries super = coord, so DrawImpact rides
 * the moving limb: start and end size both 0x2000, start and end
 * colour both 0x808080, rotate 0, rotate_speed 80, time 3, count 0,
 * type 2, proc = DrawImpact.
 */

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
 * and lose the target's 0x88-byte frame.  Finally, naming `final_py` immediately
 * after the px store preserves the target's early load and late py store.
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
            if (idx > 199)
            {
                slot = base;
                idx = 0;
            }
            if (slot->proc == 0)
            {
                EFFECT_CURSOR_ = idx + 1;
                if (199 < idx + 1)
                {
                    EFFECT_CURSOR_ = 0;
                }
                ef = slot;
                goto found;
            }
            count++;
        } while (count < 200);
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
        long final_py;
        long start_color;
        long end_color;

        start_color = 0x808080;
        end_color = 0x808080;
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
            if (idx > 199)
            {
                slot = base;
                idx = 0;
            }
            if (slot->proc == 0)
            {
                EFFECT_CURSOR_ = idx + 1;
                if (199 < idx + 1)
                {
                    EFFECT_CURSOR_ = 0;
                }
                ef = slot;
                goto impact_found;
            }
            count++;
        } while (count < 200);
        ef = &dmy;
    impact_found:
        ef->proc = (void (*)())DrawImpact;
        ef->param.impact.px = rotated[1].vx;
        impact = &ef->param.impact;
        impact->py = rotated[1].vy;
        final_py = rotated[1].vz;
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
        impact->pz = final_py;
    }
}
