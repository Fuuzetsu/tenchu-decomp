#include "common.h"
#include "mission_menu.h"
#include "../main.exe/images.h"
#include "../main.exe/vmemory.h"

void DisposeTrialMissionListGraphics(void)
{
    int i;

    DisposeTrialImagePanel(TrialMissionListPanel);
    if (TrialMissionDetailsFrame)
    {
        vfree(TrialMissionDetailsFrame);
        TrialMissionDetailsFrame = NULL;
    }
    for (i = 0; i < 3; i++)
    {
        if (TrialMissionOriginIcons[i])
        {
            vfree(TrialMissionOriginIcons[i]);
            TrialMissionOriginIcons[i] = NULL;
        }
    }
    if (TrialThemePreviewSprite)
    {
        vfree(TrialThemePreviewSprite);
        TrialThemePreviewSprite = NULL;
    }
    if (TrialListArrows)
    {
        vfree(TrialListArrows);
        TrialListArrows = NULL;
    }
    if (TrialListHelp)
    {
        vfree(TrialListHelp);
        TrialListHelp = NULL;
    }
    if (TrialListTitle)
    {
        vfree(TrialListTitle);
        TrialListTitle = NULL;
    }
    if (TrialListHighlight)
    {
        vfree(TrialListHighlight);
        TrialListHighlight = NULL;
    }
    vfree(TrialPreviewArchive);
    TrialPreviewArchive = NULL;
}

void LoadTrialMissionListGraphics(short editing)
{
    /* The original reserves forty unused stack bytes before the pointer
     * table. A sprite workspace gives that reservation; its type is inferred. */
    GsSPRITE workspace;
    Sprite3D **destinations[8] = {
        &TrialMissionDetailsFrame,
        &TrialMissionOriginIcons[0],
        &TrialMissionOriginIcons[1],
        &TrialMissionOriginIcons[2],
        &TrialListArrows,
        &TrialListHelp,
        &TrialListTitle,
        &TrialListHighlight
    };
    GsSPRITE *sprite;
    u32 half_height;
    int i;

    TrialMissionListPanel = valloc(sizeof(*TrialMissionListPanel));
    InitTrialImagePanel(TrialMissionListPanel);
    SetupTrialImagePanel(TrialMissionListPanel, TrialImageArchive, 2);
    TrialThemePreviewSprite = load_archive_sprite_(TrialPreviewArchive, 0);
    sprite = &TrialThemePreviewSprite->sprite;
    sprite->x = 444;
    sprite->y = -49;
    for (i = 0; i < 8; i++)
        *destinations[i] = load_archive_sprite_(TrialImageArchive, i + 3);
    sprite = &TrialMissionDetailsFrame->sprite;
    sprite->x = 444;
    sprite->y = 0;
    TrialListArrows->sprite.h = 16;
    TrialListArrows->sprite.attribute |= GS_ATTR_SEMITRANS_ADD;
    sprite = &TrialListHelp->sprite;
    sprite->x = -111;
    sprite->y = 95;
    TrialListHelp->sprite.attribute |= GS_ATTR_HIDDEN;
    if (!editing)
        sprite->w -= 90;
    sprite = &TrialListTitle->sprite;
    sprite->x = -131;
    sprite->y = -90;
    half_height = sprite->h / 2;
    sprite->h = half_height;
    sprite->my = half_height / 2;
    if (editing)
        sprite->v += 20;
    sprite = &TrialListHighlight->sprite;
    sprite->x = -131;
    sprite->y = -88;
    TrialListHighlight->sprite.attribute |= GS_ATTR_SEMITRANS_ENABLE;
}

/* Preset files contain geometry and mission settings. Time/name fields come
 * from the persistent save; record padding retains the preset file bytes. */
void LoadTrialPresetMission(TrialMissionPreview *preview)
{
    char filename[128];
    TrialMissionData *loaded;
    int i;

    sprintf(filename, "%spreset\\preset%d.msn", "K:\\WORK\\CDIMAGE\\TRIAL\\", preview->id);
    loaded = (TrialMissionData *)FileRead(filename);
    TrialMission = *loaded;
    for (i = 0; i < TRIAL_MISSION_RECORD_COUNT; i++)
    {
        TrialMission.header.records[i].time =
            TRIAL_PERSISTENT_STATE->preset_records[preview->id][i].time;
        __builtin_memcpy(TrialMission.header.records[i].name.text,
                         TRIAL_PRESET_RECORD_NAME(preview->id, i).text, 10);
    }
    TrialSelectedMission.preset_id = preview->id;
    TrialSelectedMission.is_user_mission = 0;
    vfree(loaded);
}
