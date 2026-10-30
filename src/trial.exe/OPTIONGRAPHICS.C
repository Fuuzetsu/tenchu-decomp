#include "common.h"
#include "mission_menu.h"
#include "../main.exe/images.h"

void DisposeTrialOptionGraphics(void)
{
    int i;

    if (TrialSettingsBackground)
    {
        DisposeBG(TrialSettingsBackground);
        TrialSettingsBackground = NULL;
    }
    if (TrialOptionCursor)
    {
        vfree(TrialOptionCursor);
        TrialOptionCursor = NULL;
    }
    if (TrialOptionThemeLabels)
    {
        vfree(TrialOptionThemeLabels);
        TrialOptionThemeLabels = NULL;
    }
    if (TrialOptionObjectiveLabels)
    {
        vfree(TrialOptionObjectiveLabels);
        TrialOptionObjectiveLabels = NULL;
    }
    if (TrialOptionStealthLabels)
    {
        vfree(TrialOptionStealthLabels);
        TrialOptionStealthLabels = NULL;
    }
    for (i = 0; i < 2; i++)
    {
        if (TrialOptionArrows[i])
        {
            vfree(TrialOptionArrows[i]);
            TrialOptionArrows[i] = NULL;
        }
    }
    if (TrialOptionHeading)
    {
        vfree(TrialOptionHeading);
        TrialOptionHeading = NULL;
    }
    if (TrialOptionHelp)
    {
        vfree(TrialOptionHelp);
        TrialOptionHelp = NULL;
    }
    if (TrialOptionThemePreview)
    {
        vfree(TrialOptionThemePreview);
        TrialOptionThemePreview = NULL;
    }
}

/* NINSET.ARC contains the form, its label strips, and the arrow pair. */
void LoadTrialOptionGraphics(short editing)
{
    /* The original reserves 32 unused bytes before this pointer table;
     * no accesses survive to identify the workspace's original type. */
    char workspace[32];
    Sprite3D **destinations[7] = {
        &TrialOptionCursor, &TrialOptionThemeLabels,
        &TrialOptionObjectiveLabels, &TrialOptionStealthLabels,
        &TrialOptionArrows[0], &TrialOptionHeading, &TrialOptionHelp
    };
    int i;
    Sprite3D *image;
    GsSPRITE *sprite;

    TrialSettingsBackground = load_background_(get_tim_from_archive(TrialSettingsArchive, 0));
    for (i = 0; i < 7; i++)
    {
        *destinations[i] = load_archive_sprite_(TrialSettingsArchive, i + 1);
        image = *destinations[i];
        image->sprite.my = 0;
        image->sprite.mx = 0;
    }
    sprite = &TrialOptionCursor->sprite;
    sprite->x = -155;
    sprite->y = -77;
    sprite = &TrialOptionThemeLabels->sprite;
    sprite->x = -60;
    sprite->y = 6;
    sprite->h /= TRIAL_THEME_COUNT;
    sprite->my = sprite->h / 2;
    sprite = &TrialOptionObjectiveLabels->sprite;
    sprite->x = -60;
    sprite->y = 30;
    sprite->h /= 5;
    sprite->my = sprite->h / 2;
    sprite = &TrialOptionStealthLabels->sprite;
    sprite->x = -60;
    sprite->y = 51;
    sprite->h >>= 1;
    sprite->my = sprite->h >> 1;
    sprite = &TrialOptionArrows[0]->sprite;
    sprite->x = 40;
    sprite->y = 0;
    sprite->w >>= 1;
    sprite->mx = sprite->w >> 1;
    if (!editing)
        TrialOptionThemePreview = load_archive_sprite_(TrialSettingsPreviewArchive, 0);
    else
        TrialOptionThemePreview = load_archive_sprite_(TrialSettingsPreviewArchive,
                                                       TrialMission.header.theme);
    sprite = &TrialOptionThemePreview->sprite;
    sprite->x = 100;
    sprite->y = 75;
    sprite->scalex = 2560;
    TrialOptionArrows[1] = valloc(sizeof(Sprite3D));
    *TrialOptionArrows[1] = *TrialOptionArrows[0];
    TrialOptionArrows[1]->sprite.x = 60;
    TrialOptionArrows[1]->sprite.y = 0;
    TrialOptionArrows[1]->sprite.u += TrialOptionArrows[1]->sprite.w;
    sprite = &TrialOptionHeading->sprite;
    sprite->x = -72;
    sprite->y = -90;
    sprite->h /= 2;
    sprite->v += editing * 22;
    sprite->mx = sprite->w / 2;
    sprite->my = sprite->h / 2;
    sprite = &TrialOptionHelp->sprite;
    sprite->x = -140;
    sprite->y = 90;
    if (editing)
        sprite->w -= 50;
}
