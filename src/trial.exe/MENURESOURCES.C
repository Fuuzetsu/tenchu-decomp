#include "common.h"
#include "mission_menu.h"
#include "../main.exe/images.h"
#include "../main.exe/timpack.h"
#include "../main.exe/vmemory.h"

extern char path_trial_previews[];

Sprite3D *TrialMissionOriginIcons[3] = {NULL, NULL, NULL};
Sprite3D *TrialConfirmationSprites[TRIAL_CONFIRM_SPRITES] = {NULL};
TrialOptionArrowPair TrialOptionArrowPositions[TRIAL_OPTION_COUNT] = {
    {30, 0, 45, 0},
    {80, 24, 95, 24},
    {-5, 45, 10, 45}
};
s16 TrialOptionLimits[TRIAL_OPTION_COUNT] = {11, 4, 1};

void LoadTrialPreviewImages(void)
{
    TrialPreviewArchive = FileRead(path_trial_previews);
}

int PrepareTrialMissionPreview(TrialMissionHeader *header, TrialMissionPreview *preview)
{
    if (header->checksum)
    {
        FormatTrialMissionPreview(header, preview);
        return 1;
    }
    return 0;
}

void LoadTrialThemePreview(TrialMissionPreview *preview)
{
    GsIMAGE image;
    u_long *data;

    data = get_tim_from_archive(TrialPreviewArchive, preview->theme);
    GetTIMInfo(data, &image);
    LoadTIM(data);
}

void DrawTrialMissionOrigin(TrialMissionPreview *preview, short x, short y, int priority)
{
    GsSPRITE *icon;
    int origin = (preview->id < TRIAL_PRESET_COUNT) * 2;

    icon = &TrialMissionOriginIcons[origin]->sprite;
    icon->y = y;
    icon->x = x;
    SortTrialSpritePrimitive(icon, 0, OTablePt, priority);
}

void DrawTrialListArrow(short down, short x, short y, u16 priority)
{
    GsSPRITE sprite = TrialListArrows->sprite;

    sprite.x = x;
    if (down)
    {
        sprite.y = y;
        sprite.v += 16;
    }
    else
    {
        sprite.y = y;
    }
    GsSortSprite(&sprite, OTablePt, priority);
}

/* Only the frame is clamped: both sprites otherwise move together. */
int SlideTrialMissionDetails(short direction)
{
    int finished = 0;

    switch (direction)
    {
    case TRIAL_PANEL_HIDE:
        TrialMissionDetailsFrame->sprite.x += 50;
        TrialThemePreviewSprite->sprite.x += 50;
        if (TrialMissionDetailsFrame->sprite.x >= 444)
        {
            TrialMissionDetailsFrame->sprite.x = 444;
            finished = 1;
        }
        break;
    case TRIAL_PANEL_SHOW:
        TrialMissionDetailsFrame->sprite.x -= 50;
        TrialThemePreviewSprite->sprite.x -= 50;
        if (TrialMissionDetailsFrame->sprite.x <= 144)
        {
            TrialMissionDetailsFrame->sprite.x = 144;
            finished = 1;
        }
        break;
    }
    GsSortSprite(&TrialMissionDetailsFrame->sprite, OTablePt, 10);
    GsSortSprite(&TrialThemePreviewSprite->sprite, OTablePt, 8);
    return finished;
}

void ResetTrialCharacterGraphics(void)
{
    int i;

    TrialCharacterBackground = NULL;
    for (i = N_PLAYABLE_CHARACTERS - 1; i >= 0; i--)
        TrialCharacterPortraits[i] = NULL;
}

void DisposeTrialCharacterGraphics(void)
{
    int i;

    if (TrialCharacterBackground)
        DisposeBG(TrialCharacterBackground);
    for (i = 0; i < N_PLAYABLE_CHARACTERS; i++)
        if (TrialCharacterPortraits[i])
            vfree(TrialCharacterPortraits[i]);
}

void LoadTrialCharacterGraphics(void)
{
    int i;
    GsSPRITE *portrait;

    TrialCharacterBackground = load_background_(get_tim_from_archive(TrialImageArchive, 11));
    TrialCharacterBackground->sz = 200;
    for (i = 0; i < N_PLAYABLE_CHARACTERS; i++)
        TrialCharacterPortraits[i] = load_archive_sprite_(TrialImageArchive, i + 12);
    MODEL_POSITION(TrialCharacterPortraits[0])->vx = 0;
    MODEL_POSITION(TrialCharacterPortraits[0])->vy = 20;
    MODEL_POSITION(TrialCharacterPortraits[0])->vz = 300;
    portrait = &TrialCharacterPortraits[1]->sprite;
    portrait->b = 64;
    portrait->g = 64;
    portrait->r = 64;
    MODEL_POSITION(TrialCharacterPortraits[1])->vx = 600;
    MODEL_POSITION(TrialCharacterPortraits[1])->vy = 20;
    MODEL_POSITION(TrialCharacterPortraits[1])->vz = 1100;
}

void ResetTrialConfirmationGraphics(void)
{
    int i;

    TrialConfirmationBackground = NULL;
    for (i = TRIAL_CONFIRM_SPRITES - 1; i >= 0; i--)
        TrialConfirmationSprites[i] = NULL;
}

void DisposeTrialConfirmationGraphics(void)
{
    int i;

    if (TrialConfirmationBackground)
        DisposeBG(TrialConfirmationBackground);
    for (i = 0; i < TRIAL_CONFIRM_SPRITES; i++)
        if (TrialConfirmationSprites[i])
            vfree(TrialConfirmationSprites[i]);
}

void DrawTrialOptionArrows(short row, short value, short first_row_maximum)
{
    if (row >= TRIAL_OPTION_FIRST_ROW)
    {
        if (row == TRIAL_OPTION_FIRST_ROW)
            TrialOptionLimits[0] = first_row_maximum;
        if (value)
        {
            TrialOptionArrows[0]->sprite.x = TrialOptionArrowPositions[row - TRIAL_OPTION_FIRST_ROW].left_x;
            TrialOptionArrows[0]->sprite.y = TrialOptionArrowPositions[row - TRIAL_OPTION_FIRST_ROW].left_y;
            GsSortSprite(&TrialOptionArrows[0]->sprite, OTablePt, 10);
        }
        if (value != TrialOptionLimits[row - TRIAL_OPTION_FIRST_ROW])
        {
            TrialOptionArrows[1]->sprite.x = TrialOptionArrowPositions[row - TRIAL_OPTION_FIRST_ROW].right_x;
            TrialOptionArrows[1]->sprite.y = TrialOptionArrowPositions[row - TRIAL_OPTION_FIRST_ROW].right_y;
            GsSortSprite(&TrialOptionArrows[1]->sprite, OTablePt, 10);
        }
    }
}

/* Theme zero is always available; persistent progress flags unlock 1..11.
 * The return count excludes the default entry. */
short GetTrialUnlockedThemes(short themes[TRIAL_THEME_COUNT])
{
    u32 bit = 1;
    short count = 0;
    int i;

    memset(themes, 0, TRIAL_THEME_COUNT * sizeof(*themes));
    themes[0] = 0;
    for (i = 0; i < TRIAL_THEME_COUNT - 1; i++, bit <<= 1)
    {
        if (TRIAL_STATE->mission_flags & bit)
            themes[++count] = i + 1;
    }
    return count;
}
