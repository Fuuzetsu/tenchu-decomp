#include "common.h"
#include "mission.h"
#include "../main.exe/graphics.h"
#include "../main.exe/images.h"
#include "../main.exe/music.h"
#include "../main.exe/score.h"
#include "../main.exe/tim.h"
#include "../main.exe/vmemory.h"

void GsInitGraph2(unsigned short width, unsigned short height,
                 unsigned short mode, unsigned short dither,
                 unsigned short rgb24);

short SelectMenuMission(void)
{
    short phase = 1;
    short fade = 128;
    short result;
    RECT clear = {0, 0, SCREEN_W * 2, SCREEN_H * 2};

    InitMissionSelection();
    SetDispMask(0);
    ClearImage(&clear, 0, 0, 0);
    DrawSync(0);
    GsInitGraph2(640, SCREEN_H, 4, 1, 0);
    GsInit3D();
    MENU_STATE->GameRetry |= MENU_SELECT_LAYOUT;
    LoadMissionImages();
    _PlayMusic(15, 1);
    ClearMenuPages();
    DrawSync(0);
    SetDispMask(1);
    do
    {
        StartDrawing();
        switch (phase)
        {
        case 1:
            fade -= 8;
            if (fade <= 0)
            {
                phase++;
                fade = 0;
                GameClock = 0;
            }
            DrawMenuBackground(fade);
            break;
        case 2:
            result = UpdateMissionSelection();
            if (result != 1)
                phase++;
            DrawMissionSelection();
            break;
        case 3:
            fade += 8;
            if (fade >= 128)
            {
                fade = 128;
                phase = 0;
            }
            DrawMenuBackground(fade);
            DrawMissionSelection();
            break;
        default:
            phase = -1;
            break;
        }
        DrawMenuImageGrid(MissionBackground, 20);
        SkipFrame = SKIPFRAME_AFTER_LOAD;
        EndDrawing(2);
    } while (phase != 0);
    CdaStop();
    ClearMenuPages();
    DisposeMissionSelection();
    GsInitGraph2(SCREEN_W, SCREEN_H, 4, 1, 0);
    GsInit3D();
    ClearMenuPages();
    SetDispMask(0);
    return result;
}

void InitMissionSelection(void)
{
    int i, j;
    u8 character;
    int completed;
    short best = -1;
    ScoreStats *stats;
    ScoreResult *result;

    MissionPreview = 0;
    MissionBanner = 0;
    MissionBackground = 0;
    MissionTitles[0] = 0;
    MissionTitles[1] = 0;
    MissionCursor = 0;
    MissionOverallRank = 0;
    for (j = 0; j < N_STAGE_RANKS; j++)
        for (i = 0; i < N_STAGE_LAYOUTS; i++)
            MissionRankSprites[j][i] = 0;
    for (i = N_STAGE_LAYOUTS - 1; i >= 0; i--)
        MissionUnplayedSprites[i] = 0;

    character = MENU_STATE->CharType;
    MissionUnlocked = MENU_STATE->StageNoMAX[character];
    MissionSelection = 1;
    MissionLayoutSelection = 0;
    MissionInputReady = 0;
    GameClock = 0;
    MissionButtons = 0;
    MissionPressedButtons = 0;
    for (i = 0; i < MISSION_COUNT; i++)
    {
        completed = 0;
        for (j = 0; j < N_STAGE_LAYOUTS; j++)
        {
            stats = &MENU_STATE->stage_stats[character][MissionStageOrder[i + 1]][j];
            result = calculate_score(stats, MissionStageOrder[i + 1]);
            if (stats->stageBosses + stats->stageEnemies != 0)
            {
                MissionLayoutRanks[i][j] = result->grade;
                completed = 1;
                if (best < result->grade)
                    best = result->grade;
            }
            else
            {
                /* A completed layout unlocks the following layout. */
                if (completed)
                    MissionLayoutRanks[i][j] = MISSION_LAYOUT_UNPLAYED;
                else
                    MissionLayoutRanks[i][j] =
                        MENU_STATE->GameRetry & MENU_UNLOCK_LAYOUTS
                            ? MISSION_LAYOUT_UNPLAYED : MISSION_LAYOUT_LOCKED;
                completed = 0;
            }
        }
        MissionBestRanks[i] = best;
        best = -1;
    }
    MenuWideMode = 1;
}

