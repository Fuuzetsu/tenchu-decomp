#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void MoveFly(struct tag_TItem *item, struct param_fly *param);
 *     ITEM.C:742, 43 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t4       struct tag_TItem * item
 *     param $a2       struct param_fly * param
 *     reg   $t3       long x
 *     reg   $t0       long y
 *     reg   $a3       long z
 *     reg   $v0       long t
 *     reg   $a0       long Q
 *     reg   $a1       long R
 *     reg   $a2       struct param_korogari * param
 * END PSX.SYM */

extern void MoveKorogari(TItem *item, param_korogari *param);

static void MoveFly(TItem *item, param_fly *param)
{
    s32 x, y, z, q, q2, w9, w8, d2, k, nv;
    s32 xs, ys, zs;
    s32 t, ax, ay, az;

    switch (param->mode)
    {
    case FLY_MODE_ARC:
    {
        t = param->p.fly.count;
        k = FIXED_ONE;
        q = k - (t << FIXED_SHIFT) / param->p.fly.count2;
        q2 = q * q;
        d2 = q * 2;
        if (q2 < 0)
            q2 += FIXED_TRUNC_BIAS;
        q2 = q2 >> FIXED_SHIFT;
        nv = q2;
        w9 = k - d2 + nv;
        w8 = d2 + nv * -2;
        x = w9 * param->p.fly.sx + w8 * param->p.fly.rx + nv * param->p.fly.vx;
        xs = x / FIXED_ONE;
        y = w9 * param->p.fly.sy + w8 * param->p.fly.ry + q2 * param->p.fly.vy;
        ys = y / FIXED_ONE;
        z = w9 * param->p.fly.sz + w8 * param->p.fly.rz + nv * param->p.fly.vz;
        zs = z / FIXED_ONE;
        if (t == 0)
        {
            ax = item->locate->locate.coord.t[0];
            ay = item->locate->locate.coord.t[1];
            az = item->locate->locate.coord.t[2];
            param->p.koro.hint = 0;
            param->p.koro.status = KORO_NORMAL;
            param->mode = FLY_MODE_ROLL;
            setVector(&param->p.koro, xs - ax, ys - ay, zs - az);
        }
        else
        {
            param->p.fly.count--;
        }
        item->locate->locate.coord.t[0] = xs;
        item->locate->locate.coord.t[1] = ys;
        item->locate->locate.coord.t[2] = zs;
        return;
    }
    case FLY_MODE_ROLL:
    {
        MoveKorogari(item, &param->p.koro);
        break;
    }
    }
}
