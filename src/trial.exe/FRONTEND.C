#include "common.h"
#include "trial.exe.h"
#include "../main.exe/graphics.h"
#include "../main.exe/images.h"
#include "../main.exe/timpack.h"
#include <psxsdk/libgpu.h>

void ClearTrialPages(void)
{
    StartDrawing();
    EndDrawing(2);
    StartDrawing();
    EndDrawing(2);
}

void SetupTrialSprite(GsSPRITE *sprite, u_long *data, int x, int y,
                      int brightness, u32 attributes)
{
    GsIMAGE image;

    GetTIMInfo(data, &image);
    InitSprite(&image, sprite);
    sprite->x = x;
    sprite->y = y;
    sprite->mx = 0;
    sprite->my = 0;
    sprite->attribute |= attributes;
    sprite->b = brightness;
    sprite->g = brightness;
    sprite->r = brightness;
}

/* Frames occupy consecutive rows of the same texture page. */
void DrawTrialSpriteFrame(GsSPRITE *sprite, int frame)
{
    u8 first_v;

    first_v = sprite->v;
    sprite->v += frame * sprite->h;
    GsSortSprite(sprite, OTablePt, 2);
    sprite->v = first_v;
}
