#include "common.h"
#include "mission_editor.h"
#include "card.h"

enum trial_card_save_phase
{
    SAVE_CHECK_CARD = 0,
    SAVE_CHECK_ID = 2,
    SAVE_FIND_FILE = 4,
    SAVE_ACCEPT_CARD = 8,
    SAVE_ACCEPT_RETRY,
    SAVE_NEW_PROMPT,
    SAVE_NEW_ACCEPTED,
    SAVE_NEW_CANCELLED,
    SAVE_NEW_WAIT_1,
    SAVE_NEW_WAIT_2,
    SAVE_NEW_CHECKSUM,
    SAVE_NEW_FILE,
    SAVE_OVERWRITE_PROMPT = 20,
    SAVE_OVERWRITE_ACCEPTED,
    SAVE_OVERWRITE_DECLINED,
    SAVE_AS_NEW_ACCEPTED,
    SAVE_AS_NEW_CANCELLED,
    SAVE_OVERWRITE_WAIT,
    SAVE_OVERWRITE_CHECK,
    SAVE_DELETE_OLD_FILE,
    SAVE_REPLACE_FILE,
    SAVE_FORMAT_PROMPT = 30,
    SAVE_FORMAT_ACCEPTED,
    SAVE_FORMAT_CANCELLED,
    SAVE_FORMAT_WAIT_1,
    SAVE_FORMAT_WAIT_2,
    SAVE_FORMAT_CARD,
    SAVE_FORMAT_COMPLETE,
    SAVE_FORMAT_FAILED = 38,
    SAVE_FORMAT_FAILURE_ACKNOWLEDGED,
    SAVE_FULL_PROMPT,
    SAVE_FULL_EXIT,
    SAVE_FULL_RETRY,
    SAVE_DAMAGED_PROMPT = 50,
    SAVE_DAMAGED_EXIT,
    SAVE_DAMAGED_RETRY,
    SAVE_MISSING_PROMPT = 60,
    SAVE_MISSING_EXIT,
    SAVE_MISSING_RETRY,
    SAVE_FILE_NOT_FOUND = 65,
    SAVE_FILE_NOT_FOUND_ACKNOWLEDGED,
    SAVE_FINISHED = 70,
    SAVE_FINISHED_ACKNOWLEDGED,
    SAVE_FAILED,
    SAVE_FAILURE_RETRY,
    SAVE_FAILURE_CANCELLED,
    SAVE_ABANDON_PROMPT = 80,
    SAVE_ABANDON_ACCEPTED,
    SAVE_ABANDON_CANCELLED,
    SAVE_COMPLETE = 99,
    SAVE_CREATE_GAME,
    SAVE_OVERWRITE_GAME,
    SAVE_WRITE_GAME
};

/* Preset records share TENCHU with campaign progress; user missions get a
 * checksum-named Mxxxxxxx file. Replacing an edited mission clears its old
 * leaderboard. Record-only saves preserve it and use the same card flow. */
