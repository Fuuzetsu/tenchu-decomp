#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawSmoke(struct tag_EffectSlot *ef);
 *     EFFECT.C:794, 39 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct tag_EffectSlot * ef
 *     reg   $s0       struct SmokeType * smoke
 *     reg   $s1       struct Sprite3D * spr
 *     reg   $s2       unsigned char alfa
 *
 * Globals it touches, as the original declared them:
 *     extern struct Sprite3D *sprSmoke;
 * END PSX.SYM */

/*
 * MATCH.
 *
 * DrawSmoke (0x800334c4, EFFECT.C:794) — the smoke-puff effect's per-frame
 * draw: picks its sprite from `sprSmoke[param->sprite]`; when the lifetime
 * countdown reaches its scheduled event time (`time==evtime`), it damps the
 * velocity by 0.8x every time `vec.vy` drifts below -20 and bumps
 * `scale`/re-rolls `evtime` from `time` and a `rand()%5` jitter; then, if
 * `time<26`, sets `alfa = time*5` (else the 0x80 default), integrates
 * `pos += vec`, writes the sprite's position/scale/color/rotate, draws it,
 * and finally decrements `time` (`+0xff`, i.e. a real `-1` wrap for the u8
 * field — see the encoding note below), disposing the slot when the OLD
 * time was 0.
 *
 * Matching constraints:
 *  - param is the proven SmokeType at ef+4. sprSmoke is a two-entry
 *    Sprite3D pointer array in retail, and Sprite3D retains its complete
 *    140-byte shared layout.
 *  - Leave the 80/100 damping and signed /2 as plain arithmetic; cc1 emits
 *    the target magic multiply and sign-correct shift.
 *  - Capture vz_old immediately after storing vx, update vy next, and store
 *    the damped vz last. This gives the target load order but vx/vy/vz store
 *    order.
 *  - Re-roll in three statements: call rand(), compute m = time - 1, then
 *    assign m - r % 5. Combining either subtraction lets fold reassociate it
 *    and leaves four extra bytes.
 *  - Load spr through a named Sprite3D **sprp. Direct array indexing changes
 *    the prologue saved-register order; the extra source identity is an
 *    allocation lever, not a second runtime indirection.
 *  - Capture rotate after the scale store and write it after r/g/b. Direct
 *    field assignment lets those color stores fill a delay slot that is a
 *    target nop.
 *  - Let the time < 26 guard read time once for its comparison and again for
 *    the multiply; alfa's 0x80 default is materialized once before dispatch.
 *  - Re-read pos.vx/vy/vz for the final sprite coordinate stores rather than
 *    carrying the integration values.
 *  - Capture oldtime once for the final test, but spell the store as
 *    param->time + 0xff. The positive byte increment and old-value test share
 *    one lbu in the target.
 */
extern short DrawSprite(Sprite3D *sprt);

void DrawSmoke(TEffectSlot *ef)
{
    SmokeType *param = &ef->param.smoke;
    Sprite3D *spr;
    Sprite3D **sprp;
    u8 alfa;
    u8 oldtime;
    s32 vz_old;
    s32 r;
    s32 m;
    s32 rotate;

    sprp = &sprSmoke[param->sprite];
    spr = *sprp;
    alfa = 0x80;

    if (param->time == param->evtime)
    {
        if (param->vec.vy < -20)
        {
            param->vec.vx = (param->vec.vx * 80) / 100;
            vz_old = param->vec.vz;
            param->vec.vy = param->vec.vy / 2;
            param->vec.vz = (vz_old * 80) / 100;
        }
        param->scale = param->scale + 0x400;
        r = rand();
        m = param->time - 1;
        param->evtime = m - r % 5;
    }

    if (param->time < 26)
    {
        alfa = param->time * 5;
    }

    param->pos.vx += param->vec.vx;
    param->pos.vy += param->vec.vy;
    param->pos.vz += param->vec.vz;
    spr->locate.coord.t[0] = param->pos.vx;
    spr->locate.coord.t[1] = param->pos.vy;
    spr->locate.coord.t[2] = param->pos.vz;
    spr->scale = param->scale;
    rotate = param->rotate;
    spr->sprite.r = alfa;
    spr->sprite.g = alfa;
    spr->sprite.b = alfa;
    spr->sprite.rotate = rotate;
    UpdateCoordinate((ModelType *)spr);
    DrawSprite(spr);

    oldtime = param->time;
    param->time = param->time + 0xff;
    if (oldtime == 0)
    {
        ef->proc = 0;
    }
}
