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
        param->scale += 0x400;
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
    spr->sprite.b = spr->sprite.g = spr->sprite.r = alfa;
    spr->sprite.rotate = rotate;
    UpdateCoordinate((ModelType *)spr);
    DrawSprite(spr);

    oldtime = param->time;
    param->time += 0xff;
    if (oldtime == 0)
    {
        ef->proc = 0;
    }
}
