#ifndef TENCHU_TRIAL_MISSION_EDITOR_H
#define TENCHU_TRIAL_MISSION_EDITOR_H

#include "mission_menu.h"

enum trial_editor_mode
{
    TRIAL_EDITOR_EXIT = -1,
    TRIAL_EDITOR_SETTINGS,
    TRIAL_EDITOR_TERRAIN,
    TRIAL_EDITOR_CHARACTERS,
    TRIAL_EDITOR_PATROLS,
    TRIAL_EDITOR_PLAYTEST,
    TRIAL_EDITOR_SAVE,
    TRIAL_EDITOR_SAVE_EXIT,
    TRIAL_EDITOR_RESUME
};

extern BackGround *TrialEditorBackground;
/* Each editing screen keeps its own cursor and saves its viewport center here. */
extern s16 TrialEditorCenterX, TrialEditorCenterZ;
extern s32 TrialEditorViewAngle;
extern s16 TrialEditorMode;
extern u32 TrialEditorChecksum;
extern TrialMapCell TrialEditorMap[TRIAL_MAP_HEIGHT][TRIAL_MAP_WIDTH];

void LoadTrialEditorTheme(short theme);
void DrawTrialEditorTile(u8 *cell, short *edge_heights);
void ValidateTrialEditorMap(void);
void LoadTrialEditorGraphics(void);
int UpdateTrialEditorCommands(short interactive);
void EditTrialTerrain(void);
void EditTrialCharacters(void);
void EditTrialPatrols(void);
short SaveTrialEditorMission(short exiting);

#endif
