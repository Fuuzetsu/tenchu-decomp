#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawExplosion(struct tag_EffectSlot *ef);
 *     EFFECT.C:1177, 57 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a1       struct tag_EffectSlot * ef
 *     reg   $a2       struct ExplosionType * param
 *     reg   $s0       struct Sprite3D * spr
 *     reg   $a3       unsigned char alfa
 *
 * Globals it touches, as the original declared them:
 *     extern struct Sprite3D *sprBomb[3];
 * END PSX.SYM */

extern short DrawSprite(Sprite3D *sprt);

void DrawExplosion(TEffectSlot *ef)
{
    enum
    {
        fo = 5
    };
    ExplosionType *param;
    Sprite3D *spr;
    u8 alfa;
    long rotate;

    param = &ef->param.explosion;
    alfa = 0x80;
    switch (param->mode)
    {
    case EXPLOSION_MODE_FLASH:
        if (param->time == 0)
        {
            param->time = 3;
            param->mode++;
        }
        else
        {
            param->scale += 0x2000;
            param->rotate += 100 * FIXED_ONE; /* 100 deg/frame */
        }
        spr = sprBomb[BOMB_SPRITE_FLASH];
        break;
    case EXPLOSION_MODE_EXPAND:
        if (param->time == 0)
        {
            param->time = fo;
            param->mode++;
        }
        param->scale += 0x2000;
        param->rotate += 100 * FIXED_ONE; /* 100 deg/frame */
        spr = sprBomb[BOMB_SPRITE_EXPANDED];
        break;
    case EXPLOSION_MODE_FADE:
        alfa = (u8)((param->time << 7) / fo);
        param->scale -= 0x333;
        param->rotate += 90 * FIXED_ONE; /* 90 deg/frame */
        if (param->time == 0)
        {
            ef->proc = 0;
        }
        spr = sprBomb[BOMB_SPRITE_EXPANDED];
        break;
    }
    param->time--;
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
}
