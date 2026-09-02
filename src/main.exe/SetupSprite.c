#include "common.h"
#include "main.exe.h"
#include "images.h"
#include "tim.h"
#include "item.h"
#include "model.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Sprite3D * SetupSprite(struct Sprite3D *orgsprt, struct GsIMAGE *image);
 *     3DCTRL.C:546, 43 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct Sprite3D * orgsprt
 *     param $s3       struct GsIMAGE * image
 *     reg   $s2       struct Sprite3D * sprt
 *     reg   $s2       struct ModelType * dim
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 * END PSX.SYM */

extern void *valloc(u32 size);
extern void *memset(void *s, s32 c, u32 n);

Sprite3D *SetupSprite(Sprite3D *orgsprt, GsIMAGE *image)
{
    Sprite3D *sprt;
    s32 texture_mode;
    s32 width_shift;

    sprt = (Sprite3D *)valloc(sizeof(Sprite3D));
    if (orgsprt != 0)
    {
        *sprt = *orgsprt;
    }
    else
    {
        ModelType *dim;

        dim = (ModelType *)sprt;
        INITIALIZE_MODEL_STATE(dim, &World.locate);
        sprt->scale = FIXED_ONE;
        memset(&sprt->sprite, 0, sizeof(GsSPRITE));
        sprt->sprite.attribute = 0;
        sprt->sprite.r = sprt->sprite.g = sprt->sprite.b = 0x80;
        sprt->sprite.scalex = sprt->sprite.scaley = FIXED_ONE;
        if (image != 0)
        {
            texture_mode = TIM_PIXEL_MODE((u16)image->pmode);
            sprt->sprite.attribute =
                sprt->sprite.attribute | GS_ATTR_TEXTURE_MODE(texture_mode);
            width_shift = 2 - texture_mode;
            sprt->sprite.w = image->pw << width_shift;
            sprt->sprite.h = image->ph;
            sprt->sprite.tpage =
                GetTPage(texture_mode, 0, image->px, image->py);
            sprt->sprite.u =
                (u8)((image->px << width_shift) &
                     ((1 << (8 - texture_mode)) - 1));
            sprt->sprite.v = (u8)image->py;
            sprt->sprite.cx = image->cx;
            sprt->sprite.cy = image->cy;
            sprt->sprite.mx = sprt->sprite.w >> 1;
            sprt->sprite.my = sprt->sprite.h >> 1;
        }
    }
    return sprt;
}
