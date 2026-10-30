#ifndef TENCHU_TRIAL_MISSION_HUD_H
#define TENCHU_TRIAL_MISSION_HUD_H

#include "trial.exe.h"

extern GsSPRITE TrialTimerDigits[11];
extern GsSPRITE TrialTimeNotices[4];
extern GsSPRITE TrialFailureBanner;
extern GsSPRITE TrialEnemyCandles[3];
extern short TrialTimeNoticeIndex, TrialTimeNoticeTicks;
extern short TrialRemainingEnemies;

enum
{
    TRIAL_TIME_NOTICE_RESET = -2,
    TRIAL_TIME_NOTICE_DRAW = -1,
    TRIAL_TIME_NOTICE_STEALTH_KILL = 0, /* -15 seconds */
    TRIAL_TIME_NOTICE_KILL = 1,         /* -5 seconds */
    TRIAL_TIME_NOTICE_SPOTTED = 2,      /* +10 seconds */
    TRIAL_TIME_NOTICE_CIVILIAN = 3      /* +30 seconds */
};

void DrawTrialMissionTimer(int ticks);
int UpdateTrialTimeNotice(short event);
void DrawTrialEnemyCandles(void);
void DrawTrialFailureBanner(void);

#endif
