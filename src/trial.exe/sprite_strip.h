#ifndef TENCHU_TRIAL_SPRITE_STRIP_H
#define TENCHU_TRIAL_SPRITE_STRIP_H

#include "mission_menu.h"
#include "../main.exe/images.h"
#include "../main.exe/tim.h"

/* Upload one TIM and expose its vertical frames as ordinary LIBGS sprites.
 * With zero frames, only the full-image sprite at index zero is initialized. */
#ifndef TRIAL_DEFINE_SPRITE_STRIP
extern __inline__
#endif
void LoadTrialSpriteStrip(ArcFile *archive, GsSPRITE *sprites,
                          short image_index, short frame_count)
{
    GsIMAGE image;
    u_long *tim;
    Sprite3D *sprite;
    short i;

    tim = get_tim_from_archive(archive, image_index);
    GetTIMInfo(tim, &image);
    LoadTIM(tim);
    if (frame_count)
        image.ph /= frame_count;
    sprite = SetupSprite(NULL, &image);
    sprites[0] = sprite->sprite;
    vfree(sprite);
    for (i = 0; i < frame_count; i++)
    {
        sprites[i] = sprites[0];
        sprites[i].v += i * image.ph;
    }
}

#endif
