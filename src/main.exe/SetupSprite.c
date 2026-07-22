#include "common.h"
#include "main.exe.h"
#include "item.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Sprite3D * SetupSprite(struct Sprite3D *orgsprt, struct GsIMAGE *image);
 *     3DCTRL.C:546, 43 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $s0       struct Sprite3D * orgsprt
 *     param $s3       struct GsIMAGE * image
 *     reg   $s2       struct Sprite3D * sprt
 *     reg   $s2       struct ModelType * dim
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 * END PSX.SYM */

/*
 * SetupSprite (0x80017a18, 0x1d0 bytes) — Sprite3D's allocate+init
 * constructor. Unlike CreateCloneModel/CreateCloneOrnament (which always
 * build a fresh zeroed instance and optionally copy just the `tmd` model
 * pointer), a non-null `orgsprt` here clones EVERY field verbatim via a
 * plain aggregate assignment (`*base = *orgsprt;` — the whole 0x8c-byte
 * struct, compiled as an inline word-at-a-time block copy, not a memcpy
 * call); only a NULL `orgsprt` takes the zero-init path (World-rooted
 * GsCOORDINATE2, zeroed translation + RotMatrixYXZ, default grey/full-scale
 * GsSPRITE) and optionally derives the sprite's pixel geometry from `image`
 * — same field-by-field shape and idioms as InitSprite.c (IMAGES.C, matched
 * — this TU's twin): `tp`/`sh` are named locals reused after the GetTPage
 * call, `image->px`/`py` are re-read (fresh loads, GetTPage clobbers the
 * caller-saved copies), and `(u8)image->py` for `.v` is a genuinely separate
 * byte load from the earlier signed `lh` of the same field.
 *
 * Sprite3D's complete 0x8C-byte PSX.SYM layout is shared in game_types.h;
 * `sprite` is its trailing GsSPRITE member at +0x68.
 */
extern void *valloc(u32 size);
extern void *memset(void *s, s32 c, u32 n);


Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image)
{
    Sprite3D *base;
    s32 tp;
    s32 sh;

    base = (Sprite3D *)valloc(sizeof(Sprite3D));
    if (orgsprt != 0)
    {
        *base = *orgsprt;
    }
    else
    {
        GsInitCoordinate2(&World.locate, (GsCOORDINATE2 *)base);
        base->locate.coord.t[0] = 0;
        base->locate.coord.t[1] = 0;
        base->locate.coord.t[2] = 0;
        base->rotate.vx = 0;
        base->rotate.vy = 0;
        base->rotate.vz = 0;
        base->clip.vx = 0;
        base->clip.vy = 0;
        base->clip.vz = 0;
        RotMatrixYXZ(&base->rotate, &base->locate.coord);
        base->locate.flg = 0;
        base->id = -1;
        base->attribute = 0;
        base->scale = 0x1000;
        memset(&base->sprite, 0, sizeof(GsSPRITE));
        base->sprite.attribute = 0;
        base->sprite.b = 0x80;
        base->sprite.g = 0x80;
        base->sprite.r = 0x80;
        base->sprite.scaley = 0x1000;
        base->sprite.scalex = 0x1000;
        if (image != 0)
        {
            tp = *(u16 *)&image->pmode & 3;
            base->sprite.attribute = base->sprite.attribute | (tp << 0x18);
            sh = 2 - tp;
            base->sprite.w = image->pw << sh;
            base->sprite.h = image->ph;
            base->sprite.tpage = GetTPage(tp, 0, image->px, image->py);
            base->sprite.u = (u8)((image->px << sh) & ((1 << (8 - tp)) - 1));
            base->sprite.v = (u8)image->py;
            base->sprite.cx = image->cx;
            base->sprite.cy = image->cy;
            base->sprite.mx = base->sprite.w >> 1;
            base->sprite.my = base->sprite.h >> 1;
        }
    }
    return base;
}