short UpdateTrialCardSave(u_long pressed, short records_only)
{
    char filename[24];

    setup_card_screen_(CARD_SCREEN_RESOURCES_ACQUIRE);
    switch (TrialCardSaveState)
    {
    case SAVE_CHECK_CARD:
        TrialCardSaveRetry = 0;
        switch (CheckTrialCard())
        {
        case CARD_RESULT_SUCCESS:
        case CARD_RESULT_NEW_CARD:
            TrialCardSaveState = SAVE_CHECK_ID;
            break;
        case CARD_RESULT_NO_CARD:
            TrialCardSaveState = SAVE_MISSING_PROMPT;
            break;
        case CARD_RESULT_DAMAGED:
            TrialCardSaveState = SAVE_DAMAGED_PROMPT;
            break;
        default:
            TrialCardSaveState = SAVE_FAILED;
            break;
        }
        break;
    case SAVE_CHECK_ID:
        if (!TrialSelectedMission.is_user_mission || TrialMission.header.checksum)
            TrialCardSaveState = SAVE_FIND_FILE;
        else
            TrialCardSaveState = SAVE_ACCEPT_CARD;
        break;
    case SAVE_FIND_FILE:
        if (TrialSelectedMission.is_user_mission)
            sprintf(filename, fmt_trial_card_path, TrialCardMissionPrefix,
                TrialMission.header.checksum);
        else
            sprintf(filename, fmt_trial_card_pattern, TrialCardGamePath);
        if (GetTrialCardDirectory(filename) > 0)
            TrialCardSaveState = SAVE_OVERWRITE_PROMPT;
        else
            TrialCardSaveState = SAVE_ACCEPT_CARD;
        break;
    case SAVE_ACCEPT_CARD:
        TrialCardSaveRetry = 0;
        TrialCardSaveState++;
        /* Recheck after directory lookup, including unformatted cards. */
    case SAVE_ACCEPT_RETRY:
        switch (ChkCard())
        {
        case CARD_RESULT_SUCCESS:
        case CARD_RESULT_NEW_CARD:
            TrialCardSaveState = SAVE_NEW_PROMPT;
            break;
        case CARD_RESULT_NO_CARD:
            TrialCardSaveState = SAVE_MISSING_PROMPT;
            break;
        case CARD_RESULT_DAMAGED:
            TrialCardSaveState = SAVE_DAMAGED_PROMPT;
            break;
        case CARD_RESULT_UNFORMATTED:
            TrialCardSaveState = SAVE_FORMAT_PROMPT;
            McardStateFlag = 0;
            break;
        default:
            TrialCardSaveState = SAVE_FAILED;
            break;
        }
        if (TrialCardSaveState != SAVE_NEW_PROMPT)
            TrialCardSaveState = TrialCardSaveRetry++ < 3
                ? SAVE_ACCEPT_RETRY : TrialCardSaveState;
        break;
    case SAVE_NEW_PROMPT:
        TrialCardSavePage = TRIAL_CARD_PAGE_SAVE_PROMPT;
        break;
    case SAVE_NEW_ACCEPTED:
        TrialCardSavePage = CARD_PAGE_WRITING;
        TrialCardSaveRetry = 0;
        TrialCardSaveState = SAVE_NEW_WAIT_1;
        break;
    case SAVE_NEW_CHECKSUM:
        if (!TrialSelectedMission.is_user_mission)
        {
            TrialCardSaveState = SAVE_CREATE_GAME;
            break;
        }
        TrialCardPendingChecksum = CalculateTrialMissionChecksum();
        TrialCardPreviousChecksum = TrialMission.header.checksum;
    case SAVE_NEW_FILE:
        sprintf(filename, fmt_trial_card_mission_name, TrialCardPendingChecksum);
        TrialMission.header.checksum = TrialCardPendingChecksum;
        switch (SaveCard(0, (u8 *)filename, &TrialMission, sizeof(TrialMission), 0))
        {
        case CARD_RESULT_SUCCESS:
            TrialCardSaveState = SAVE_FINISHED;
            break;
        case CARD_RESULT_FILE_EXISTS:
            TrialCardSaveState = SAVE_NEW_FILE;
            TrialCardPendingChecksum =
                (TrialCardPendingChecksum & TRIAL_MISSION_PRESERVED_CHECKSUM_BIT) |
                ((TrialCardPendingChecksum + 1) & TRIAL_MISSION_CHECKSUM_MASK);
            break;
        case CARD_RESULT_FULL:
            TrialCardSaveState = SAVE_FULL_PROMPT;
            break;
        case CARD_RESULT_UNFORMATTED:
            McardStateFlag = 0;
            TrialCardSaveState = SAVE_FORMAT_PROMPT;
            break;
        case CARD_RESULT_NO_CARD:
            TrialCardSaveState = SAVE_MISSING_PROMPT;
            break;
        default:
            TrialCardSaveState = SAVE_FAILED;
            break;
        }
        if (TrialCardSaveState == SAVE_FULL_PROMPT ||
            TrialCardSaveState == SAVE_FORMAT_PROMPT ||
            TrialCardSaveState == SAVE_MISSING_PROMPT ||
            TrialCardSaveState == SAVE_FAILED)
        {
            TrialMission.header.checksum = TrialCardPreviousChecksum;
            TrialCardSaveState = TrialCardSaveRetry++ < 3
                ? SAVE_NEW_CHECKSUM : TrialCardSaveState;
        }
        break;
    case SAVE_OVERWRITE_PROMPT:
        if (!records_only)
            TrialCardSavePage = TRIAL_CARD_PAGE_OVERWRITE_PROMPT;
        else
            TrialCardSavePage = TrialSelectedMission.is_user_mission ? TRIAL_CARD_PAGE_OVERWRITE_PROMPT
                : CARD_PAGE_OVERWRITE_GAME_DATA_PROMPT;
        break;
    case SAVE_OVERWRITE_ACCEPTED:
        TrialCardSavePage = CARD_PAGE_WRITING;
        TrialCardSaveRetry = 0;
        TrialCardSaveState = SAVE_OVERWRITE_WAIT;
        break;
    case SAVE_OVERWRITE_DECLINED:
        if (!records_only)
            TrialCardSavePage = TRIAL_CARD_PAGE_SAVE_NEW_PROMPT;
        else
            TrialCardSaveState = SAVE_COMPLETE;
        break;
    case SAVE_OVERWRITE_CHECK:
        switch (ChkCard())
        {
        case CARD_RESULT_SUCCESS:
            break;
        case CARD_RESULT_NEW_CARD:
            TrialCardSaveState = SAVE_NEW_ACCEPTED;
            break;
        case CARD_RESULT_NO_CARD:
            TrialCardSaveState = SAVE_MISSING_PROMPT;
            break;
        case CARD_RESULT_DAMAGED:
            TrialCardSaveState = SAVE_DAMAGED_PROMPT;
            break;
        case CARD_RESULT_UNFORMATTED:
            TrialCardSaveState = SAVE_FORMAT_PROMPT;
            McardStateFlag = 0;
            break;
        default:
            TrialCardSaveState = SAVE_FAILED;
            break;
        }
        if (TrialCardSaveState != SAVE_OVERWRITE_CHECK)
        {
            TrialCardSaveState = TrialCardSaveRetry++ < 3
                ? SAVE_OVERWRITE_CHECK : TrialCardSaveState;
            break;
        }
        TrialCardSaveRetry = 0;
        if (!TrialSelectedMission.is_user_mission)
        {
            TrialCardSaveState = SAVE_OVERWRITE_GAME;
            break;
        }
        TrialCardSaveState = SAVE_DELETE_OLD_FILE;
        /* User stages are recreated under the new checksum. */
    case SAVE_DELETE_OLD_FILE:
        sprintf(filename, fmt_trial_card_mission_name, TrialMission.header.checksum);
        switch (DeleteCard((u8 *)filename))
        {
        case CARD_RESULT_SUCCESS:
            break;
        case CARD_RESULT_FILE_NOT_FOUND:
            TrialCardSaveState = SAVE_FILE_NOT_FOUND;
            break;
        default:
            TrialCardSaveState = SAVE_FAILED;
            break;
        }
        if (TrialCardSaveState != SAVE_DELETE_OLD_FILE)
        {
            TrialCardSaveState = TrialCardSaveRetry++ < 3
                ? SAVE_DELETE_OLD_FILE : TrialCardSaveState;
            break;
        }
        TrialCardSaveRetry = 0;
        TrialCardPendingChecksum = CalculateTrialMissionChecksum();
        if (!records_only && TrialEditorModified)
            memset(TrialMission.header.records, 0xff, sizeof(TrialMission.header.records));
        TrialCardSaveState = SAVE_REPLACE_FILE;
    case SAVE_REPLACE_FILE:
        sprintf(filename, fmt_trial_card_mission_name, TrialCardPendingChecksum);
        TrialMission.header.checksum = TrialCardPendingChecksum;
        switch (SaveCard(0, (u8 *)filename, &TrialMission, sizeof(TrialMission), 0))
        {
        case CARD_RESULT_SUCCESS:
            TrialCardSaveState = SAVE_FINISHED;
            break;
        case CARD_RESULT_FILE_EXISTS:
            TrialCardSaveState = SAVE_REPLACE_FILE;
            TrialCardPendingChecksum =
                (TrialCardPendingChecksum & TRIAL_MISSION_PRESERVED_CHECKSUM_BIT) |
                ((TrialCardPendingChecksum + 1) & TRIAL_MISSION_CHECKSUM_MASK);
            break;
        default:
            TrialCardSaveState = SAVE_FAILED;
            break;
        }
        if (TrialCardSaveState == SAVE_FAILED)
            TrialCardSaveState = TrialCardSaveRetry++ < 3
                ? SAVE_REPLACE_FILE : TrialCardSaveState;
        break;
    case SAVE_FORMAT_PROMPT:
        TrialCardSavePage = CARD_PAGE_FORMAT_PROMPT;
        if (CheckTrialCard())
        {
            TrialCardSaveState = 0;
            TrialCardSavePage = 0;
        }
        break;
    case SAVE_FORMAT_ACCEPTED:
        TrialCardSavePage = CARD_PAGE_FORMATTING;
        TrialCardSaveState = SAVE_FORMAT_WAIT_1;
        TrialCardSaveRetry = 0;
        break;
    case SAVE_NEW_WAIT_1:
    case SAVE_NEW_WAIT_2:
    case SAVE_OVERWRITE_WAIT:
    case SAVE_FORMAT_WAIT_1:
    case SAVE_FORMAT_WAIT_2:
        TrialCardSaveState++;
        break;
    case SAVE_FORMAT_CARD:
        if (FormatCard() == CARD_RESULT_SUCCESS)
            TrialCardSaveState = SAVE_FORMAT_COMPLETE;
        else
            TrialCardSaveState = TrialCardSaveRetry++ >= 3
                ? SAVE_FORMAT_FAILED : SAVE_FORMAT_CARD;
        break;
    case SAVE_AS_NEW_ACCEPTED:
    case SAVE_FORMAT_COMPLETE:
        TrialCardSaveState = SAVE_NEW_ACCEPTED;
        break;
    case SAVE_FORMAT_FAILED:
        TrialCardSavePage = CARD_PAGE_FORMAT_FAILED;
        break;
    case SAVE_FORMAT_CANCELLED:
    case SAVE_FORMAT_FAILURE_ACKNOWLEDGED:
        TrialCardSaveState = SAVE_ABANDON_PROMPT;
        break;
    case SAVE_FULL_PROMPT:
        TrialCardSavePage = CARD_PAGE_NOT_ENOUGH_SPACE_CANNOT_SAVE_PROMPT;
        break;
    case SAVE_DAMAGED_PROMPT:
        TrialCardSavePage = CARD_PAGE_DAMAGED_CANNOT_SAVE_PROMPT;
        break;
    case SAVE_MISSING_PROMPT:
        TrialCardSavePage = CARD_PAGE_NO_CARD_CANNOT_SAVE_PROMPT;
        break;
    case SAVE_FILE_NOT_FOUND:
        TrialCardSavePage = TRIAL_CARD_PAGE_FILE_NOT_FOUND;
        break;
    case SAVE_FILE_NOT_FOUND_ACKNOWLEDGED:
        TrialCardSavePage = 0;
        TrialCardSaveState = SAVE_ABANDON_PROMPT;
        break;
    case SAVE_FINISHED:
        TrialCardSavePage = CARD_PAGE_WRITE_COMPLETE;
        break;
    case SAVE_NEW_CANCELLED:
    case SAVE_AS_NEW_CANCELLED:
    case SAVE_FULL_EXIT:
    case SAVE_DAMAGED_EXIT:
    case SAVE_MISSING_EXIT:
    case SAVE_FINISHED_ACKNOWLEDGED:
        TrialCardSaveState = SAVE_COMPLETE;
        break;
    case SAVE_FAILED:
        TrialCardSavePage = TRIAL_CARD_PAGE_RETRY_SAVE;
        break;
    case SAVE_ABANDON_PROMPT:
        TrialCardSavePage = CARD_PAGE_CANNOT_SAVE_PROMPT;
        break;
    case SAVE_FAILURE_CANCELLED:
    case SAVE_ABANDON_ACCEPTED:
        TrialCardSaveState = -1;
        break;
    case SAVE_FULL_RETRY:
    case SAVE_DAMAGED_RETRY:
    case SAVE_MISSING_RETRY:
    case SAVE_FAILURE_RETRY:
    case SAVE_ABANDON_CANCELLED:
        TrialCardSavePage = 0;
        TrialCardSaveState = 0;
        break;
    case SAVE_CREATE_GAME:
        TrialCardOverwriteGame = 0;
        TrialCardSaveState = SAVE_WRITE_GAME;
        TrialCardSaveRetry = 0;
        break;
    case SAVE_OVERWRITE_GAME:
        TrialCardOverwriteGame = 1;
        TrialCardSaveRetry = 0;
    case SAVE_WRITE_GAME:
        sprintf(filename, TrialCardGameFilename);
        switch (SaveCard(0, (u8 *)filename, TRIAL_PERSISTENT_STATE,
            sizeof(TrialPersistentState), TrialCardOverwriteGame))
        {
        case CARD_RESULT_SUCCESS:
            TrialCardSaveState = SAVE_FINISHED;
            break;
        case CARD_RESULT_FILE_EXISTS:
            TrialCardSaveState = SAVE_OVERWRITE_GAME;
            break;
        case CARD_RESULT_FULL:
            TrialCardSaveState = SAVE_FULL_PROMPT;
            break;
        default:
            TrialCardSaveState = SAVE_FAILED;
            break;
        }
        if (TrialCardSaveState == SAVE_FAILED)
            TrialCardSaveState = TrialCardSaveRetry++ < 3
                ? SAVE_WRITE_GAME : TrialCardSaveState;
        break;
    default:
        TrialCardSavePage = 0;
        TrialCardSaveRetry = 0;
        setup_card_screen_(CARD_SCREEN_RESOURCES_RELEASE);
        if (TrialCardSaveState < 0)
        {
            TrialCardSaveState = 0;
            return 1;
        }
        TrialCardSaveState = 0;
        return -1;
    }
    TrialCardSaveState += draw_card_help_(TrialCardSavePage, pressed);
    return 0;
}
