#include "common.h"
#include "trial.exe.h"
#include "drawing.h"
#include "mission.h"
#include "../main.exe/images.h"
#include "../main.exe/tim.h"
#include "../main.exe/vmemory.h"

void SetupTrialImagePanel(TrialImagePanel *grid, ArcFile *archive, short image_index)
{
    short mode;
    u_short columns;
    u_short rows;
    u_long *data;
    GsIMAGE *image;
    POLY_FT4 *polygon = (POLY_FT4 *)TENCHU_SCRATCHPAD(0);
    short x;
    short y;
    short sx;
    short sy;
    short width;
    short height;
    short tx;
    short ty;
    short u;

    columns = grid->columns;
    rows = grid->rows;
    image = &grid->image;
    grid->tiles = valloc((columns * sizeof(POLY_FT4)) * rows);
    data = get_tim_from_archive(archive, image_index);
    GetTIMInfo(data, image);
    LoadTIM(data);
    mode = TIM_PIXEL_MODE(image->pmode);
    sy = grid->y;
    width = grid->tile_width;
    height = grid->tile_height;
    for (y = 0; y < rows; y++)
    {
        sx = grid->x;
        for (x = 0; x < columns; x++)
        {
            setPolyFT4(polygon);
            setRGB0(polygon, 128, 128, 128);
            setXYWH(polygon, sx, sy, width, height);
            tx = image->px + ((width * x) >> (2 - mode));
            ty = image->py + height * y;
            setClut(polygon, image->cx, image->cy);
            setTPage(polygon, mode, 0, tx, ty);
            u = tx << (2 - mode);
            u &= (1 << (8 - mode)) - 1;
            setUV4(polygon, u, ty, u + width - 1, ty,
                   u, ty + height - 1, u + width - 1, ty + height - 1);
            grid->tiles[y * columns + x] = *polygon;
            sx += width;
        }
        sy += height;
    }
}

/* The number atlas has digits 0..9, a minus sign, and a colon. */
void DrawTrialTime(GsSPRITE *sprite, s32 ticks, s32 x, s32 y)
{
    int i;

    if (ticks >= 0)
    {
        ticks /= 30;
        DrawTrialNumberInline(sprite, ticks / 60,
                              x - (sprite->w * 2 + (sprite->w >> 1)), y);
        ticks %= 60;
        DrawTrialNumberInline(sprite, ticks / 10, x - sprite->w, y);
        DrawTrialNumberInline(sprite, ticks % 10, x, y);
    }
    else
    {
        sprite->y = y;
        for (i = 0; i < 2; i++)
        {
            sprite->x = x - sprite->w * i;
            SortTrialSpriteFrame(sprite, 10);
        }
        for (i = 0; i < 2; i++)
        {
            sprite->x = x - sprite->w * i -
                (sprite->w * 2 + (sprite->w >> 1));
            SortTrialSpriteFrame(sprite, 10);
        }
    }
    sprite->x = x - sprite->w * 2;
    SortTrialSpriteFrame(sprite, 11);
}

/* Lower times rank first; an equal time is inserted before the old entry. */
s32 InsertTrialRecord(s32 ticks)
{
    TrialMissionRecord *records;
    s32 rank;
    s32 i;

    records = TrialMission.header.records;
    rank = -1;
    if (ticks <= records[TRIAL_MISSION_RECORD_COUNT - 1].time ||
        records[TRIAL_MISSION_RECORD_COUNT - 1].time == -1)
    {
        for (i = 0; i < TRIAL_MISSION_RECORD_COUNT; i++)
        {
            if (records[i].time == -1 || ticks <= records[i].time)
            {
                rank = i;
                break;
            }
        }
        if (rank >= 0)
        {
            for (i = TRIAL_MISSION_RECORD_COUNT - 1; i > rank; i--)
            {
                records[i].time = records[i - 1].time;
                records[i].name = records[i - 1].name;
            }
            records[rank].time = ticks;
            memset(records[rank].name.text, 0, sizeof(records[rank].name.text));
        }
    }
    return rank;
}
