#ifndef TRIAL_MISSION_MENU_H
#define TRIAL_MISSION_MENU_H

#include "trial.exe.h"
#include "mission.h"

enum
{
    TRIAL_PRESET_COUNT = 16,
    TRIAL_THEME_COUNT = 12,
    TRIAL_PREVIEW_RECORDS = 3,
    TRIAL_CONFIRM_SPRITES = 8,
    TRIAL_OPTION_FIRST_ROW = 4,
    TRIAL_OPTION_COUNT = 3,
    TRIAL_PANEL_HIDE = 0,
    TRIAL_PANEL_SHOW = 1
};

typedef struct TrialMissionPreview
{
    s32 id;                 /* 0x00: 0..15 = preset; 16 or more = card */
    s16 available;          /* 0x04 */
    s16 theme;              /* 0x06 */
    char *title;            /* 0x08: points into the loaded mission header */
    char *author;           /* 0x0c */
    u32 unknown10;          /* 0x10: not yet identified */
    char names[TRIAL_PREVIEW_RECORDS][10]; /* 0x14 */
    char times[TRIAL_PREVIEW_RECORDS][12]; /* 0x32: full-width Japanese text */
} TrialMissionPreview; /* 0x58 */

typedef struct TrialOptionArrowPair
{
    s16 left_x, left_y;
    s16 right_x, right_y;
} TrialOptionArrowPair;

extern ArcFile *TrialPreviewArchive;
extern TrialImagePanel *TrialMissionListPanel;
extern Sprite3D *TrialMissionDetailsFrame;
extern Sprite3D *TrialThemePreviewSprite;
extern Sprite3D *TrialListArrows;
extern Sprite3D *TrialListHelp;
extern Sprite3D *TrialListTitle;
extern Sprite3D *TrialListHighlight;
/* Atlas images 4, 5, and 6: card slot 1, card slot 2, and CD. */
extern Sprite3D *TrialMissionOriginIcons[3];
extern Sprite3D *TrialCharacterPortraits[N_PLAYABLE_CHARACTERS];
extern BackGround *TrialCharacterBackground;
extern BackGround *TrialConfirmationBackground;
extern Sprite3D *TrialConfirmationSprites[TRIAL_CONFIRM_SPRITES];
extern Sprite3D *TrialOptionArrows[2];
extern s16 TrialOptionLimits[TRIAL_OPTION_COUNT];
extern TrialOptionArrowPair TrialOptionArrowPositions[TRIAL_OPTION_COUNT];

void FormatTrialMissionPreview(TrialMissionHeader *header, TrialMissionPreview *preview);
void SortTrialSpritePrimitive(GsSPRITE *sprite, short transparent, GsOT *ot, int priority);
void LoadTrialPreviewImages(void);
int PrepareTrialMissionPreview(TrialMissionHeader *header, TrialMissionPreview *preview);
void LoadTrialThemePreview(TrialMissionPreview *preview);
void DrawTrialMissionOrigin(TrialMissionPreview *preview, short x, short y, int priority);
void DrawTrialListArrow(short down, short x, short y, u16 priority);
int SlideTrialMissionDetails(short direction);
void ResetTrialCharacterGraphics(void);
void DisposeTrialCharacterGraphics(void);
void LoadTrialCharacterGraphics(void);
void ResetTrialConfirmationGraphics(void);
void DisposeTrialConfirmationGraphics(void);
void DrawTrialOptionArrows(short row, short value, short first_row_maximum);
short GetTrialUnlockedThemes(short themes[TRIAL_THEME_COUNT]);

#endif
