#include "common.h"
#include "main.exe.h"

void reset_alert_duration(void)
{
    s32 duration;

    duration = ALERT_DURATION;
    if (gNannido == DIFFICULTY_HARD)
    {
        duration = ALERT_DURATION_HARD;
    }
    EmergencyNotice = duration;
}
