#ifndef TRIAL_MISSION_MENU_H
#define TRIAL_MISSION_MENU_H

#include "trial.exe.h"
#include "mission.h"

enum
{
    TRIAL_PRESET_COUNT = 16,
    TRIAL_THEME_COUNT = 12,
    TRIAL_PREVIEW_RECORDS = 3,
    TRIAL_OPTION_FIRST_ROW = 4,
    TRIAL_OPTION_COUNT = 3,
    TRIAL_PANEL_HIDE = 0,
    TRIAL_PANEL_SHOW = 1
};

enum trial_confirmation_sprite
{
    TRIAL_CONFIRM_HEADING,
    TRIAL_CONFIRM_FIELD_BARS,
    TRIAL_CONFIRM_HIGHLIGHT,
    TRIAL_CONFIRM_FIELD_LABELS,
    TRIAL_CONFIRM_RECORD_HEADING,
    TRIAL_CONFIRM_RANKS,
    TRIAL_CONFIRM_CONTROLS,
    TRIAL_CONFIRM_STEALTH_LABEL,
    TRIAL_CONFIRM_SPRITES
};

enum trial_menu_sound
{
    TRIAL_SOUND_CURSOR = 3,
    TRIAL_SOUND_ACCEPT = 4,
    TRIAL_SOUND_CANCEL = 5,
    TRIAL_SOUND_CHARACTER = 6,
    TRIAL_SOUND_UNLOCK = 7,
    TRIAL_SOUND_OPTION = 11,
    TRIAL_SOUND_ERROR = 37
};

enum trial_text_mode
{
    TRIAL_TEXT_TITLE,
    TRIAL_TEXT_PASSWORD,
    TRIAL_TEXT_AUTHOR,
    TRIAL_TEXT_RECORD_NAME
};

enum trial_setting_field
{
    TRIAL_FIELD_TITLE = 1,
    TRIAL_FIELD_AUTHOR,
    TRIAL_FIELD_PASSWORD,
    TRIAL_FIELD_THEME,
    TRIAL_FIELD_OBJECTIVE,
    TRIAL_FIELD_STEALTH,
    TRIAL_SETTINGS_CONTINUE = 10,
    TRIAL_SETTINGS_LEAVE = 11
};

typedef struct TrialMissionPreview
{
    s32 id;                 /* 0x00: 0..15 = preset; 16 or more = card */
    s16 available;          /* 0x04 */
    s16 theme;              /* 0x06 */
    char *title;            /* 0x08: points into the loaded mission header */
    char *author;           /* 0x0c */
    u32 unused;             /* 0x10: never accessed by TRIAL; original purpose unknown */
    char names[TRIAL_PREVIEW_RECORDS][10]; /* 0x14 */
    char times[TRIAL_PREVIEW_RECORDS][12]; /* 0x32: full-width Japanese text */
} TrialMissionPreview; /* 0x58 */

typedef struct TrialOptionArrowPair
{
    s16 left_x, left_y;
    s16 right_x, right_y;
} TrialOptionArrowPair;

/* The campaign handoff is followed by sixteen sets of ten preset records.
 * Together they occupy the existing 0xe70-byte persistent-state contract. */
typedef struct TrialPersistentState
{
    TLinkInfo game;
    TrialMissionRecord preset_records[TRIAL_PRESET_COUNT][TRIAL_MISSION_RECORD_COUNT];
} TrialPersistentState;

#define TRIAL_PERSISTENT_STATE \
    ((TrialPersistentState *)TENCHU_PERSISTENT_STATE_ADDRESS)

/* A name occupies ten bytes in each sixteen-byte record. Keep this field's
 * fixed persistent-RAM base explicit when copying the name independently. */
#define TRIAL_PRESET_RECORD_NAME(preset, rank) \
    (*(TrialRecordName *)((u8 *)&TRIAL_PERSISTENT_STATE->preset_records[0][0].name + \
        (preset) * sizeof(TRIAL_PERSISTENT_STATE->preset_records[0]) + \
        (rank) * sizeof(TrialMissionRecord)))

typedef struct TrialThemeNames
{
    const char *text[TRIAL_THEME_COUNT];
} TrialThemeNames;

/* The thumbnail atlas follows this table's character order, including the
 * final fallback image. The two-byte kind field is followed by alignment. */
typedef struct TrialCharacterLabel
{
    s16 kind;
    char *text;
} TrialCharacterLabel;
extern TrialCharacterLabel TrialCharacterLabels[];
extern const TrialThemeNames TrialDefaultThemeNames;
extern const char path_trial_settings[];
extern const char fmt_trial_default_title[];

/* Labels shared by mission previews and the confirmation screen. */
typedef struct TrialObjectiveLabels
{
    const char *text[5];
} TrialObjectiveLabels;
extern const TrialObjectiveLabels TrialObjectiveText;
extern const char TrialEmptyRecordName[];
extern const char TrialEmptyRecordTime[];
extern const char TrialRecordTimeFormat[];
extern const char TrialMaximumRecordTime[];

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
extern ArcFile *TrialSettingsArchive;
extern ArcFile *TrialSettingsPreviewArchive;
extern BackGround *TrialSettingsBackground;
extern Sprite3D *TrialOptionCursor;
extern Sprite3D *TrialOptionThemeLabels;
extern Sprite3D *TrialOptionObjectiveLabels;
extern Sprite3D *TrialOptionStealthLabels;
extern Sprite3D *TrialOptionHeading;
extern Sprite3D *TrialOptionHelp;
extern Sprite3D *TrialOptionThemePreview;
extern u_long TrialResultsButtons;

void LoadTrialSpriteStrip(ArcFile *archive, GsSPRITE *sprites,
                          short image_index, short frame_count);
void PlaytestTrialMission(void);
char *PrepareTrialCharacterLabel(short kind);

void DrawTrialMissionList(TrialMissionPreview *previews, short selected,
                          u32 reveal_frames, int priority);
void DisposeTrialMissionListGraphics(void);
void LoadTrialMissionListGraphics(short editing);
void LoadTrialPresetMission(TrialMissionPreview *preview);
int UpdateTrialCharacterSelection(u_long pressed, short *character);
void FormatTrialMissionPreview(TrialMissionHeader *header, TrialMissionPreview *preview);
short LoadTrialPresetPreviews(short card_slot, TrialMissionHeader *headers,
                              TrialMissionPreview *previews);
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
void LoadTrialConfirmationGraphics(void);
short GetTrialMissionNarrationCue(void);
void ResetTrialConfirmationGraphics(void);
void DisposeTrialConfirmationGraphics(void);
void DisposeTrialOptionGraphics(void);
void LoadTrialOptionGraphics(short editing);
short EditTrialMissionSettings(short editing, TrialMissionHeader *header);
void ShowTrialTextEntry(short mode);
void EditTrialTextField(short mode, char *text, int size);
short UpdateTrialCardCheck(u_long pressed, short existing_mission);
short UpdateTrialCardDelete(u_long pressed, const TrialMissionHeader *header);
short UpdateTrialCardLoad(u_long pressed, TrialMissionHeader *headers, short whole_mission);
short UpdateTrialCardSave(u_long pressed, short records_only);
short UpdateTrialRecordSave(short active);
short AskRetryTrialPassword(void);
void DrawTrialOptionArrows(short row, short value, short first_row_maximum);
short GetTrialUnlockedThemes(short themes[TRIAL_THEME_COUNT]);

#endif
