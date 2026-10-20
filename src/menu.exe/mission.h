#ifndef MENU_MISSION_H
#define MENU_MISSION_H

#include "frontend.h"
#include "imagegrid.h"

enum
{
    MISSION_COUNT = 10,
    MISSION_LAYOUT_UNPLAYED = -1,
    MISSION_LAYOUT_LOCKED = -2,
    MENU_SELECT_LAYOUT = 4,
    MENU_UNLOCK_LAYOUTS = 0x10
};

/* SELSTG_*.ARC: each preview/title shares a VRAM slot with its siblings. */
enum
{
    MISSION_IMAGE_MAP,
    MISSION_IMAGE_LAYOUT_BANNER,
    MISSION_IMAGE_RANK_BANNER,
    MISSION_IMAGE_PREVIEW_FIRST,
    MISSION_IMAGE_TITLE_FIRST = 13,
    MISSION_IMAGE_RANK_FIRST = 23,
    MISSION_IMAGE_RANK_LAST = 27,
    MISSION_IMAGE_CURSOR,
    MISSION_IMAGE_UNPLAYED,
    MISSION_IMAGE_OVERALL_UNPLAYED,
    MISSION_IMAGE_OVERALL_RANK_FIRST
};

typedef struct MissionCursorPosition
{
    short x, y;
} MissionCursorPosition;

extern packed_stage_id MissionStageOrder[MISSION_COUNT + 1];
extern char *MissionImagePaths[N_LANGUAGES];
extern MissionCursorPosition MissionCursorTargets[MISSION_COUNT];
extern MissionCursorPosition MissionCursorStarts[MISSION_COUNT];
extern u_long *MissionImageArchive;
extern Sprite3D *MissionPreview;
extern Sprite3D *MissionTitles[2];
extern Sprite3D *MissionCursor;
extern Sprite3D *MissionOverallRank;
extern MenuImageGrid *MissionBackground;
extern MenuImageGrid *MissionBanner;
extern u8 MissionUnlocked;
/* MissionSelection is one-based; layout and image arrays are zero-based. */
extern short MissionSelection;
extern short MissionLayoutSelection;
extern short MissionInputReady;
extern int MissionButtons;
extern int MissionPressedButtons;
extern Sprite3D *MissionRankSprites[N_STAGE_RANKS][N_STAGE_LAYOUTS];
extern Sprite3D *MissionUnplayedSprites[N_STAGE_LAYOUTS];
extern GsSPRITE *MissionVisibleRanks[N_STAGE_LAYOUTS];
extern short MissionLayoutRanks[MISSION_COUNT][N_STAGE_LAYOUTS];
extern short MissionBestRanks[MISSION_COUNT];
extern short MissionBannerX;
extern short MissionBannerY;
extern short MissionFirstBannerColumn;
extern short MissionFirstRank;
extern short MissionFirstTitle;
extern short MissionRevealFlag; /* Written when the reveal starts; unused. */

short SelectMenuMission(void);
void InitMissionSelection(void);
void DisposeMissionSelection(void);
void LoadMissionImages(void);
void SetupMissionSprites(void);
void DrawMissionSelection(void);
short UpdateMissionSelection(void);

#endif
