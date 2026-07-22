#include "common.h"
#include "main.exe.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawHinoko(struct tag_EffectSlot *ef);
 *     EFFECT.C:1258, 61 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $a1       struct tag_EffectSlot * ef
 *     reg   $a2       struct ExplosionType * param
 *     reg   $s0       struct Sprite3D * spr
 *     reg   $a3       unsigned char alfa
 *
 * Globals it touches, as the original declared them:
 *     extern struct Sprite3D *sprBomb[3];
 * END PSX.SYM */

/*
 * MATCH.
 *
 * DrawHinoko advances the spark/explosion state, integrates its velocity,
 * copies the result to sprBomb[2], and draws it. `ef->param.hinoko` has the
 * ExplosionType layout: vec@0x0, pos@0x8, rotate@0x18, scale@0x1c,
 * time@0x20, and mode@0x21.
 *
 * After normalizing relocated global addresses, the demo implementation is
 * instruction-identical to retail apart from its epilogue. For this function,
 * PSX.SYM's line events map to the demo executable at +0x20 and form a useful
 * source-order oracle: after the switch the original statements are time,
 * vertical acceleration, scale, then position x/y/z. The old parked
 * draft instead preloaded z/x/y into invented scalar locals; that changed
 * sched1's quantity graph and created the supposed 18-byte allocation floor.
 * The ordinary compound assignments below reproduce the target allocation.
 *
 * Two other carriers are unnecessary. GCC CSEs the two direct `time` reads in
 * case 1 into the target's single lbu. A direct rotation assignment written
 * before the RGB stores keeps its load there and schedules its store into
 * UpdateCoordinate's delay slot. The result uses exactly PSX.SYM's original
 * locals: param, spr, and alfa.
 */

extern short DrawSprite(Sprite3D *sprt);

/* Originally static in EFFECT.C; global here because SetHinoko is split into
 * a separate translation unit and stores this function's address. */
void DrawHinoko(TEffectSlot *ef)
{
    enum { fo = 30 };
    ExplosionType *param;
    Sprite3D *spr;
    u8 alfa;

    param = &ef->param.hinoko;
    spr = sprBomb[2];
    alfa = 0x80;
    switch (param->mode)
    {
    case 0:
        if (param->time == 0)
        {
            param->mode = 1;
            param->time = fo;
        }
        break;
    case 1:
        alfa = (u8)((param->time * 0x80) / fo);
        if (param->time == 0)
        {
            ef->proc = 0;
        }
        break;
    }

    param->time--;
    param->vec.vy += 5;
    param->scale += 0xc00;
    param->pos.vx += param->vec.vx;
    param->pos.vy += param->vec.vy;
    param->pos.vz += param->vec.vz;
    spr->locate.coord.t[0] = param->pos.vx;
    spr->locate.coord.t[1] = param->pos.vy;
    spr->locate.coord.t[2] = param->pos.vz;
    spr->scale = param->scale;
    spr->sprite.rotate = param->rotate;
    spr->sprite.r = alfa;
    spr->sprite.g = alfa;
    spr->sprite.b = alfa;
    UpdateCoordinate((ModelType *)spr);
    DrawSprite(spr);
}
