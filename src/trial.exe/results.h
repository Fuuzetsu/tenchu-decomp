#ifndef TENCHU_TRIAL_RESULTS_H
#define TENCHU_TRIAL_RESULTS_H

#include "mission_menu.h"

enum
{
    TRIAL_RESULTS_SUCCESS = 0,
    TRIAL_RESULTS_FAILURE,
    TRIAL_RESULTS_NEW_RECORD,
    TRIAL_RESULTS_SAVE
};

/* Each page handles resource setup, input, drawing, and resource disposal. */
enum
{
    TRIAL_RESULT_PAGE_INIT = 0,
    TRIAL_RESULT_PAGE_INPUT,
    TRIAL_RESULT_PAGE_DRAW,
    TRIAL_RESULT_PAGE_DISPOSE
};

extern s32 TrialRecordRank, TrialResultTime;
extern u_long TrialResultsNewButtons;
typedef short (*TrialResultPage)(short operation);
extern TrialResultPage TrialResultPages[4];

typedef struct TrialResultStatFields
{
    short *values[4];
} TrialResultStatFields;
extern const TrialResultStatFields TrialResultStats;
extern const u8 TrialResultsEmptyName[];

extern TrialImagePanel *TrialSuccessPanel, *TrialFailurePanel;
extern GsSPRITE *TrialSuccessDigits[2], *TrialFailureDigits;
extern ArcFile *TrialSuccessArchive, *TrialFailureArchive;
extern u32 TrialSuccessTextFrames, TrialFailureTextFrames;
extern s32 TrialSuccessTitleX, TrialFailureTitleX;

short ShowTrialResultPage(short page, short width);
short UpdateTrialSuccessPage(short operation);
short UpdateTrialFailurePage(short operation);
short UpdateTrialNewRecordPage(short operation);

#endif
