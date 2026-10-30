#include "common.h"
#include "results.h"
#include "text_entry.h"

/* Quantize the clock to seconds, optionally insert a leaderboard entry,
 * then offer name entry and saving before returning the retry/menu choice. */
short ShowTrialResults(short failed)
{
    short phase = 1;
    short new_record = 0;
    short result;

    TrialResultTime = GameClock / 30 * 30;
    if (TrialResultTime >= 108000)
        TrialResultTime = 107970;
    if (failed || (TrialRecordRank = InsertTrialRecord(TrialResultTime)) == -1)
        phase = 3;
    if (TrialRecordRank < 0)
        phase = 3;

    while (phase)
    {
        switch (phase)
        {
        case 1:
            phase = ShowTrialResultPage(TRIAL_RESULTS_NEW_RECORD, 320) ? 2 : 3;
            new_record = 1;
            break;
        case 2:
            EditTrialTextField(TRIAL_TEXT_RECORD_NAME,
                TrialMission.header.records[TrialRecordRank].name.text, 8);
            phase = 3;
            break;
        case 3:
            result = ShowTrialResultPage(failed, 512);
            phase = 4;
            break;
        case 4:
            if (new_record)
                ShowTrialResultPage(TRIAL_RESULTS_SAVE, 320);
            phase++;
            break;
        default:
            phase = 0;
            break;
        }
    }
    if (failed == 1)
        result = !result;
    return result;
}
