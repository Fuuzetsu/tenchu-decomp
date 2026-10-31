#include "common.h"
#include "mission_menu.h"

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
        if (!TrialSelectedMission.is_user_mission)
        {
            for (i = 0; i < TRIAL_MISSION_RECORD_COUNT; i++)
            {
                TRIAL_PERSISTENT_STATE->preset_records[TrialSelectedMission.preset_id][i].time =
                    TrialMission.header.records[i].time;
                __builtin_memcpy(TRIAL_PRESET_RECORD_NAME(TrialSelectedMission.preset_id, i).text,
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
