#include "common.h"
#include "main.exe.h"
#include "images.h"

void tile_sprite_(Sprite3D *g, u16 shade)
{
    GsSPRITE *sp;
    u32 flags;
    s16 signedShade;
    s32 level;
    s32 initial_h;
    u16 h;
    s32 width;
    s32 height;

    signedShade = (s16)shade;
    width = 160;
    height = 120;
    flags = g->sprite.attribute & ~GS_ATTR_SEMITRANS_MASK;
    /* Dead store, but retail's own bytes (removal breaks the image): the
     * masked word is written back once plain before the blend bit lands. */
    g->sprite.attribute = flags;
    g->sprite.attribute =
        flags | (signedShade > 0
                     ? GS_ATTR_SEMITRANS_SUBTRACT
                     : GS_ATTR_SEMITRANS_ADD);
    sp = &g->sprite;
    level = __builtin_abs((s32)signedShade);
    sp->b = (u8)level;
    sp->g = (u8)level;
    sp->r = (u8)level;
    initial_h = sp->h;
    sp->y = -height;
    if (-height <= height + initial_h)
    {
        do
        {
            sp->x = -width;
            while (sp->x <= width)
            {
                GsSortSprite(sp, OTablePt, 1);
                sp->x += sp->w;
            }
            sp->y += sp->h;
            h = sp->h;
        } while (sp->y <= height + h);
    }
}
