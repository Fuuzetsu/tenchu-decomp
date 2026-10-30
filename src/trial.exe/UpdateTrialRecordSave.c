#include "common.h"
#include "mission_menu.h"

/* STATUS: NON_MATCHING. The typed time store lets cc1 reuse TrialPresetId;
 * retail reloads it between the time and name writes. A scalar pointer store
 * restores that reload but hoists the fixed time base, leaving 320 bytes
 * against the original 316. Keep the clean typed draft and assembly fallback. */
#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/trial.exe/asm/nonmatchings/UpdateTrialRecordSave", UpdateTrialRecordSave);
#else
/* Preset leaderboards live in the extended TENCHU save. User-stage records
 * remain in their mission file. The card state machine handles both cases. */
short UpdateTrialRecordSave(short active)
{
    short result = 0;
    int i;
    u_long previous_buttons;

    switch (active)
    {
    case 0:
        break;
    case 1:
        if (!TrialUserMission)
        {
            for (i = 0; i < TRIAL_MISSION_RECORD_COUNT; i++)
            {
                TRIAL_PERSISTENT_STATE->preset_records[TrialPresetId][i].time =
                    TrialMission.header.records[i].time;
                __builtin_memcpy(TRIAL_PRESET_RECORD_NAME(TrialPresetId, i).text,
                    TrialMission.header.records[i].name.text, 10);
            }
        }
        previous_buttons = TrialResultsButtons;
        TrialResultsButtons = GetRealPad(0);
        switch (UpdateTrialCardSave((previous_buttons ^ TrialResultsButtons) & TrialResultsButtons, 1))
        {
        case -1:
            result = 1;
            break;
        case 1:
            result = -1;
            break;
        default:
            result = 0;
            break;
        }
        break;
    }
    return result;
}
#endif
