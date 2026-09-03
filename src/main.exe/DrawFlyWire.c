#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"
#include "item.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawFlyWire(struct tag_EffectSlot *ef);
 *     EFFECT.C:1346, 36 src lines, frame 72 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct tag_EffectSlot * ef
 *     reg   $s0       struct FlyWireType * param
 *     stack sp+24     struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

void DrawFlyWire(TEffectSlot *ef)
{
    enum
    {
        FLYWIRE_STRAIGHTEN_FRAMES = 5
    };
    FlyWireType *param;

    param = &ef->param.flywire;
    switch (param->mode)
    {
    case FLYWIRE_MODE_EXTEND:
    {
        s16 time;
        s32 sum;

        time = param->time;
        sum = (u16)param->count + FIXED_ONE / time;
        param->count = sum;
        if ((s16)sum > FIXED_ONE)
        {
            param->count = 0;
            param->mode++;
            SetBleeds(&param->end, 0, 50, 10, 30, COLOR_YELLOW);
            Sound(CamState.Owner, SE_PROJECTILE_IMPACT);
        }
        else
        {
            SetWire(&param->start, &param->end, &param->center, (s16)sum);
        }
        return;
    }
    case FLYWIRE_MODE_STRAIGHTEN:
    {
        VECTOR pos = {
            .vx = ((param->center.vx *
                    (FLYWIRE_STRAIGHTEN_FRAMES - param->count)) +
                   (param->NCenter.vx * param->count)) /
                FLYWIRE_STRAIGHTEN_FRAMES,
            .vy = ((param->center.vy *
                    (FLYWIRE_STRAIGHTEN_FRAMES - param->count)) +
                   (param->NCenter.vy * param->count)) /
                FLYWIRE_STRAIGHTEN_FRAMES,
            .vz = ((param->center.vz *
                    (FLYWIRE_STRAIGHTEN_FRAMES - param->count)) +
                   (param->NCenter.vz * param->count)) /
                FLYWIRE_STRAIGHTEN_FRAMES
        };

        SetWire(&param->start, &param->end, &pos, FIXED_ONE);
        if (param->count >= FLYWIRE_STRAIGHTEN_FRAMES)
        {
            ef->proc = 0;
        }
        param->count++;
        return;
    }
    }
}
