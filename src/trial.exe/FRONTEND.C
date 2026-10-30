#include "common.h"
#include "trial.exe.h"
#include "drawing.h"
#include "../main.exe/graphics.h"
#include "../main.exe/images.h"
#include "../main.exe/timpack.h"
#include "../main.exe/effect.h"
#include "../main.exe/game_globals.h"
#include "../main.exe/vmemory.h"
#include <psxsdk/libgpu.h>


void InitTrialImagePanel(TrialImagePanel *panel)
{
    if (panel)
    {
        panel->tiles = NULL;
        panel->x = -256;
        panel->y = -120;
        panel->columns = 4;
        panel->rows = 1;
        panel->tile_width = 128;
        panel->tile_height = 240;
        panel->draw_columns = 4;
        panel->draw_rows = 1;
    }
}

void DrawTrialImagePanel(TrialImagePanel *grid, u_short priority)
{
    POLY_FT4 *polygons;
    POLY_FT4 *polygon;
    u_short columns;
    u_short width;
    u_short height;
    short x;
    short y;

    polygons = grid->tiles;
    columns = grid->columns;
    height = grid->draw_rows;
    width = grid->draw_columns;
    for (y = 0; y < height; y++)
    {
        polygon = &polygons[y * columns];
        for (x = 0; x < width; x++)
        {
            GsSortPoly(polygon++, OTablePt, priority);
        }
    }
}

void DisposeTrialImagePanel(TrialImagePanel *panel)
{
    if (panel)
    {
        if (panel->tiles)
        {
            vfree(panel->tiles);
            panel->tiles = NULL;
        }
        vfree(panel);
    }
}

/* Keep the TIM-derived texture flags while applying a caller's extra flags. */
static inline void PlaceTrialSprite(GsSPRITE *sprite, int x, int y,
                                    int brightness, u32 attributes)
{
    sprite->x = x;
    sprite->y = y;
    sprite->mx = 0;
    sprite->my = 0;
    sprite->attribute |= attributes;
    sprite->b = brightness;
    sprite->g = brightness;
    sprite->r = brightness;
}

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
    PlaceTrialSprite(sprite, x, y, brightness, attributes);
}

void DrawTrialSpriteFrame(GsSPRITE *sprite, int frame)
{
    SortTrialSpriteFrame(sprite, frame);
}

void DrawTrialNumber(GsSPRITE *sprite, s16 value, s16 x, s16 y)
{
    DrawTrialNumberInline(sprite, value, x, y);
}

GsSPRITE *CreateTrialSprite(ArcFile *archive, int index)
{
    u_long *data;
    GsSPRITE *sprite;
    GsIMAGE image;

    data = get_tim_from_archive(archive, index);
    sprite = valloc(sizeof(*sprite));
    GetTIMInfo(data, &image);
    InitSprite(&image, sprite);
    PlaceTrialSprite(sprite, 0, 0, 128, 0);
    LoadTIM(data);
    return sprite;
}

void ReturnFromTrialToMenu(void)
{
    FadeOutDirect(32, 2, 8, 8, 8);
    clear_screen_();
    TRIAL_STATE->layout = STAGE_LAYOUT_RANDOM;
    TRIAL_STATE->GameRetry &= ~GAME_RETRY_REPLAY;
    exec_process_(PROCESS_MENU);
}
