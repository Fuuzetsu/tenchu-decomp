#include "common.h"
#include "mission_menu.h"
#include "card.h"

enum trial_card_load_phase
{
    CARD_LOAD_INIT = 0,
    CARD_LOAD_DIRECTORY = 1,
    CARD_LOAD_BEGIN = 10,
    CARD_LOAD_WAIT,
    CARD_LOAD_CHECK,
    CARD_LOAD_HEADERS_WAIT_1 = 20,
    CARD_LOAD_HEADERS_WAIT_2,
    CARD_LOAD_HEADER,
    CARD_LOAD_HEADERS_CANCELLED = 24,
    CARD_LOAD_HEADERS_COMPLETE = 26,
    CARD_LOAD_MISSION_BEGIN = 30,
    CARD_LOAD_MISSION_WAIT,
    CARD_LOAD_MISSION,
    CARD_LOAD_FAILED = 34,
    CARD_LOAD_FAILURE_ACKNOWLEDGED,
    CARD_LOAD_VERIFY,
    CARD_LOAD_INVALID = 38,
    CARD_LOAD_INVALID_ACKNOWLEDGED,
    CARD_LOAD_MISSING = 60,
    CARD_LOAD_MISSING_ACKNOWLEDGED,
    CARD_LOAD_COMPLETE = 99
};

/* Browsing reads only the headers, in reverse directory order. Selecting
 * a mission loads its complete card block into TrialMission and validates it. */
