#include "common.h"
#include "mission_editor.h"
#include "card.h"
#include "../main.exe/appear.h"
#include "../main.exe/graphics.h"
#include "../main.exe/images.h"
#include "../main.exe/sound.h"

extern s32 SlightPoint;

/* Settings, playtest, and save return to the previous editing screen.
 * The initial terrain mode establishes that screen before it can be used. */
void EditTrialMission(void)
{
    RECT clear_area = {0, 0, 320, 480};
    short mode;
    short previous_mode;

    SetDispMask(0);
    ClearImage(&clear_area, 0, 0, 0);
    DrawSync(0);
    VSync(30);
    SetupSoundEffect(RIKIMARU_0, -1);
    DrawTrialEditorTile(NULL, NULL);
    InitGraphicsSystem();
    SlightPoint = 0;
    SetupSoundEffect(RIKIMARU_0, TrialMission.header.theme);
    SetupAppearance(RIKIMARU_0, TrialMission.header.theme + 1);
    LoadTrialEditorTheme(TrialMission.header.theme);
    LoadTrialEditorGraphics();
    TrialEditorChecksum = CalculateTrialMissionChecksum();
    __builtin_memcpy(TrialEditorMap, TrialMission.map, sizeof(TrialEditorMap));
    TrialEditorCenterX = TrialEditorCenterZ = TRIAL_MAP_WIDTH / 2;
    TrialEditorViewAngle = 0;
    TrialEditorMode = TRIAL_EDITOR_TERRAIN;
    do
    {
        ClearImage(&clear_area, 0, 0, 0);
        DrawSync(0);
        UpdateTrialEditorCommands(0);
        previous_mode = mode;
        ValidateTrialEditorMap();
        switch (mode = TrialEditorMode)
        {
        case TRIAL_EDITOR_SETTINGS:
            ConfigureTrialMission(1);
            TrialEditorMode = previous_mode;
            break;
        case TRIAL_EDITOR_TERRAIN:
            EditTrialTerrain();
            break;
        case TRIAL_EDITOR_CHARACTERS:
            EditTrialCharacters();
            break;
        case TRIAL_EDITOR_PATROLS:
            EditTrialPatrols();
            break;
        case TRIAL_EDITOR_PLAYTEST:
            PlaytestTrialMission();
            LoadTrialEditorGraphics();
            TrialEditorMode = previous_mode;
            break;
        case TRIAL_EDITOR_SAVE:
            SaveTrialEditorMission(0);
            TrialEditorMode = previous_mode;
            break;
        case TRIAL_EDITOR_SAVE_EXIT:
            TrialEditorMode = SaveTrialEditorMission(1) ? TRIAL_EDITOR_EXIT : previous_mode;
            break;
        }
    } while (TrialEditorMode >= 0);
    DisposeBG(TrialEditorBackground);
    TrialEditorBackground = NULL;
    LoadTrialEditorTheme(-1);
    SetupAppearance(RIKIMARU_0, -1);
}
