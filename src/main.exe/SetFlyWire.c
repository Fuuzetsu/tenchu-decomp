#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int SetFlyWire(struct VECTOR *start, struct VECTOR *end);
 *     EFFECT.C:1384, 40 src lines, frame 48 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a2       struct VECTOR * start
 *     param $a3       struct VECTOR * end
 *     reg   $s5       struct tag_EffectSlot * slot
 *     reg   $s3       struct FlyWireType * param
 *     reg   $s2       int dist
 *     reg   $a1       int i
 *     reg   $s3       struct VECTOR * v1
 *     reg   $a0       long dz
 *     reg   $a2       long dy
 *     reg   $t0       long dx
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

extern long abs(long value);
extern void DrawFlyWire(TEffectSlot *ef);

int SetFlyWire(VECTOR *start, VECTOR *end)
{
    TEffectSlot *base;
    TEffectSlot *slot;
    FlyWireType *param;
    int idx;
    int i;
    int dist;
    int result;

    idx = EFFECT_CURSOR_;
    i = 0;
    base = EffectSlot;
    do
    {
        idx++;
        if (idx >= N_EFFECT_SLOTS)
        {
            idx = 0;
        }
        i++;
        if (base[idx].proc == 0)
        {
            EFFECT_CURSOR_ = idx + 1;
            if (EFFECT_CURSOR_ >= N_EFFECT_SLOTS)
            {
                EFFECT_CURSOR_ = 0;
            }
            slot = &base[idx];
            goto found;
        }
    } while (i < N_EFFECT_SLOTS);
    slot = &dmy;

found:
    param = &slot->param.flywire;
    param->start = *start;
    param->end = *end;
    param->count = 0;
    param->mode = FLYWIRE_MODE_EXTEND;

    {
        VECTOR *v1;
        long dx;
        long dy;
        long dz;
        int big;
        long root;
        long base_x;
        long base_y;
        long base_z;
        long value_x;
        long value_y;
        long value_z;

        v1 = &param->end;
        dx = param->start.vx - v1->vx;
        dy = param->start.vy - v1->vy;
        dz = param->start.vz - v1->vz;

        big = 0;
        if (abs(dx) > FIXED_ONE || abs(dy) > FIXED_ONE ||
            abs(dz) > FIXED_ONE)
        {
            big = 1;
        }
        if (big)
        {
            dx /= 0x100;
            dy /= 0x100;
            dz /= 0x100;
            root = SquareRoot0(dx * dx + dy * dy + dz * dz) << 8;
        }
        else
        {
            root = SquareRoot0(dx * dx + dy * dy + dz * dz);
        }
        dist = root;

        param->NCenter.vx = (param->start.vx + param->end.vx) / 2;
        param->NCenter.vy = (param->start.vy + param->end.vy) / 2;
        param->NCenter.vz = (param->start.vz + param->end.vz) / 2;
        param->time = dist / 1000;

        dist /= 16;

        base_x = param->NCenter.vx;
        if (dist * 2 > 0)
        {
            value_x = base_x + (rand() % (dist * 2) - dist);
        }
        else
        {
            value_x = base_x - dist;
        }
        param->center.vx = value_x;

        base_y = param->NCenter.vy;
        if (dist > 0)
        {
            value_y = base_y + (rand() % dist - dist);
        }
        else
        {
            value_y = base_y - dist;
        }
        param->center.vy = value_y;

        base_z = param->NCenter.vz;
        if (dist * 2 > 0)
        {
            value_z = base_z + (rand() % (dist * 2) - dist);
        }
        else
        {
            value_z = base_z - dist;
        }
        param->center.vz = value_z;
    }

    if (param->time > 0)
    {
        slot->proc = DrawFlyWire;
        result = param->time + 5;
    }
    else
    {
        result = 0;
    }
    return result;
}
