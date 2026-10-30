#ifndef TENCHU_TRIAL_CARD_H
#define TENCHU_TRIAL_CARD_H

#include "../main.exe/memcard.h"

enum trial_card_icon_operation
{
    TRIAL_CARD_ICON_LOAD = 0,
    TRIAL_CARD_ICON_COPY = 1,
    TRIAL_CARD_ICON_RELEASE = 2
};

enum
{
    TRIAL_CARD_THEME_COUNT = 12,
    TRIAL_CARD_DIRECTORY_CAPACITY = 15
};

extern ArcFile *TrialCardIconArchive;
extern card_result TrialCardStatus;
extern DIRENTRY TrialCardDirectory[TRIAL_CARD_DIRECTORY_CAPACITY];
extern s16 TrialCardCheckState;
extern s16 TrialCardCheckPage;
extern s16 TrialCardCheckRetry;
extern s16 TrialCardDeleteState;
extern s16 TrialCardDeletePage;
extern s16 TrialCardDeleteWait;
extern char fmt_trial_card_mission_name[];
extern s16 TrialCardLoadState;
extern s16 TrialCardLoadPage;
extern s16 TrialCardLoadRetry;
extern s32 TrialCardDirectoryCursor;
extern char TrialCardMissionPrefix[];
extern char TrialCardMissionPattern[];
extern char fmt_trial_card_path[];
extern char fmt_trial_card_pattern[];
extern s16 TrialCardSaveState;
extern s16 TrialCardSavePage;
extern s16 TrialCardSaveRetry;
extern u32 TrialCardPendingChecksum;
extern u32 TrialCardPreviousChecksum;
extern s16 TrialCardOverwriteGame;
extern char TrialCardGamePath[];
extern char TrialCardGameFilename[];

/* Additional pages in the shared CARD_J.TXT used only by the editor. */
enum trial_card_page
{
    TRIAL_CARD_PAGE_NO_CARD = 1,
    TRIAL_CARD_PAGE_READING = 6,
    TRIAL_CARD_PAGE_READ_FAILED = 13,
    TRIAL_CARD_PAGE_SAVE_NEW_PROMPT = 28,
    TRIAL_CARD_PAGE_SAVE_PROMPT = 29,
    TRIAL_CARD_PAGE_OVERWRITE_PROMPT = 30,
    TRIAL_CARD_PAGE_RETRY_SAVE = 32,
    TRIAL_CARD_PAGE_DELETE_PROMPT = 33,
    TRIAL_CARD_PAGE_CONFIRM_DELETE,
    TRIAL_CARD_PAGE_DELETING,
    TRIAL_CARD_PAGE_DELETED,
    TRIAL_CARD_PAGE_DELETE_FAILED,
    TRIAL_CARD_PAGE_NO_CARD_CANNOT_EDIT = 38,
    TRIAL_CARD_PAGE_UNFORMATTED_CANNOT_EDIT = 40,
    TRIAL_CARD_PAGE_INVALID_MISSION = 42,
    TRIAL_CARD_PAGE_FILE_NOT_FOUND = 43
};

void PrepareTrialCardIcon(TCardHeader *header, s16 operation);
u32 CalculateTrialMissionChecksum(void);
card_result CheckTrialCard(void);
s16 GetTrialCardDirectory(char *pattern);
card_result LoadTrialCard(s32 target, u8 *path, void *header, s32 size);

#endif
