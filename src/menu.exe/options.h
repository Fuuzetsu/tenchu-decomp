#ifndef MENU_OPTIONS_H
#define MENU_OPTIONS_H

#include "frontend.h"
#include "cardmenu.h"

enum
{
    OPTIONS_MAIN,
    OPTIONS_RECORDS,
    OPTIONS_DIFFICULTY,
    OPTIONS_SOUND_MODE,
    OPTIONS_VOLUME,
    OPTIONS_SOUND_TEST,
    OPTIONS_CONTROLS,
    OPTIONS_RUMBLE,
    OPTIONS_MEMORY_CARD,
    OPTIONS_AUTO_SAVE,
    OPTIONS_LANGUAGE,
    OPTIONS_SCREEN_COUNT
};

/* Entries in MENU_J.ARC. Several images hold vertically stacked text rows. */
enum
{
    OPTION_IMAGE_BACKGROUND,
    OPTION_IMAGE_HEADERS_1,
    OPTION_IMAGE_HEADERS_2,
    OPTION_IMAGE_CONFIRM_EXIT,
    OPTION_IMAGE_CONFIRM_CANCEL,
    OPTION_IMAGE_SMALL_PANEL,
    OPTION_IMAGE_CHECKMARK,
    OPTION_IMAGE_SWORD,
    OPTION_IMAGE_CURSOR,
    OPTION_IMAGE_MENU_LABELS,
    OPTION_IMAGE_CONTROLLER_PANEL,
    OPTION_IMAGE_CONTROL_ACTIONS,
    OPTION_IMAGE_CONTROL_SCHEMES,
    OPTION_IMAGE_CHOICES,
    OPTION_IMAGE_LANGUAGES,
    OPTION_IMAGE_LOAD_SAVE,
    OPTION_IMAGE_VOLUME_LABELS,
    OPTION_IMAGE_SOUND_TEST_LABELS,
    OPTION_IMAGE_LARGE_PANEL,
    OPTION_IMAGE_VOLUME_TRACK,
    OPTION_IMAGE_VOLUME_KNOB,
    OPTION_IMAGE_LARGE_DIGITS,
    OPTION_IMAGE_SMALL_DIGITS,
    OPTION_IMAGE_RIKIMARU_NAME,
    OPTION_IMAGE_AYAME_NAME,
    OPTION_IMAGE_RANKS,
    OPTION_IMAGE_STAGES,
    OPTION_IMAGE_RECORD_COLUMNS,
    OPTION_IMAGE_RECORD_HIGHLIGHT,
    OPTION_IMAGE_COLON
};

/* Handlers receive -1 to initialize, 0 to update/draw, and 1 to dispose.
 * A negative result requests a fade before switching to OptionsNextScreen;
 * a positive result switches immediately after the current frame. */
typedef short (*OptionHandler)(short mode);
extern OptionHandler OptionHandlers[OPTIONS_SCREEN_COUNT];
extern ArcFile *OptionsImageArchive;
extern BackGround *OptionsBackground;
extern Sprite3D *OptionsHeaders[2];
extern Sprite3D *OptionsHelp[2];
extern Sprite3D *OptionsSmallPanel;
extern Sprite3D *OptionsCheckmark;
extern int OptionsButtons;
extern int OptionsPreviousButtons;
extern short OptionsNextScreen;
extern short OptionsSelection;
extern Sprite3D *OptionDigitSprites[10];
extern Sprite3D *OptionsCursor;
extern short OptionsListSlide;
extern Sprite3D *OptionSwords[10];
extern Sprite3D *OptionLabels[10];
extern Sprite3D *ControlOptionPanels[2];
extern short ControlOptionSlide;
extern short ControlOptionScheme;
extern Sprite3D *ControlSchemeLabels[4];
extern Sprite3D *ControlActionLabels[6];

typedef struct OptionLabelPosition
{
    short x;
    short y;
} OptionLabelPosition;
extern OptionLabelPosition ControlLabelPositions[8][N_LANGUAGES];
extern short ControlLabelActions[4][8];

extern int OptionsMusicTrack;
extern short VolumeOptionSlide;
extern short VolumeOptionSelection;
extern short VolumePositions[2];
extern short VolumeSavedLevels[2];
enum { VOLUME_SLIDER_TRACK, VOLUME_SLIDER_KNOB };
extern Sprite3D *VolumeSliders[2];
extern Sprite3D *VolumeLabels[2];
extern Sprite3D *VolumePanel;
extern Sprite3D *SoundTestLabels[2];
extern Sprite3D *SoundTestPanel;
extern short SoundTestSlide;
extern short SoundTestSelection;
extern short SoundTestChoices[2];
extern short SoundTestEffects[100];
extern int SoundTestMusic[19];
extern Sprite3D *ChoiceOptionHeader;
extern short ChoiceOptionSlide;
extern short ChoiceOptionScreen;
extern short ChoiceOptionFirst;
extern short ChoiceOptionCount;
extern short ChoiceOptionSelection;
extern short ChoiceCardOperation;
extern short ChoicePreviousLanguage;
extern Sprite3D *ChoiceOptionLabels[7];

/* Display order starts with training, followed by the ten story missions. */
enum { RECORD_STAGE_COUNT = 11 };
extern packed_stage_id RecordStageOrder[RECORD_STAGE_COUNT];
extern short RecordCharacter;
extern short RecordExiting;
extern Sprite3D *RecordNameplates[N_PLAYABLE_CHARACTERS];
extern Sprite3D *RecordRankSprites[N_STAGE_RANKS];
extern Sprite3D *RecordStageLabels[RECORD_STAGE_COUNT];
enum { RECORD_COLUMNS, RECORD_RANK_BACKING, RECORD_COLON, RECORD_EXIT_HELP };
extern Sprite3D *RecordWidgets[4];
extern short RecordRanks[N_PLAYABLE_CHARACTERS][RECORD_STAGE_COUNT][N_STAGE_LAYOUTS];
typedef struct OptionRecord
{
    int score;
    int time; /* Frames while collecting records; seconds when drawing. */
} OptionRecord;
extern OptionRecord OptionRecords[N_PLAYABLE_CHARACTERS][RECORD_STAGE_COUNT];
extern char RecordNumberFormat[4];
extern char RecordSecondsFormat[8];

enum
{
    MENU_EXTRA_LANGUAGES = 0x20,
    MENU_CHEAT_LANGUAGES = 0x800
};

void RunOptionsMenu(void);
void SetupOptionsImages(short load);
short UpdateOptionsList(short mode);
short UpdateControlOptions(short mode);
short UpdateChoiceOptions(short mode);
short UpdateVolumeOptions(short mode);
short UpdateSoundTest(short mode);
short UpdateRecordOptions(short mode);
int UnusedOptionHandler(void);
void DrawOptionTwoDigits(short value, short x, short y);
void DrawOptionNumber(char *text, short x, short y);

#endif
