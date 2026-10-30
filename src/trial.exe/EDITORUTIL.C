#include "common.h"
#include "mission_menu.h"
#include "../main.exe/images.h"

u16 TrialDefaultTiles[TRIAL_THEME_COUNT] = {
    0, 0, 0, 0, 11, 0, 0, 1, 0, 0, 0, 0
};

void InitNewTrialMission(void)
{
    memset(&TrialMission, 0, sizeof(TrialMission));
    TrialMission.header.flags = 0;
    TrialMission.header.theme = 0;
    FillTrialMissionMap();
    memset(TrialMission.header.records, 0xff, sizeof(TrialMission.header.records));
    SetupTrialObjectivePlacements(1);
}

/* The readable draft is four bytes short: row-index normalization differs
 * from the original. Keep the original function until this compiler pattern
 * is recovered; it is deliberately excluded from the matching-C count. */
#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/trial.exe/asm/nonmatchings/EDITORUTIL", FillTrialMissionMap);
#else
void FillTrialMissionMap(void)
{
    short x = 0;
    short z = 0;
    TrialMissionData *mission = &TrialMission;
    u16 tile = TrialDefaultTiles[mission->header.theme];

    do
    {
        mission->map[z][x][0] = tile;
        mission->map[z][x][1] = TRIAL_EMPTY_TILE;
        if (++x >= TRIAL_MAP_WIDTH)
        {
            x = 0;
            z++;
        }
    } while (z < TRIAL_MAP_HEIGHT);
}

#endif

/* The placement validator uses this terrain-kind classification: the five
 * regular support classes return 1, class 0xa0 returns 0, and others -1.
 * The same switch is inlined in the live placement-validation path. */
short ClassifyTrialTerrainSupport(short kind)
{
    switch (kind)
    {
    case 0x10:
    case 0x20:
    case 0x30:
    case 0x40:
    case 0x50:
        return 1;
    case 0xa0:
        return 0;
    default:
        return -1;
    }
}

/* Upload one TIM and expose its vertical frames as ordinary LIBGS sprites.
 * With zero frames, only the full-image sprite at index zero is initialized. */
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
