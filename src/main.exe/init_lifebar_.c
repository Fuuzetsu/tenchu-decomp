#include "common.h"
#include "main.exe.h"
#include "images.h"

typedef struct
{
    s32 rotation;    /* +0x0, forwarded into both sprites verbatim */
    u8 frame_image;  /* +0x4 */
    u8 fill_image;   /* +0x5 */
} LifeBarSpriteEntry;

extern LifeBarSpriteEntry LifeBarParts[];

void init_lifebar_(void)
{
    u8 image[25];
    GsSPRITE *slot;
    s32 tmp;
    int i;

    for (i = 0; i < N_LIFE_BAR_STYLES; i++)
    {
        slot = &LifeBarStyle[i].frame;
        InitSprite(GetImage(LifeBarParts[i].frame_image), slot);
        slot->mx = 0;
        slot->my = 0;
        slot->rotate = LifeBarParts[i].rotation;
        slot->attribute = GS_ATTR_SEMITRANS_AVERAGE;

        slot = &LifeBarStyle[i].fill;
        InitSprite(GetImage(LifeBarParts[i].fill_image), slot);
        slot->mx = 0;
        slot->my = 0;
        tmp = LifeBarParts[i].rotation;
        slot->attribute = GS_ATTR_SEMITRANS_ADD;
        slot->rotate = tmp;
    }
}
