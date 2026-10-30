#include "common.h"
#include "mission_menu.h"
#include "card.h"

extern char TrialCardAllFiles[]; /* "*", shared card-UI small data */

enum trial_card_check_phase
{
    CARD_CHECK_INIT = 0,
    CARD_CHECK_DIRECTORY = 1,
    CARD_CHECK_SPACE = 10,
    CARD_CHECK_UNFORMATTED = 30,
    CARD_CHECK_FULL = 40,
    CARD_CHECK_DAMAGED = 50,
    CARD_CHECK_MISSING = 60,
    CARD_CHECK_COMPLETE = 99
};

/* Creation can proceed after acknowledging that saving will be unavailable.
 * Browsing existing missions instead warns when the card cannot be edited.
 * Return zero while drawing, -1 to proceed, or 1 when the user cancels. */
short UpdateTrialCardCheck(u_long pressed, short existing_mission)
{
    s32 count;
    int i, used_bytes;

    setup_card_screen_(CARD_SCREEN_RESOURCES_ACQUIRE);
    switch (TrialCardCheckState)
    {
    case CARD_CHECK_INIT:
        TrialCardCheckRetry = 0;
        TrialCardCheckState = CARD_CHECK_DIRECTORY;
        break;
    case CARD_CHECK_DIRECTORY:
        memset(TrialCardDirectory, 0, sizeof(TrialCardDirectory));
        TrialCardStatus = MemCardGetDirentry(MEMCARD_CHANNEL_0, TrialCardAllFiles,
            TrialCardDirectory, &count, 0, TRIAL_CARD_DIRECTORY_CAPACITY);
        switch (TrialCardStatus)
        {
        case CARD_RESULT_SUCCESS:
            TrialCardCheckState = CARD_CHECK_SPACE;
            break;
        case CARD_RESULT_NO_CARD:
            TrialCardCheckState = CARD_CHECK_MISSING;
            break;
        case CARD_RESULT_DAMAGED:
            TrialCardCheckState = CARD_CHECK_DAMAGED;
            break;
        case CARD_RESULT_UNFORMATTED:
            TrialCardCheckState = CARD_CHECK_UNFORMATTED;
            McardStateFlag = 0;
            break;
        default:
            TrialCardCheckState = -1;
            break;
        }
        if (TrialCardCheckState == -1)
            TrialCardCheckState = TrialCardCheckRetry++ < 3
                ? CARD_CHECK_DIRECTORY : TrialCardCheckState;
        break;
    case CARD_CHECK_SPACE:
        i = 0;
        used_bytes = 0;
        for (; i < TRIAL_CARD_DIRECTORY_CAPACITY; i++)
            used_bytes += TrialCardDirectory[i].size;
        if (used_bytes >= TRIAL_CARD_DIRECTORY_CAPACITY * BLOCKSIZE)
        {
            TrialCardCheckState = CARD_CHECK_FULL;
            break;
        }
        TrialCardCheckState = CARD_CHECK_COMPLETE;
        break;
    case CARD_CHECK_UNFORMATTED:
        if (existing_mission)
            TrialCardCheckPage = TRIAL_CARD_PAGE_UNFORMATTED_CANNOT_EDIT;
        else
            TrialCardCheckState = CARD_CHECK_COMPLETE;
        break;
    case CARD_CHECK_FULL:
        if (!existing_mission)
            TrialCardCheckPage = CARD_PAGE_NOT_ENOUGH_SPACE_CANNOT_SAVE_PROMPT;
        else
            TrialCardCheckState = CARD_CHECK_COMPLETE;
        break;
    case CARD_CHECK_DAMAGED:
        TrialCardCheckPage = CARD_PAGE_DAMAGED_CANNOT_SAVE_PROMPT;
        break;
    case CARD_CHECK_MISSING:
        if (!existing_mission)
            TrialCardCheckPage = CARD_PAGE_NO_CARD_CANNOT_SAVE_PROMPT;
        else
            TrialCardCheckPage = TRIAL_CARD_PAGE_NO_CARD_CANNOT_EDIT;
        break;
    case CARD_CHECK_UNFORMATTED + 1:
    case CARD_CHECK_FULL + 1:
    case CARD_CHECK_DAMAGED + 1:
    case CARD_CHECK_MISSING + 1:
        TrialCardCheckState = CARD_CHECK_COMPLETE;
        break;
    case CARD_CHECK_UNFORMATTED + 2:
    case CARD_CHECK_FULL + 2:
    case CARD_CHECK_DAMAGED + 2:
    case CARD_CHECK_MISSING + 2:
        TrialCardCheckState = -1;
        break;
    default:
        TrialCardCheckPage = 0;
        TrialCardCheckRetry = 0;
        setup_card_screen_(CARD_SCREEN_RESOURCES_RELEASE);
        if (TrialCardCheckState < 0)
        {
            TrialCardCheckState = 0;
            return 1;
        }
        TrialCardCheckState = 0;
        return -1;
    }
    TrialCardCheckState += draw_card_help_(TrialCardCheckPage, pressed);
    return 0;
}