void DisposeMissionSelection(void)
{
    int i, j;

    if (MissionPreview)
        vfree(MissionPreview);
    if (MissionTitles[0])
        vfree(MissionTitles[0]);
    if (MissionTitles[1])
        vfree(MissionTitles[1]);
    if (MissionCursor)
        vfree(MissionCursor);
    if (MissionOverallRank)
        vfree(MissionOverallRank);
    DisposeMenuImageGrid(MissionBanner);
    DisposeMenuImageGrid(MissionBackground);
    vfree(MissionImageArchive);
    for (j = 0; j < N_STAGE_RANKS; j++)
        for (i = 0; i < N_STAGE_LAYOUTS; i++)
            if (MissionRankSprites[j][i])
                vfree(MissionRankSprites[j][i]);
    for (i = 0; i < N_STAGE_LAYOUTS; i++)
        if (MissionUnplayedSprites[i])
            vfree(MissionUnplayedSprites[i]);
    MenuWideMode = 0;
}

void LoadMissionImages(void)
{
    MenuImageGrid *grid;

    MissionImageArchive = FileRead(MissionImagePaths[MENU_STATE->language]);
    MissionBackground = grid = valloc(sizeof(MenuImageGrid));
    grid->x = -320;
    grid->y = -120;
    grid->cell_width = 128;
    grid->cell_height = 240;
    grid->columns = 5;
    grid->rows = 1;
    grid->visible_columns = 5;
    grid->visible_rows = 1;
    SetupMenuImageGrid(grid, MISSION_IMAGE_MAP);
    MissionBanner = grid = valloc(sizeof(MenuImageGrid));
    grid->x = -299;
    grid->y = 30;
    grid->cell_width = 16;
    grid->cell_height = 68;
    grid->columns = 37;
    grid->rows = 1;
    grid->visible_columns = 2;
    grid->visible_rows = 1;
    SetupMenuImageGrid(grid, MENU_STATE->GameRetry & MENU_SELECT_LAYOUT
                                ? MISSION_IMAGE_LAYOUT_BANNER : MISSION_IMAGE_RANK_BANNER);
    SetupMissionSprites();
}

void SetupMissionSprites(void)
{
    GsIMAGE image;
    /* The retail frame reserves these bytes without reading or writing them. */
    unsigned char unused[128];
    u_long *data;
    GsSPRITE *sprite;
    int rank, layout;

    data = get_tim_from_archive(MissionImageArchive, MISSION_IMAGE_PREVIEW_FIRST);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    MissionPreview = SetupSprite(0, &image);
    sprite = &MissionPreview->sprite;
    sprite->x = -190;
    sprite->y = 65;
    sprite->attribute |= 0x08000000;
    data = get_tim_from_archive(MissionImageArchive, MISSION_IMAGE_CURSOR);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    MissionCursor = SetupSprite(0, &image);
    sprite = &MissionCursor->sprite;
    sprite->x = 35;
    sprite->y = -250;
    sprite->attribute |= GS_ATTR_BLEND_MODE(GPU_BLEND_ADD);
    data = get_tim_from_archive(MissionImageArchive, MISSION_IMAGE_TITLE_FIRST);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    MissionTitles[0] = SetupSprite(0, &image);
    sprite = &MissionTitles[0]->sprite;
    sprite->w = 256;
    sprite->h = 12;
    sprite->mx = 0;
    sprite->my = 0;
    sprite->x = -116;
    sprite->y = 40;
    sprite->attribute |= 0x08000000;
    MissionTitles[1] = SetupSprite(MissionTitles[0], &image);
    sprite = &MissionTitles[1]->sprite;
    sprite->w = 136;
    sprite->h = 12;
    sprite->mx = 0;
    sprite->my = 0;
    sprite->x = 140;
    sprite->y = 40;
    sprite->tpage++;
    sprite->attribute |= 0x08000000;
    for (rank = 0; rank < N_STAGE_RANKS; rank++)
    {
        data = get_tim_from_archive(MissionImageArchive, MISSION_IMAGE_RANK_LAST - rank);
        GetTIMInfo(data, &image);
        LoadTIM(data);
        for (layout = 0; layout < N_STAGE_LAYOUTS; layout++)
        {
            MissionRankSprites[rank][layout] = SetupSprite(MissionRankSprites[rank][0], &image);
            sprite = &MissionRankSprites[rank][layout]->sprite;
            sprite->x = -42 + layout * 120;
            sprite->y = 77;
            sprite->attribute |= 0x08000000 | GS_ATTR_SEMITRANS(GPU_BLEND_ADD_QUARTER);
        }
    }
    data = get_tim_from_archive(MissionImageArchive, MISSION_IMAGE_UNPLAYED);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    for (layout = 0; layout < N_STAGE_LAYOUTS; layout++)
    {
        MissionUnplayedSprites[layout] = SetupSprite(MissionUnplayedSprites[0], &image);
        sprite = &MissionUnplayedSprites[layout]->sprite;
        sprite->x = -42 + layout * 120;
        sprite->y = 75;
        sprite->attribute |= 0x08000000 | GS_ATTR_SEMITRANS(GPU_BLEND_ADD_QUARTER);
    }
    data = get_tim_from_archive(MissionImageArchive, MissionBestRanks[0] + MISSION_IMAGE_OVERALL_RANK_FIRST);
    GetTIMInfo(data, &image);
    LoadTIM(data);
    MissionOverallRank = SetupSprite(0, &image);
    sprite = &MissionOverallRank->sprite;
    sprite->x = 78;
    sprite->y = 73;
    sprite->attribute |= 0x08000000;
}

