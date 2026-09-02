#include "common.h"
#include "main.exe.h"
#include "images.h"
#include "tim.h"
#include <psxsdk/libgpu.h>

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitSprite(struct GsIMAGE *image, struct GsSPRITE *sprite);
 *     IMAGES.C:79, 23 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct GsIMAGE * image
 *     param $s2       struct GsSPRITE * sprite
 * END PSX.SYM */

extern void *memset(void *s, s32 c, u32 n);

void InitSprite(GsIMAGE *image, GsSPRITE *sprite)
{
    s32 texture_mode;
    s32 width_shift;

    memset(sprite, 0, sizeof(GsSPRITE));
    sprite->b = 0x80;
    sprite->g = 0x80;
    sprite->r = 0x80;
    sprite->attribute = 0;
    sprite->scaley = FIXED_ONE;
    sprite->scalex = FIXED_ONE;
    if (image != 0)
    {
        texture_mode = TIM_PIXEL_MODE((u16)image->pmode);
        sprite->attribute =
            sprite->attribute | GS_ATTR_TEXTURE_MODE(texture_mode);
        width_shift = 2 - texture_mode;
        sprite->w = image->pw << width_shift;
        sprite->h = image->ph;
        sprite->tpage = GetTPage(texture_mode, 0, image->px, image->py);
        sprite->u = (u8)((image->px << width_shift) &
                          ((1 << (8 - texture_mode)) - 1));
        sprite->v = (u8)image->py;
        sprite->cx = image->cx;
        sprite->cy = image->cy;
        sprite->mx = sprite->w >> 1;
        sprite->my = sprite->h >> 1;
    }
}
