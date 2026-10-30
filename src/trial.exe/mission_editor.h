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

enum trial_editor_sound
{
    TRIAL_EDITOR_SOUND_COMMANDS = 2,
    TRIAL_EDITOR_SOUND_SELECT = 11,
    TRIAL_EDITOR_SOUND_REMOVE = 12,
    TRIAL_EDITOR_SOUND_PLACE = 13,
    TRIAL_EDITOR_SOUND_ROTATE = 16,
    TRIAL_EDITOR_SOUND_MOVE = 18
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
    TRIAL_TILE_ORNAMENT_THICK = 0x0400,
    TRIAL_TILE_ORNAMENT_THIN = 0x0800,
    TRIAL_TILE_ORNAMENT_MASK = 0x0c00,
    TRIAL_TILE_TRIGGER = 0x1000,
    TRIAL_TILE_SHOOTER = 0x2000,
    /* The same bit on an ornament describes a disguised cover for a hole. */
    TRIAL_TILE_DISGUISED_ORNAMENT = TRIAL_TILE_SHOOTER,
    TRIAL_TILE_LIGHT = 0x4000,
    TRIAL_TILE_KIND_MASK = 0xf0,
    TRIAL_TILE_INDEX_MASK = 0x3f,
    TRIAL_HEIGHT_GROUND = 1,
    TRIAL_HEIGHT_ORNAMENT = 2
};

/* Terrain kinds are named by the editor captions. Negative heights raise
 * the surface in the engine's downward-positive world Y coordinates. */
enum trial_terrain_kind
{
    TRIAL_TERRAIN_GROUND = 0x10,
    TRIAL_TERRAIN_LEDGE_1M = 0x20,
    TRIAL_TERRAIN_LEDGE_2M = 0x30,
    TRIAL_TERRAIN_LEDGE_3M = 0x40,
    TRIAL_TERRAIN_LEDGE_4M = 0x50,
    TRIAL_TERRAIN_RAMP_1M = 0x60,
    TRIAL_TERRAIN_RAMP_2M = 0x70,
    TRIAL_TERRAIN_WALL = 0x80,
    TRIAL_TERRAIN_HOLE = 0x90,
    TRIAL_TERRAIN_POND = 0xa0
};

/* Generated ornament instances retain their world position and a packed
 * model index (low byte) / quarter-turn rotation (next byte). */
typedef struct TrialTerrainOrnament
{
    s32 x, y, z;
    s32 model_rotation;
} TrialTerrainOrnament;

enum { TRIAL_TERRAIN_CHUNKS = 8, TRIAL_TERRAIN_ORNAMENT_CAPACITY = 64 };
extern ModelType *TrialTerrainChunks[TRIAL_TERRAIN_CHUNKS * TRIAL_TERRAIN_CHUNKS];
extern TrialTerrainOrnament TrialTerrainOrnaments[TRIAL_TERRAIN_ORNAMENT_CAPACITY];

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
extern s16 TrialTerrainBrightness;
extern s16 TrialTerrainScale;
extern s16 TrialTerrainHeightSkew;
extern ModelType *TrialEditorTileModel;
extern s16 TrialTerrainVertexCount;
extern SVECTOR *TrialTerrainVertices;
extern s16 TrialTerrainPrimitiveCount;
extern s16 TrialTerrainHeights[10];
extern s16 TrialEditorCommandSelection;
extern s32 TrialEditorCommandButtons, TrialEditorPreviousCommandButtons;
extern GsSPRITE TrialEditorCommandLabels[TRIAL_EDITOR_COMMAND_COUNT];
extern GsSPRITE TrialEditorCommandBackground;
extern GsSPRITE TrialEditorResumeHint;
extern GsSPRITE TrialEditorConfirmButtons;
extern GsSPRITE TrialEditorScrollArrows[4];
typedef struct TrialEditorCursorSprites
{
    GsSPRITE map;
    GsSPRITE palette;
} TrialEditorCursorSprites;
extern TrialEditorCursorSprites TrialEditorCursors;
/* Moving arrows followed by the four wait-at-waypoint arrows. */
extern GsSPRITE TrialPatrolArrows[8];
extern GsSPRITE TrialPatrolWaitDigits[10];
extern POLY_FT4 TrialPreviewDirectionQuad;
extern TrialMapCell TrialEditorMap[TRIAL_MAP_HEIGHT][TRIAL_MAP_WIDTH];

extern char TrialTileLabelText[];
char *PrepareTrialTileLabel(short tile);
extern char TrialPatrolCountText[8];
extern char TrialPatrolLabelText[];
char *PrepareTrialPatrolLabel(TrialPlacement *placement, short remaining);
void DrawTrialEditorOverviewFrame(short cursor_x, short cursor_z,
                                 short center_x, short center_z);
void DrawTrialCharacterThumbnail(short kind, short x, short y,
                                 int depth, short scale);
void DrawTrialEditorTerrain(short cursor_x, short cursor_z,
                            short center_x, short center_z);
void DrawTrialEditorCharacters(short cursor_x, short cursor_z,
                               short center_x, short center_z);

void LoadTrialEditorTheme(short theme);
void DisposeTrialTerrainModel(ModelType *model);
ModelType *BuildTrialTerrainModel(short x, short z, short size);
AreaMapType *BuildTrialCollisionMap(void);
void UpdateTrialTerrainChunks(short mode);
void DrawTrialTerrainChunks(int mission);
void DrawTrialTerrainOrnaments(void);
void PreviewTrialTerrain(short x, short z);
short GetTrialTerrainHeight(short x, short z, short layers);
short FindOrAddTrialTerrainVertex(short x, short y, short z);
short SetupTrialTerrainPrimitive(TMD_P_TNG4 *primitive, short texture, short half);
void DrawTrialEditorTile(u8 *cell, short *edge_heights);
void ValidateTrialPlacements(void);
short IsTrialPatrolStepBlocked(short x, short z, short next_x, short next_z, short facing);
void ValidateTrialEditorMap(void);
void LoadTrialEditorGraphics(void);
int UpdateTrialEditorCommands(short interactive);
void EditTrialTerrain(void);
void EditTrialCharacters(void);
void EditTrialPatrols(void);
short SaveTrialEditorMission(short exiting);

#endif
