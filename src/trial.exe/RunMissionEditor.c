#include "common.h"
#include "trial.exe.h"
#include "../main.exe/vmemory.h"

/* The menu archive is released while either the playable stage or the map
 * editor owns the heap. Each path reloads it before returning to the menus. */
void RunMissionEditor(void)
{
    short mode = TRIAL_MENU_SELECT;
    short step = 0;
    short result;

    TrialPreviousLanguage = TRIAL_STATE->language;
    TRIAL_STATE->language = LANG_JAPANESE;
    TrialImageArchive = FileRead(path_trial_image_archive);
    memset(TrialTextEntry, 0, sizeof(TrialTextEntry));
    do
    {
        switch (mode)
        {
        case TRIAL_MENU_SELECT:
            mode = ShowTrialMainMenu();
            break;
        case TRIAL_MENU_PLAY:
            switch (step)
            {
            case 0:
                step = SelectTrialMission(0) ? 1 : -1;
                break;
            case 1:
                step = ChooseTrialCharacter() ? 2 : 0;
                break;
            case 2:
                step = ConfirmTrialMission() ? 3 : 1;
                break;
            case 3:
                vfree(TrialImageArchive);
                result = RunTrialMission(TRIAL_STATE->CharType != 0);
                SetupTrialMission(TRIAL_STATE->CharType != 0, -1);
                step = 4;
                break;
            case 4:
                step = ShowTrialResults(result <= 0) ? 0 : 1;
                TrialImageArchive = FileRead(path_trial_image_archive);
                break;
            default:
                mode = TRIAL_MENU_SELECT;
                step = 0;
                break;
            }
            break;
        case TRIAL_MENU_CREATE:
            switch (step)
            {
            case 0:
                step = ConfigureTrialMission(0) ? 1 : -1;
                break;
            case 1:
                vfree(TrialImageArchive);
                EditTrialMission();
                TrialImageArchive = FileRead(path_trial_image_archive);
                step = -1;
                break;
            default:
                mode = TRIAL_MENU_SELECT;
                step = 0;
                break;
            }
            break;
        case TRIAL_MENU_EDIT:
            switch (step)
            {
            case 0:
                step = PrepareTrialMemoryCard() ? 1 : -1;
                break;
            case 1:
                step = SelectTrialMission(1) ? 2 : -1;
                break;
            case 2:
                step = VerifyTrialMissionPassword() ? 3 : 1;
                break;
            case 3:
                vfree(TrialImageArchive);
                EditTrialMission();
                TrialImageArchive = FileRead(path_trial_image_archive);
                step = -1;
                break;
            default:
                mode = TRIAL_MENU_SELECT;
                step = 0;
                break;
            }
            break;
        default:
            mode = TRIAL_MENU_EXIT;
            break;
        }
    } while (mode != TRIAL_MENU_EXIT);
    TRIAL_STATE->language = TrialPreviousLanguage;
    DrawTrialScreenFade(0, 320, 240, 1);
    vfree(TrialImageArchive);
}
