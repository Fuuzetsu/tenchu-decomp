#include "common.h"
#include "mission_menu.h"

extern const char path_trial_previews[];

enum trial_settings_step
{
    TRIAL_SETTINGS_FORM = 20,
    TRIAL_SETTINGS_TITLE,
    TRIAL_SETTINGS_AUTHOR,
    TRIAL_SETTINGS_PASSWORD,
    TRIAL_SETTINGS_CHECK_CARD = 30,
    TRIAL_SETTINGS_CANCEL,
    TRIAL_SETTINGS_ACCEPT,
    TRIAL_SETTINGS_WAIT_CARD = 40
};

/* New missions need a writable card before proceeding to the map editor.
 * Existing missions retain their edited header when leaving this screen. */
short ConfigureTrialMission(short editing)
{
    TrialMissionHeader settings;
    TrialMissionHeader original;
    TrialThemeNames themes;
    u_long previous_buttons;
    short phase = TRIAL_SETTINGS_FORM;
    short accepted;
    short card_result;

    themes = TrialDefaultThemeNames;
    memset(&settings, 0, sizeof(settings));
    if (!editing)
        InitNewTrialMission();
    settings = TrialMission.header;
    original = TrialMission.header;
    TrialSettingsArchive = FileRead(path_trial_settings);
    TrialSettingsPreviewArchive = FileRead(path_trial_previews);
    while (phase)
    {
        switch (phase)
        {
        case TRIAL_SETTINGS_FORM:
            phase += EditTrialMissionSettings(editing, &settings);
            break;
        case TRIAL_SETTINGS_TITLE:
            EditTrialTextField(TRIAL_TEXT_TITLE, settings.title, sizeof(settings.title));
            phase = TRIAL_SETTINGS_FORM;
            break;
        case TRIAL_SETTINGS_AUTHOR:
            EditTrialTextField(TRIAL_TEXT_AUTHOR, settings.author, sizeof(settings.author));
            phase = TRIAL_SETTINGS_FORM;
            break;
        case TRIAL_SETTINGS_PASSWORD:
            EditTrialTextField(TRIAL_TEXT_PASSWORD, settings.password, sizeof(settings.password));
            phase = TRIAL_SETTINGS_FORM;
            break;
        case TRIAL_SETTINGS_CHECK_CARD:
            previous_buttons = GetRealPad(0);
            phase = TRIAL_SETTINGS_WAIT_CARD;
            ClearTrialDisplay(320, 480);
            break;
        case TRIAL_SETTINGS_CANCEL:
            accepted = 0;
            phase = 0;
            break;
        case TRIAL_SETTINGS_ACCEPT:
            accepted = 1;
            phase = 0;
            break;
        case TRIAL_SETTINGS_WAIT_CARD:
            StartDrawing();
            card_result = UpdateTrialCardCheck(ReadTrialMenuPad(0, &previous_buttons, 0), 0);
            switch (card_result)
            {
            case -1:
                phase = TRIAL_SETTINGS_ACCEPT;
                break;
            case 1:
                phase = TRIAL_SETTINGS_FORM;
                break;
            }
            SkipFrame = 2;
            EndDrawing(-2);
            break;
        }
    }
    ClearTrialPages();
    vfree(TrialSettingsArchive);
    vfree(TrialSettingsPreviewArchive);
    if (accepted || editing)
    {
        if (!settings.title[0])
            sprintf(settings.title, fmt_trial_default_title, themes.text[settings.theme]);
        TrialMission.header = settings;
    }
    if ((original.flags & TRIAL_OBJECTIVE_MASK) != (settings.flags & TRIAL_OBJECTIVE_MASK))
        SetupTrialObjectivePlacements(1);
    if (!editing)
        FillTrialMissionMap();
    return accepted;
}
