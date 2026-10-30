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
    TRIAL_EDITOR_RESUME,
    TRIAL_EDITOR_COMMAND_COUNT
};

/* Each .LND entry is 24 bytes. Terrain uses ten face-texture indices;
 * ornament entries contain a TMD basename, replaced by an index after load. */
typedef struct TrialTileDefinition
{
    u16 flags;
    u16 unknown2;
    union
    {
        s16 textures[10];
        char ornament_name[20];
        s16 ornament_index;
    } data;
} TrialTileDefinition;

typedef struct TrialTileTexture
{
    s16 page, clut;
    s16 u, v;
} TrialTileTexture;

enum
{
    TRIAL_TILE_ORNAMENT_MASK = 0x0c00
};

extern TrialTileDefinition *TrialTileDefinitions;
extern s16 TrialTileDefinitionCount;
extern TrialTileTexture *TrialTileTextures;
extern OrnamentType **TrialThemeOrnaments;

extern BackGround *TrialEditorBackground;
/* Each editing screen keeps its own cursor and saves its viewport center here. */
extern s16 TrialEditorCenterX, TrialEditorCenterZ;
extern s32 TrialEditorViewAngle;
extern s16 TrialEditorMode;
extern u32 TrialEditorChecksum;
extern s16 TrialEditorModified;
extern s16 TrialTerrainOrnamentCount;
extern s16 TrialEditorCommandSelection;
extern s32 TrialEditorCommandButtons, TrialEditorPreviousCommandButtons;
extern GsSPRITE TrialEditorCommandLabels[TRIAL_EDITOR_COMMAND_COUNT];
extern GsSPRITE TrialEditorCommandBackground;
extern GsSPRITE TrialEditorResumeHint;
extern GsSPRITE TrialEditorConfirmButtons;
extern TrialMapCell TrialEditorMap[TRIAL_MAP_HEIGHT][TRIAL_MAP_WIDTH];

void LoadTrialEditorTheme(short theme);
void DisposeTrialTerrainModel(ModelType *model);
void DrawTrialEditorTile(u8 *cell, short *edge_heights);
void ValidateTrialEditorMap(void);
void LoadTrialEditorGraphics(void);
int UpdateTrialEditorCommands(short interactive);
void EditTrialTerrain(void);
void EditTrialCharacters(void);
void EditTrialPatrols(void);
short SaveTrialEditorMission(short exiting);

#endif
