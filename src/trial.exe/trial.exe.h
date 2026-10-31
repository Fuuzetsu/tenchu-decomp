/* TRIAL shares engine types and the persistent handoff with MAIN. */
#ifndef TRIAL_EXE_H
#define TRIAL_EXE_H

#include "../main.exe/main.exe.h"

/* TRIAL reserves 74 KiB per graphics packet page. */
enum { TRIAL_PACKET_PAGE_SIZE = 0x12800 };
extern u8 Packet[][TRIAL_PACKET_PAGE_SIZE];

#define TRIAL_STATE ((TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS)

/* Values below 100 select TRIAL<n><R/A>.VAB; larger values select the
 * campaign STAGE<n><R/A>.VAB banks. The editor menu uses STAGE9R.VAB. */
enum
{
    TRIAL_CAMPAIGN_SOUND_BASE = 100,
    TRIAL_MENU_SOUND_SET = TRIAL_CAMPAIGN_SOUND_BASE + 9
};

void RunMissionEditor(void);
short ShowTrialMainMenu(void);
short SelectTrialMission(short saved_only);
short ChooseTrialCharacter(void);
short ConfirmTrialMission(void);
short ConfigureTrialMission(short editing);
short VerifyTrialMissionPassword(void);
short PrepareTrialMemoryCard(void);
short RunTrialMission(short character);
void SetupTrialMission(short character, short mode);
void ActivateTrialHumans(short initial);
short ShowTrialResults(short failed);
void EditTrialMission(void);
void DrawTrialScreenFade(short brightness, short width, short height, short release);

extern game_language TrialPreviousLanguage;
extern ArcFile *TrialImageArchive;
extern char TrialTextEntry[32];
extern char path_trial_image_archive[];
extern Sprite3D *TrialMenuCursor;
extern BackGround *TrialMenuBackground;
/* The selected mission's preset index and save-file kind occupy adjacent
 * halfwords. A user mission saves to its Mxxxxxxx file instead of TENCHU. */
typedef struct TrialMissionSelection
{
    s16 preset_id;
    s16 is_user_mission;
} TrialMissionSelection;
extern TrialMissionSelection TrialSelectedMission;
/* Mission execution enables pause input while the HUD is active. */
extern s16 InfoViewPauseEnabled;

void InitTrialMenuGraphics(void);
void DisposeTrialMenuGraphics(void);
void LoadTrialMenuGraphics(void);
void LoadTrialArchiveImage(ArcFile *archive, int index);
void ClearTrialDisplay(short width, short height);
u_long ReadTrialMenuPad(short port, u_long *previous_buttons, short inhibit);

/* These values are returned by the editor's four-entry main menu. */
enum trial_menu_mode
{
    TRIAL_MENU_EXIT = 0,
    TRIAL_MENU_SELECT = 1,
    TRIAL_MENU_PLAY = 10,
    TRIAL_MENU_CREATE = 20,
    TRIAL_MENU_EDIT = 30
};

/* The mission-list backdrop is a rectangular array of textured quads. */
typedef struct TrialImagePanel TrialImagePanel;
struct TrialImagePanel
{
    POLY_FT4 *tiles;     /* 0x00: row-major, columns * rows */
    s16 x, y;           /* 0x04: top-left screen position */
    s16 tile_width;     /* 0x08 */
    s16 tile_height;    /* 0x0a */
    u16 columns, rows;  /* 0x0c: allocated grid */
    u16 draw_columns;   /* 0x10: visible part of each row */
    u16 draw_rows;      /* 0x12 */
    GsIMAGE image;      /* 0x14 */
}; /* 0x30 */

void InitTrialImagePanel(TrialImagePanel *panel);
void SetupTrialImagePanel(TrialImagePanel *panel, ArcFile *archive, short index);
void DrawTrialTime(GsSPRITE *sprite, s32 ticks, s32 x, s32 y);
s32 InsertTrialRecord(s32 ticks);
void DrawTrialImagePanel(TrialImagePanel *panel, u16 depth);
void DisposeTrialImagePanel(TrialImagePanel *panel);
void DrawTrialNumber(GsSPRITE *sprite, s16 value, s16 x, s16 y);
GsSPRITE *CreateTrialSprite(ArcFile *archive, int index);
void ReturnFromTrialToMenu(void);

void ClearTrialPages(void);
void SetupTrialSprite(GsSPRITE *sprite, u_long *data, int x, int y,
                      int brightness, u32 attributes);
void DrawTrialSpriteFrame(GsSPRITE *sprite, int frame);

#endif
