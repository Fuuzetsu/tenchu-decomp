#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void MoveFly(struct tag_TItem *item, struct param_fly *param);
 *     ITEM.C:742, 43 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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

/*
 * MoveFly (0x8003dfd4) — advances a thrown/flying item one frame along a
 * quadratic Bezier arc, or hands off to MoveKorogari once it has landed.
 * Needs maspsx --expand-div (Build.hs + permute.py): it divides `t<<12` by
 * the runtime byte `param->div` (ASPSX break 7 / break 6 guards).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - The mode dispatch is the two-independent-goto shape: `if (mode==0) goto
 *    fly; if (mode==1) goto korogari; return;` — both bodies are forward-jump
 *    targets, the do-nothing default falls through.
 *  - `k = 0x1000;` as a named local shares the constant across `q = k - …`
 *    and `w9 = k - d2 + …` (one `li` reused as a subtract base), matching the
 *    target's a3; two inline `0x1000` literals compile a fold-reassociated
 *    `q2 + 0x1000` and diverge.
 *  - `d2 = q * 2` as its own statement lets reorg steal the `sll` into the
 *    `q*q < 0` guard's delay slot; folded into w8/w9 it leaves a nop there.
 *  - Each `xs/ys/zs = >>12` sits immediately after its own `<0` adjust so
 *    reorg fills the NEXT guard's delay slot with it (deferred one step).
 *  - `nv = q2;` is an explicit second copy of q2 (permuter-found): the target
 *    keeps q2 live in TWO registers — one (`q2`) used only by the `y` multiply,
 *    one (`nv`) by w9/w8 and the `x`/`z` multiplies. gcc 2.8.1 never splits a
 *    live range, so one value in two registers = two source variables.
 *
 * PSX.SYM names the three curve points `s`, `v`, and `r` in `tag_fly`.
 * `param_fly.p` overlays that record with param_korogari for the landing
 * transition. */

extern void MoveKorogari(tag_TItem *item, param_korogari *param);

static void MoveFly(tag_TItem *item, param_fly *param)
{
    s32 x, y, z, q, q2, w9, w8, d2, k, nv;
    s32 xs, ys, zs;
    s32 t, ax, ay, az;
    ModelType *model;
    param_korogari *pk;

    if (param->mode == 0)
        goto fly;
    if (param->mode == 1)
        goto korogari;
    return;

fly:
    t = param->p.fly.count;
    k = 0x1000;
    q = k - (t << 12) / param->p.fly.count2;
    q2 = q * q;
    d2 = q * 2;
    if (q2 < 0)
        q2 += 0xfff;
    q2 = q2 >> 12;
    nv = q2;
    w9 = k - d2 + nv;
    w8 = d2 + nv * -2;
    x = w9 * param->p.fly.sx + w8 * param->p.fly.rx + nv * param->p.fly.vx;
    if (x < 0)
        x += 0xfff;
    xs = x >> 12;
    y = w9 * param->p.fly.sy + w8 * param->p.fly.ry + q2 * param->p.fly.vy;
    if (y < 0)
        y += 0xfff;
    ys = y >> 12;
    z = w9 * param->p.fly.sz + w8 * param->p.fly.rz + nv * param->p.fly.vz;
    if (z < 0)
        z += 0xfff;
    zs = z >> 12;
    if (t == 0)
    {
        model = item->locate;
        ax = model->locate.coord.t[0];
        ay = model->locate.coord.t[1];
        az = model->locate.coord.t[2];
        pk = &param->p.koro;
        pk->hint = 0;
        pk->status = 0;
        param->mode = 1;
        pk->vx = xs - ax;
        pk->vy = ys - ay;
        pk->vz = zs - az;
    }
    else
    {
        param->p.fly.count = param->p.fly.count - 1;
    }
    item->locate->locate.coord.t[0] = xs;
    item->locate->locate.coord.t[1] = ys;
    item->locate->locate.coord.t[2] = zs;
    return;

korogari:
    MoveKorogari(item, &param->p.koro);
}