void DrawMissionSelection(void)
{
    int i;
    short x, y, width, bottom;
    POLY_FT4 *polygons, *polygon;
    GsSPRITE *sprite;
    MissionCursorPosition *position;

    if (GameClock == 1)
    {
        MissionBannerX = 197;
        MissionBannerY = 30;
        MissionFirstBannerColumn = 34;
        MissionFirstRank = 3;
        MissionFirstTitle = 2;
        MissionRevealFlag = 1;
    }
    else if (GameClock < 15)
    {
        MissionBannerX = MissionBannerX - 80 > -299 ? (short)(MissionBannerX - 80) : -299;
        MissionFirstBannerColumn = MissionFirstBannerColumn - 5 > 3
                                       ? (short)(MissionFirstBannerColumn - 5) : 3;
    }
    else
        MissionInputReady = 1;

    if (MissionFirstBannerColumn < 6)
        GsSortSprite(&MissionPreview->sprite, OTablePt, 5);
    if (MissionFirstBannerColumn < 16)
        MissionFirstTitle = 0;
    else if (MissionFirstBannerColumn < 20)
        MissionFirstTitle = 1;
    if (MissionFirstBannerColumn < 18)
        MissionFirstRank = 0;
    else if (MissionFirstBannerColumn < 23)
        MissionFirstRank = 1;
    else if (MissionFirstBannerColumn < 28)
        MissionFirstRank = 2;

    for (i = MissionFirstTitle; i < 2; i++)
        GsSortSprite(&MissionTitles[i]->sprite, OTablePt, 5);
    if (MENU_STATE->GameRetry & MENU_SELECT_LAYOUT)
    {
        for (i = MissionFirstRank; i < N_STAGE_LAYOUTS; i++)
            if (MissionVisibleRanks[i])
                GsSortSprite(MissionVisibleRanks[i], OTablePt, 5);
    }
    else if (MissionFirstRank <= 0)
        GsSortSprite(&MissionOverallRank->sprite, OTablePt, 5);

    x = MissionBannerX;
    y = MissionBannerY;
    width = MissionBanner->cell_width;
    bottom = y + MissionBanner->cell_height;
    polygons = MissionBanner->polygons;
    for (i = 0; i < 3; i++)
    {
        polygon = &polygons[i];
        setXY4(polygon, x, y, x + width, y, x, bottom, x + width, bottom);
        x += width;
        GsSortPoly(polygon, OTablePt, 8);
    }
    for (i = MissionFirstBannerColumn; i < 37; i++)
        GsSortPoly(&polygons[i], OTablePt, 8);

    sprite = &MissionCursor->sprite;
    sprite->r = sprite->r + 16 > 208 ? 80 : sprite->r + 16;
    sprite->g = sprite->b = sprite->r;
    position = &MissionCursorTargets[MissionSelection - 1];
    if (sprite->x != position->x)
    {
        sprite->x = sprite->x - 10 < position->x ? position->x : sprite->x - 10;
        {
            short cursor_y;

            if (sprite->y + 50 <= position->y)
                cursor_y = sprite->y + 50;
            else
                cursor_y = position->y;
            sprite->y = cursor_y;
        }
        if (sprite->x == position->x)
            Sound(0, 6);
    }
    GsSortSprite(&MissionCursor->sprite, OTablePt, 15);
}

