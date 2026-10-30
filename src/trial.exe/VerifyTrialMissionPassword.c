#include "common.h"
#include "mission_menu.h"

/* Passwords contain two-byte Shift-JIS characters. Empty passwords allow
 * editing immediately; a mismatch offers another attempt or cancellation. */
short VerifyTrialMissionPassword(void)
{
    short phase = 1;
    int accepted = 0;
    int difference;

    while (phase)
    {
        switch (phase)
        {
        case 1:
            if (TrialMission.header.password[0] == 0 && TrialMission.header.password[1] == 0)
                phase = 4;
            else
                phase = 2;
            break;
        case 2:
            memset(TrialTextEntry, 0, sizeof(TrialTextEntry));
            ShowTrialTextEntry(TRIAL_TEXT_PASSWORD);
            difference = strcmp(TrialMission.header.password, TrialTextEntry);
            memset(TrialTextEntry, 0, sizeof(TrialTextEntry));
            if (difference)
                phase = 3;
            else
                phase = 4;
            break;
        case 3:
            phase = AskRetryTrialPassword() ? 2 : 0;
            if (!phase)
                accepted = 0;
            break;
        default:
            phase = 0;
            accepted = 1;
            break;
        }
    }
    return accepted;
}