short UpdateTrialCardLoad(u_long pressed, TrialMissionHeader *headers, short whole_mission)
{
    char filename[32];
    int checksum;

    setup_card_screen_(CARD_SCREEN_RESOURCES_ACQUIRE);
    switch (TrialCardLoadState)
    {
    case CARD_LOAD_INIT:
        TrialCardLoadState = CARD_LOAD_DIRECTORY;
        TrialCardLoadRetry = 0;
        break;
    case CARD_LOAD_DIRECTORY:
        TrialCardDirectoryCursor = 0;
        memset(TrialCardDirectory, 0, sizeof(TrialCardDirectory));
        if (whole_mission)
            sprintf(filename, fmt_trial_card_path, TrialCardMissionPrefix, headers->checksum);
        else
            sprintf(filename, fmt_trial_card_pattern, TrialCardMissionPattern);
        TrialCardDirectoryCursor = GetTrialCardDirectory(filename);
        if (TrialCardDirectoryCursor <= 0)
        {
            TrialCardStatus = __builtin_abs(TrialCardDirectoryCursor);
            if (whole_mission)
                TrialCardLoadState = TrialCardStatus == CARD_RESULT_NO_CARD
                    ? CARD_LOAD_MISSING : CARD_LOAD_FAILED;
            else
                TrialCardLoadState = CARD_LOAD_BEGIN;
        }
        else
            TrialCardLoadState = CARD_LOAD_BEGIN;
        if (TrialCardLoadState != CARD_LOAD_BEGIN)
            TrialCardLoadState = TrialCardLoadRetry++ < 3
                ? CARD_LOAD_DIRECTORY : TrialCardLoadState;
        break;
    case CARD_LOAD_BEGIN:
        TrialCardLoadPage = whole_mission ? TRIAL_CARD_PAGE_READING : CARD_PAGE_CHECKING;
    case CARD_LOAD_WAIT:
        TrialCardLoadRetry = 0;
        TrialCardLoadState++;
        break;
    case CARD_LOAD_CHECK:
        TrialCardStatus = ChkCard();
        switch (TrialCardStatus)
        {
        case CARD_RESULT_SUCCESS:
            TrialCardLoadState = whole_mission ? CARD_LOAD_MISSION_BEGIN : CARD_LOAD_HEADERS_WAIT_2;
            break;
        case CARD_RESULT_NEW_CARD:
            TrialCardLoadState = whole_mission ? CARD_LOAD_FAILED : CARD_LOAD_HEADERS_WAIT_2;
            break;
        case CARD_RESULT_NO_CARD:
            TrialCardLoadState = whole_mission ? CARD_LOAD_MISSING : -1;
            /* The original falls through and replaces MISSING with FAILED. */
        default:
            TrialCardLoadState = whole_mission ? CARD_LOAD_FAILED : -1;
            break;
        }
        if (TrialCardLoadState == CARD_LOAD_FAILED ||
            TrialCardLoadState == CARD_LOAD_MISSING || TrialCardLoadState == -1)
            TrialCardLoadState = TrialCardLoadRetry++ < 3
                ? CARD_LOAD_CHECK : TrialCardLoadState;
        break;
    case CARD_LOAD_HEADERS_WAIT_1:
    case CARD_LOAD_HEADERS_WAIT_2:
        TrialCardLoadState++;
        break;
    case CARD_LOAD_HEADER:
        if (--TrialCardDirectoryCursor < 0)
        {
            TrialCardLoadState = -1;
            break;
        }
        TrialCardStatus = LoadTrialCard(0, TrialCardDirectory[TrialCardDirectoryCursor].name,
            &headers[TrialCardDirectoryCursor], 0x100);
        if (TrialCardStatus == CARD_RESULT_SUCCESS)
            TrialCardLoadState = TrialCardDirectoryCursor
                ? CARD_LOAD_HEADER : CARD_LOAD_HEADERS_COMPLETE;
        else
            TrialCardLoadState = -1;
        headers[TrialCardDirectoryCursor].checksum &=
            TRIAL_MISSION_CHECKSUM_MASK | TRIAL_MISSION_PRESERVED_CHECKSUM_BIT;
        headers[TrialCardDirectoryCursor].flags &= TRIAL_OBJECTIVE_MASK | TRIAL_REQUIRE_STEALTH;
        headers[TrialCardDirectoryCursor].theme =
            __builtin_abs(headers[TrialCardDirectoryCursor].theme) % TRIAL_THEME_COUNT;
        break;
    case CARD_LOAD_HEADERS_COMPLETE:
        TrialCardLoadState = CARD_LOAD_COMPLETE;
        break;
    case CARD_LOAD_MISSION_BEGIN:
        TrialCardLoadPage = TRIAL_CARD_PAGE_READING;
    case CARD_LOAD_MISSION_WAIT:
        TrialCardLoadRetry = 0;
        TrialCardLoadState++;
        break;
    case CARD_LOAD_MISSION:
        sprintf(filename, fmt_trial_card_path, TrialCardMissionPrefix, headers->checksum);
        TrialCardStatus = LoadTrialCard(0, (u8 *)filename, NULL, BLOCKSIZE);
        switch (TrialCardStatus)
        {
        case CARD_RESULT_SUCCESS:
            TrialCardLoadState = CARD_LOAD_VERIFY;
            break;
        case CARD_RESULT_NO_CARD:
            TrialCardLoadState = CARD_LOAD_MISSING;
            break;
        default:
            TrialCardLoadState = CARD_LOAD_FAILED;
            break;
        }
        if (TrialCardLoadState != CARD_LOAD_VERIFY)
            TrialCardLoadState = TrialCardLoadRetry++ < 3
                ? CARD_LOAD_MISSION : TrialCardLoadState;
        break;
    case CARD_LOAD_FAILED:
        TrialCardLoadPage = TRIAL_CARD_PAGE_READ_FAILED;
        break;
    case CARD_LOAD_VERIFY:
        checksum = CalculateTrialMissionChecksum();
        TrialMission.header.flags &= TRIAL_OBJECTIVE_MASK | TRIAL_REQUIRE_STEALTH;
        TrialMission.header.theme = __builtin_abs(TrialMission.header.theme) % TRIAL_THEME_COUNT;
        /* Signed comparisons preserve rejection of corrupt high-bit IDs.
         * The original accepts up to sixteen collision-avoidance increments. */
        if (__builtin_abs(checksum) <= (s32)TrialMission.header.checksum &&
            (s32)TrialMission.header.checksum <= __builtin_abs(checksum + 16))
            TrialCardLoadState = CARD_LOAD_COMPLETE;
        else
            TrialCardLoadState = CARD_LOAD_INVALID;
        break;
    case CARD_LOAD_INVALID:
        TrialCardLoadPage = TRIAL_CARD_PAGE_INVALID_MISSION;
        break;
    case CARD_LOAD_MISSING:
        TrialCardLoadPage = TRIAL_CARD_PAGE_NO_CARD;
        break;
    case CARD_LOAD_HEADERS_CANCELLED:
    case CARD_LOAD_FAILURE_ACKNOWLEDGED:
    case CARD_LOAD_INVALID_ACKNOWLEDGED:
    case CARD_LOAD_MISSING_ACKNOWLEDGED:
        TrialCardLoadState = -1;
        break;
    default:
        TrialCardLoadPage = 0;
        setup_card_screen_(CARD_SCREEN_RESOURCES_RELEASE);
        if (TrialCardLoadState < 0)
        {
            TrialCardLoadState = 0;
            return 1;
        }
        TrialCardLoadState = 0;
        return -1;
    }
    TrialCardLoadState += draw_card_help_(TrialCardLoadPage, pressed);
    return 0;
}