short UpdateMissionSelection(void)
{
    short mission;
    short result = 1;
    int j;
    int requested_layout;
    short rank;
    GsSPRITE *sprite;
    short *ranks;

    MissionPressedButtons = MissionButtons;
    MissionButtons = GetRealPad(0);
    MissionPressedButtons = (MissionPressedButtons ^ MissionButtons) & MissionButtons;
    if (MissionPressedButtons && MissionInputReady)
    {
        switch ((short)check_cheat_command_((short)MissionButtons, (short)MissionPressedButtons))
        {
        case 0x100:
            MissionUnlocked = MISSION_COUNT;
            MENU_STATE->StageNoMAX[MENU_STATE->CharType] = MISSION_COUNT;
            if (MENU_STATE->CharType)
                Sound(0, 8);
            else
                Sound(0, 7);
            break;
        case 0x200:
            MENU_STATE->GameRetry |= 8;
            if (MENU_STATE->CharType)
                Sound(0, 8);
            else
                Sound(0, 7);
            break;
        case 0x400:
            for (mission = 0; mission < MISSION_COUNT; mission++)
                for (j = 0; j < N_STAGE_LAYOUTS; j++)
                    if (MissionLayoutRanks[mission][j] == MISSION_LAYOUT_LOCKED)
                        MissionLayoutRanks[mission][j] = MISSION_LAYOUT_UNPLAYED;
            LoadTIM(get_tim_from_archive(MissionImageArchive, MISSION_IMAGE_LAYOUT_BANNER));
            MENU_STATE->GameRetry |= MENU_UNLOCK_LAYOUTS;
            if (MENU_STATE->CharType)
                Sound(0, 8);
            else
                Sound(0, 7);
            break;
        }

        mission = MissionSelection;
        if (MissionButtons == PADLup)
        {
            MissionSelection--;
            if (MissionSelection <= 0)
                MissionSelection = 1;
            else
            {
                LoadTIM(get_tim_from_archive(MissionImageArchive,
                    MissionBestRanks[MissionSelection - 1] + MISSION_IMAGE_OVERALL_RANK_FIRST));
                Sound(0, 3);
            }
        }
        else if (MissionButtons == PADLdown)
        {
            MissionSelection++;
            if (MissionSelection == MISSION_COUNT + 1)
                MissionSelection = MISSION_COUNT;
            else if (MissionUnlocked + 1 < MissionSelection)
                MissionSelection = MissionUnlocked + 1;
            else
            {
                LoadTIM(get_tim_from_archive(MissionImageArchive,
                    MissionBestRanks[MissionSelection - 1] + MISSION_IMAGE_OVERALL_RANK_FIRST));
                Sound(0, 3);
            }
        }
        else if (MissionButtons == PADLleft)
        {
            MissionLayoutSelection--;
            if (MissionLayoutSelection < 0)
            {
                MissionLayoutSelection = 0;
                MissionButtons = 0;
            }
        }
        else if (MissionButtons == PADLright)
        {
            MissionLayoutSelection++;
            if (MissionLayoutSelection >= N_STAGE_LAYOUTS)
            {
                MissionLayoutSelection = N_STAGE_LAYOUTS - 1;
                MissionButtons = 0;
            }
        }

        if (MissionButtons & (PADLup | PADLdown | PADLleft | PADLright))
        {
            requested_layout = MissionLayoutSelection;
            while (MissionLayoutSelection != 0 &&
                   MissionLayoutRanks[MissionSelection - 1][MissionLayoutSelection] == MISSION_LAYOUT_LOCKED)
            {
                MissionLayoutSelection--;
                requested_layout += 10;
            }
            if ((MissionButtons == PADLleft || MissionButtons == PADLright) &&
                requested_layout == MissionLayoutSelection)
                Sound(0, 5);
        }
        else if (MissionButtons == PADRright)
        {
            result = 0;
            Sound(0, 4);
            MENU_STATE->StageNo = MissionStageOrder[MissionSelection];
            MENU_STATE->layout = MissionLayoutSelection;
        }
        else if (MissionButtons == PADRdown)
        {
            Sound(0, 4);
            result = -1;
        }

        if (mission != MissionSelection)
        {
            LoadTIM(get_tim_from_archive(MissionImageArchive,
                MissionSelection + MISSION_IMAGE_PREVIEW_FIRST - 1));
            LoadTIM(get_tim_from_archive(MissionImageArchive,
                MissionSelection + MISSION_IMAGE_TITLE_FIRST - 1));
            MissionCursor->sprite.x = MissionCursorStarts[MissionSelection - 1].x;
            MissionCursor->sprite.y = MissionCursorStarts[MissionSelection - 1].y;
        }
    }

    ranks = MissionLayoutRanks[MissionSelection - 1];
    for (j = 0; j < N_STAGE_LAYOUTS; j++)
    {
        rank = *ranks;
        if (rank >= 0)
            sprite = &MissionRankSprites[rank][j]->sprite;
        else
            sprite = &MissionUnplayedSprites[j]->sprite;
        if (j == MissionLayoutSelection)
            sprite->attribute &= ~GS_ATTR_SEMITRANS_ENABLE;
        else
            sprite->attribute |= GS_ATTR_SEMITRANS_ENABLE;
        MissionVisibleRanks[j] = sprite;
        ranks++;
    }
    return result;
}
