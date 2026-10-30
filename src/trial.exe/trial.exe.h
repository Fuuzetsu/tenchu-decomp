/* TRIAL shares engine types and the persistent handoff with MAIN. */
#ifndef TRIAL_EXE_H
#define TRIAL_EXE_H

#include "../main.exe/main.exe.h"

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
short ShowTrialResults(short failed);
void EditTrialMission(void);
void DrawTrialScreenFade(short brightness, short width, short height, short release);

extern game_language TrialPreviousLanguage;
extern ArcFile *TrialImageArchive;
extern char TrialTextEntry[32];
extern char path_trial_image_archive[];
extern Sprite3D *TrialMenuCursor;
extern BackGround *TrialMenuBackground;
/* Zero selects preset scores in TENCHU; one selects a user Mxxxxxxx file. */
extern short TrialUserMission;

void InitTrialMenuGraphics(void);
void DisposeTrialMenuGraphics(void);
void LoadTrialMenuGraphics(void);
void LoadTrialArchiveImage(ArcFile *archive, int index);
void SetTrialDisplaySize(short width, short height);
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

void ClearTrialPages(void);
void SetupTrialSprite(GsSPRITE *sprite, u_long *data, int x, int y,
                      int brightness, u32 attributes);
void DrawTrialSpriteFrame(GsSPRITE *sprite, int frame);

#endif
