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
 * `sprite` is its trailing GsSPRITE member at +0x68. PSX.SYM's `dim` view
 * initializes the ModelType-compatible prefix through +0x63; `sprt` handles
 * the Sprite3D-only scale and sprite tail.
 */
extern void *valloc(u32 size);
extern void *memset(void *s, s32 c, u32 n);


Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image)
{
    Sprite3D *sprt;
    s32 tp;
    s32 sh;

    sprt = (Sprite3D *)valloc(sizeof(Sprite3D));
    if (orgsprt != 0)
    {
        *sprt = *orgsprt;
    }
    else
    {
        ModelType *dim;

        dim = (ModelType *)sprt;
        GsInitCoordinate2(&World.locate, &dim->locate);
        dim->locate.coord.t[0] = 0;
        dim->locate.coord.t[1] = 0;
        dim->locate.coord.t[2] = 0;
        dim->rotate.vx = 0;
        dim->rotate.vy = 0;
        dim->rotate.vz = 0;
        dim->clip.vx = 0;
        dim->clip.vy = 0;
        dim->clip.vz = 0;
        RotMatrixYXZ(&dim->rotate, &dim->locate.coord);
        dim->locate.flg = 0;
        dim->id = -1;
        dim->attribute = 0;
        sprt->scale = 0x1000;
        memset(&sprt->sprite, 0, sizeof(GsSPRITE));
        sprt->sprite.attribute = 0;
        sprt->sprite.b = 0x80;
        sprt->sprite.g = 0x80;
        sprt->sprite.r = 0x80;
        sprt->sprite.scaley = 0x1000;
        sprt->sprite.scalex = 0x1000;
        if (image != 0)
        {
            tp = *(u16 *)&image->pmode & 3;
            sprt->sprite.attribute = sprt->sprite.attribute | (tp << 0x18);
            sh = 2 - tp;
            sprt->sprite.w = image->pw << sh;
            sprt->sprite.h = image->ph;
            sprt->sprite.tpage = GetTPage(tp, 0, image->px, image->py);
            sprt->sprite.u = (u8)((image->px << sh) & ((1 << (8 - tp)) - 1));
            sprt->sprite.v = (u8)image->py;
            sprt->sprite.cx = image->cx;
            sprt->sprite.cy = image->cy;
            sprt->sprite.mx = sprt->sprite.w >> 1;
            sprt->sprite.my = sprt->sprite.h >> 1;
        }
    }
    return sprt;
}
