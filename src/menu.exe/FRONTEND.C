#include "common.h"
#include "frontend.h"
#include "../main.exe/tuning.h"
#include "../main.exe/graphics.h"
#include "../main.exe/images.h"
#include "../main.exe/tim.h"
#include "../main.exe/vmemory.h"
#include <psxsdk/libgpu.h>
#include <psxsdk/libsnd.h>

extern char path_menu_fade[];

void GsInitGraph2(unsigned short width, unsigned short height,
                 unsigned short mode, unsigned short dither,
                 unsigned short rgb24);

void DrawMenuBackground(short brightness)
{
    GsIMAGE image;
    u_long *data;
    GsSPRITE *sprite;
    int half_width = SCREEN_W / 2;
    int half_height;

    if (MenuWideMode != 0)
        half_width = SCREEN_W;
    half_height = SCREEN_H / 2;

    if (MenuFadeSprite == 0)
    {
        data = FileRead(path_menu_fade);
        GetTIMInfo(data, &image);
        LoadTIMAndFree(data);
        MenuFadeSprite = SetupSprite(0, &image);
        MenuFadeSprite->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
    }

    MenuFadeSprite->sprite.attribute &= ~GS_ATTR_SEMITRANS_MASK;
    if (brightness > 0)
        MenuFadeSprite->sprite.attribute |= GS_ATTR_SEMITRANS_SUBTRACT;
    else
        MenuFadeSprite->sprite.attribute |= GS_ATTR_SEMITRANS_ADD;

    sprite = &MenuFadeSprite->sprite;
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

BackGround *LoadMenuBackground(char *path)
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

void FadeMenuScreen(int step, int hold_frames)
{
    short buttons = -1;
    short previous;
    int brightness = MENU_FADE_MAX;
    int phase = 0;
    int elapsed;

    SetDispMask(1);
    do
    {
        StartDrawing();
        MenuDrawCallback(0);
        DrawMenuBackground(brightness);
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
            if (brightness >= MENU_FADE_MAX)
            {
                brightness = MENU_FADE_MAX;
                phase = 3;
                elapsed = 0;
            }
            break;
        }
        EndDrawing(0);
        previous = buttons;
        buttons = GetRealPad(0);
        if (phase != 2 && previous == 0 && (buttons & MENU_SKIP_BUTTONS))
            phase = 2;
    } while (phase != 3);
    ClearMenuPages();
    SetDispMask(0);
}

void PlayOpeningMovie(char *path, int frames)
{
    short buttons = -1;
    short previous;
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
        frame = DecodeMovieFrame();
        VSync(0);
        GsSwapDispBuff();
        previous = buttons;
        buttons = GetRealPad(0);
    } while (frame < frames &&
             (previous != 0 || !(buttons & MENU_SKIP_BUTTONS)));

    SsSetMVol(0, 0);
    close_stream();
    ClearMenuPages();
    GsInitGraph2(SCREEN_W, SCREEN_H, 4, 1, 0);
    GsInit3D();
    ClearMenuPages();
    SetDispMask(0);
}

void ClearMenuPages(void)
{
    StartDrawing();
    EndDrawing(2);
    StartDrawing();
    EndDrawing(2);
}

void EmptyMenuCallback(int unused)
{
}
