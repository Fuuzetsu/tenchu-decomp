#include "common.h"
#include "frontend.h"
#include "../main.exe/tuning.h"
#include "../main.exe/graphics.h"
#include "../main.exe/images.h"
#include "../main.exe/tim.h"
#include "../main.exe/vmemory.h"
#include <psxsdk/libgpu.h>
#include <psxsdk/libsnd.h>

extern char path_ending_fade[];

void GsInitGraph2(unsigned short width, unsigned short height,
                 unsigned short mode, unsigned short dither,
                 unsigned short rgb24);

void DrawEndingBackground(short brightness)
{
    GsIMAGE image;
    u_long *data;
    GsSPRITE *sprite;
    int half_width = SCREEN_W / 2;
    int half_height;

    if (EndingWideMode != 0)
        half_width = SCREEN_W;
    half_height = SCREEN_H / 2;

    if (EndingFadeSprite == 0)
    {
        data = FileRead(path_ending_fade);
        GetTIMInfo(data, &image);
        LoadTIMAndFree(data);
        EndingFadeSprite = SetupSprite(0, &image);
        EndingFadeSprite->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
    }

    EndingFadeSprite->sprite.attribute &= ~GS_ATTR_SEMITRANS_MASK;
    if (brightness > 0)
        EndingFadeSprite->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
    else
        EndingFadeSprite->sprite.attribute |= GS_ATTR_SEMITRANS_ADD;

    sprite = &EndingFadeSprite->sprite;
    sprite->r = sprite->g = sprite->b = __builtin_abs(brightness);
    for (sprite->y = -half_height;
         sprite->y <= half_height + sprite->h;
         sprite->y += sprite->h)
    {
        for (sprite->x = -half_width;
             sprite->x <= half_width;
             sprite->x += sprite->w)
        {
            GsSortSprite(sprite, OTablePt, 1);
        }
    }
}

BackGround *LoadEndingBackground(char *path)
{
    GsIMAGE image;
    u_long *data;
    BackGround *background;
    int i;

    data = FileRead(path);
    GetTIMInfo(data, &image);
    LoadTIMAndFree(data);
    background = SetupBG(&image, SCREEN_W, SCREEN_H);
    for (i = background->map.ncellw * background->map.ncellh - 1;
         i >= 0; i--)
    {
        background->index[i] = i;
    }
    background->sz = 2;
    return background;
}

void FadeEndingScreen(int step, int hold_frames)
{
    int brightness = ENDING_FADE_MAX;
    int phase = 0;
    int elapsed;

    SetDispMask(1);
    do
    {
        StartDrawing();
        EndingDrawCallback(0);
        DrawEndingBackground(brightness);
        switch (phase)
        {
        case 0:
            brightness -= step;
            if (brightness <= 0)
            {
                brightness = 0;
                phase = 1;
                elapsed = 0;
            }
            break;
        case 1:
            if (++elapsed > hold_frames)
                phase = 2;
            break;
        case 2:
            brightness += step;
            if (brightness >= ENDING_FADE_MAX)
            {
                brightness = ENDING_FADE_MAX;
                phase = 3;
                elapsed = 0;
            }
            break;
        }
        EndDrawing(0);
        if (phase != 2 && (GetRealPad(0) & ENDING_SKIP_BUTTONS))
            phase = 2;
    } while (phase != 3);
    while (GetRealPad(0) != 0)
    {
    }
    ClearEndingPages();
    SetDispMask(0);
}

void PlayEndingMovie(char *path, int frames)
{
    int frame;
    RECT rect;

    SetDispMask(0);
    GsInitGraph2(SCREEN_W, SCREEN_H, 4, 0, 1);
    SsSetMVol(MASTER_VOLUME_MAX, MASTER_VOLUME_MAX);
    rect.x = 0;
    rect.y = 0;
    rect.w = SCREEN_W * 3 / 2;
    rect.h = 512;
    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);
    open_stream(path, 0, 0, 0x1c0, 1);
    SetDispMask(1);
    do
    {
        StartDrawing();
        frame = DecodeMovieFrame();
        SkipFrame = SKIPFRAME_AFTER_LOAD;
        EndDrawing(0);
    } while (frame < frames && !(GetRealPad(0) & ENDING_SKIP_BUTTONS));

    SsSetMVol(0, 0);
    close_stream();
    while (GetRealPad(0) != 0)
    {
    }
    ClearEndingPages();
    GsInitGraph2(SCREEN_W, SCREEN_H, 4, 1, 0);
    GsInit3D();
    ClearEndingPages();
    SetDispMask(0);
}

void ClearEndingPages(void)
{
    StartDrawing();
    EndDrawing(2);
    StartDrawing();
    EndDrawing(2);
}

/* ENDING string storage. Keep the retail spelling and development paths. */
char path_ending_fade[] = "K:\\WORK\\CDIMAGE\\DEMO\\start\\fadeio.tim";
