#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void SetupFly(struct param_fly *pfly, struct VECTOR *start, struct VECTOR *end, int yw, int yh, int time);
 *     ITEM.C:792, 39 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct param_fly * pfly
 *     param $a1       struct VECTOR * start
 *     param $a2       struct VECTOR * end
 *     param $s5       int yw
 *     param stack+16  int yh
 *     param stack+20  int time
 *     reg   $s6       int yh
 *     reg   $s0       int time
 *     reg   $a0       long len
 *     reg   $s1       struct tag_fly * fly
 * END PSX.SYM */

static inline long SubFlyJitter(long mid, long half, long range)
{
    if (range > 0)
    {
        return mid - (rand() % range + half);
    }
    return mid - half;
}

void SetupFly(param_fly *pfly, VECTOR *start, VECTOR *end, s32 yw, s32 yh, s32 time)
{
    long len;
    long v8;
    long midx;
    long midz;
    long current_z;
    long x_product;
    struct tag_fly *fly;

    fly = &pfly->p.fly;
    pfly->mode = FLY_MODE_ARC;
    fly->sx = start->vx;
    fly->sy = start->vy;
    fly->sz = start->vz;
    copyVector(fly, end);
    len = GetVectorDistance(start, end);
    if (time > 0)
    {
        fly->count = len / time;
        if ((fly->count & 0xff) != 0)
        {
            goto skip_default;
        }
    }
    fly->count = 1;
skip_default:
    /* These biased shifts implement signed division with truncation toward zero. */
    x_product = len * (yw / 2);
    fly->count2 = fly->count;
    if (x_product < 0)
    {
        x_product += FIXED_TRUNC_BIAS;
    }
    len = len * (yh / 2);
    yw = x_product >> FIXED_SHIFT;
    if (len < 0)
    {
        len += FIXED_TRUNC_BIAS;
    }
    yh = len >> FIXED_SHIFT;
    midx = (fly->sx + fly->vx) / 2;
    v8 = yw << 1;
    if (v8 > 0)
    {
        len = midx + (rand() % v8 - yw);
    }
    else
    {
        len = midx - yw;
    }
    fly->rx = len;
    len = SubFlyJitter((fly->sy + fly->vy) / 2, yh / 2,
                       yh - yh / 2);
    midz = (fly->sz + fly->vz) / 2;
    v8 = yw << 1;
    fly->ry = len;
    if (v8 > 0)
    {
        current_z = midz + (rand() % v8 - yw);
    }
    else
    {
        current_z = midz - yw;
    }
    fly->rz = current_z;
    fly->count--;
}
