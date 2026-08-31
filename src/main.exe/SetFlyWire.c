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
    TEffectSlot *ef;
    FlyWireType *param;
    int idx;
    int i;
    int dist;
    int result;

    idx = EFFECT_CURSOR_;
    i = 0;
    base = EffectSlot;
    slot = base + idx;
loop:
    idx++;
    slot++;
    if (idx > N_EFFECT_SLOTS - 1)
    {
        slot = base;
        idx = 0;
    }
    i++;
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
    if (i > N_EFFECT_SLOTS - 1)
    {
        ef = &dmy;
        goto found;
    }
    goto loop;

found:
    param = &ef->param.flywire;
    param->start = *start;
    param->end = *end;
    param->count = 0;
    param->mode = 0;

    {
        VECTOR *v1;
        long dz;
        long dy;
        long dx;
        int big;
        long v;
        long root;
        long scaled;
        long range;
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
        if (abs(dx) > 0x1000 || abs(dy) > 0x1000 || abs(dz) > 0x1000)
        {
            big = 1;
        }
        if (big)
        {
            /* The whole hand-spelled /0x100 cluster through `v` is
             * byte-required (plain dx /= 0x100 recolors the mult pair;
             * measured -- unlike the SetWire/SetLightningI twins). */
            v = dx;
            if (dx < 0)
            {
                v = dx + 0xff;
            }
            dx = v >> 8;
            /* Folded after flow to replace the former dy allocation weight. */
            v = ((u32)dy + (u32)dy) - (u32)dy;
            if (dy < 0)
            {
                v = dy + 0xff;
            }
            dy = v >> 8;
            v = dz;
            if (dz < 0)
            {
                v = dz + 0xff;
            }
            dz = v >> 8;
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

        scaled = dist;
        if (dist < 0)
        {
            scaled = dist + 0xf;
        }
        dist = scaled >> 4;
        /* empty one-shot: a sched1 region fence (an emptied debug print
         * reads the same way -- see DefaultActionHumanoid's header). */
        do
        {
        } while (0);

        base_x = param->NCenter.vx;
        range = dist * 2;
        if (range > 0)
        {
            value_x = base_x + (rand() % range - dist);
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
        range = dist * 2;
        if (range > 0)
        {
            value_z = base_z + (rand() % range - dist);
        }
        else
        {
            value_z = base_z - dist;
        }
        param->center.vz = value_z;
    }

    if (param->time > 0)
    {
        ef->proc = (void (*)())DrawFlyWire;
        result = param->time + 5;
    }
    else
    {
        result = 0;
    }
    return result;
}
