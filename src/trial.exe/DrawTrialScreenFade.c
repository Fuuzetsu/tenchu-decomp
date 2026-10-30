#include "common.h"
#include "trial.exe.h"
#include "../main.exe/graphics.h"
#include "../main.exe/images.h"
#include "../main.exe/tim.h"

extern Sprite3D *TrialFadeSprite;
const char path_trial_fade[] = "K:\\WORK\\CDIMAGE\\DEMO\\start\\fadeio.tim";

void DrawTrialScreenFade(short brightness, short width, short height, short release)
{
    GsIMAGE image;
    u_long *data;
    GsSPRITE *sprite;
    short half_width = width / 2;
    short half_height = height / 2;

    if (release)
    {
        if (TrialFadeSprite)
            vfree(TrialFadeSprite);
    }
    else
    {
        if (!TrialFadeSprite)
        {
            data = FileRead(path_trial_fade);
            GetTIMInfo(data, &image);
            LoadTIMAndFree(data);
            TrialFadeSprite = SetupSprite(NULL, &image);
            TrialFadeSprite->sprite.mx = TrialFadeSprite->sprite.my = 0;
            TrialFadeSprite->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
        }
        sprite = &TrialFadeSprite->sprite;
        TrialFadeSprite->sprite.attribute &= ~GS_ATTR_SEMITRANS_MASK;
        if (brightness > 0)
            TrialFadeSprite->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
        else
            TrialFadeSprite->sprite.attribute |= GS_ATTR_SEMITRANS_ADD;
        sprite->r = sprite->g = sprite->b = __builtin_abs(brightness);
        for (sprite->y = -half_height; sprite->y < half_height;
             sprite->y += sprite->h)
        {
            for (sprite->x = -half_width; sprite->x < half_width;
                 sprite->x += sprite->w)
                GsSortSprite(sprite, OTablePt, 0);
        }
    }
}
