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
extern DIRENTRY TrialCardDirectory[TRIAL_CARD_DIRECTORY_CAPACITY];

void PrepareTrialCardIcon(TCardHeader *header, s16 operation);
u32 CalculateTrialMissionChecksum(void);
card_result CheckTrialCard(void);
s16 GetTrialCardDirectory(char *pattern);
card_result LoadTrialCard(s32 target, u8 *path, void *header, s32 size);

#endif
