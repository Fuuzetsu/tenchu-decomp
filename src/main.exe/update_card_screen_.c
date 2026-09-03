#include "common.h"
#include "main.exe.h"
#include "memcard.h"

extern char *McardFile;
extern s16 McardStateFlag;
extern card_state McardState;
extern s16 McardPage;
extern s16 McardRetry;

s16 update_card_screen_(s32 pad)
{
    u16 saved_state;
    s16 value;
    s32 cond;
    u16 next_state;
    u16 save_result_state;
    u16 incremented;

    setup_card_screen_(CARD_SCREEN_RESOURCES_ACQUIRE);
    switch (McardState)
    {
    case CARD_STATE_NO_CARD:
        McardPage = CARD_PAGE_NO_CARD_CANNOT_SAVE_PROMPT;
        break;
    case CARD_STATE_DAMAGED:
        McardPage = CARD_PAGE_DAMAGED_CANNOT_SAVE_PROMPT;
        break;
    case CARD_STATE_FORMAT_COMPLETE:
        McardState = CARD_STATE_CARD_READY;
        break;
    case CARD_STATE_CARD_READY:
        McardState = CARD_STATE_CHECK_SAVE_FILE;
        break;
    case CARD_STATE_CHECK_SAVE_FILE:
        value = check_card_file_(McardFile);
        switch (value)
        {
        default:
            goto restart_card_check;
        case CARD_RESULT_SUCCESS:
            McardState = CARD_STATE_OVERWRITE_GAME_DATA_PROMPT;
            break;
        case CARD_RESULT_FILE_NOT_FOUND:
            McardState = CARD_STATE_BEGIN_NEW_SAVE;
            break;
            }
            break;
        case CARD_STATE_BEGIN_NEW_SAVE:
            McardPage = CARD_PAGE_WRITING;
            McardRetry = 0;
            goto increment_state;
        case CARD_STATE_WRITE_NEW_SAVE:
            SaveCard(0, (u8 *)McardFile,
                     (void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                     TENCHU_PERSISTENT_STATE_SIZE, 0);
            value = SaveCard(0, (u8 *)McardFile,
                             (void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                             TENCHU_PERSISTENT_STATE_SIZE, 1);
            switch (value)
            {
        default:
            save_result_state = CARD_STATE_WRITE_FAILED;
            break;
        case CARD_RESULT_SUCCESS:
            save_result_state = CARD_STATE_WRITE_COMPLETE;
            break;
        case CARD_RESULT_NO_CARD:
            save_result_state = CARD_STATE_NO_CARD;
            break;
        case CARD_RESULT_FULL:
            save_result_state = CARD_STATE_NOT_ENOUGH_SPACE_PROMPT;
            break;
        case CARD_RESULT_UNFORMATTED:
            McardState = CARD_STATE_FORMAT_PROMPT;
            McardStateFlag = 0;
            goto retry_new_save;
        }
        McardState = save_result_state;
    retry_new_save:
        if (McardState == CARD_STATE_WRITE_COMPLETE)
            break;
        next_state = CARD_STATE_WRITE_NEW_SAVE;
        saved_state = (u16)McardState;
        value = McardRetry;
        incremented = value + 1;
        goto update_write_retry;
    case CARD_STATE_WRITE_COMPLETE:
        if (gfMemory == 0)
        {
            McardPage = CARD_PAGE_WRITE_COMPLETE;
            break;
        }
    case CARD_STATE_FINISH_SAVE:
        McardState = CARD_STATE_SAVE_COMPLETE_EXIT;
        break;
    case CARD_STATE_WRITE_FAILED:
        McardPage = CARD_PAGE_WRITE_FAILED;
        break;
    case CARD_STATE_OVERWRITE_GAME_DATA_PROMPT:
        McardPage = CARD_PAGE_OVERWRITE_GAME_DATA_PROMPT;
        break;
    case CARD_STATE_BEGIN_OVERWRITE:
        McardPage = CARD_PAGE_WRITING;
        McardState = CARD_STATE_OVERWRITE_WAIT_1;
        McardRetry = 0;
        break;
    case CARD_STATE_WRITE_FAILURE_ACKNOWLEDGED:
    case CARD_STATE_CANCEL_OVERWRITE:
        McardState = CARD_STATE_CANNOT_SAVE_PROMPT;
        break;
    case CARD_STATE_FILE_CHECK_WAIT_1:
    case CARD_STATE_FILE_CHECK_WAIT_2:
    case CARD_STATE_NEW_SAVE_WAIT_1:
    case CARD_STATE_NEW_SAVE_WAIT_2:
    case CARD_STATE_OVERWRITE_WAIT_1:
    case CARD_STATE_OVERWRITE_WAIT_2:
        increment_state:
            McardState++;
            break;
    case CARD_STATE_WRITE_OVERWRITE:
        SaveCard(0, (u8 *)McardFile,
                 (void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                 TENCHU_PERSISTENT_STATE_SIZE, 0);
        value = SaveCard(0, (u8 *)McardFile,
                         (void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                         TENCHU_PERSISTENT_STATE_SIZE, 1);
        switch (value)
        {
        default:
            save_result_state = CARD_STATE_WRITE_FAILED;
            break;
        case CARD_RESULT_SUCCESS:
            save_result_state = CARD_STATE_WRITE_COMPLETE;
            break;
        case CARD_RESULT_NO_CARD:
            save_result_state = CARD_STATE_NO_CARD;
            break;
        case CARD_RESULT_FULL:
            save_result_state = CARD_STATE_NOT_ENOUGH_SPACE_PROMPT;
            break;
        case CARD_RESULT_UNFORMATTED:
            McardState = CARD_STATE_FORMAT_PROMPT;
            McardStateFlag = 0;
            goto retry_overwrite;
        }
        McardState = save_result_state;
    retry_overwrite:
        if (McardState == CARD_STATE_WRITE_COMPLETE)
            break;
        next_state = CARD_STATE_WRITE_OVERWRITE;
        saved_state = (u16)McardState;
        value = McardRetry;
        incremented = value + 1;
    update_write_retry:
        cond = value < CARD_RETRY_LIMIT;
        do
        {
            McardRetry = incremented;
        } while (0);
        if (!cond)
            next_state = saved_state;
        do
        {
            McardState = next_state;
        } while (0);
        break;
    case CARD_STATE_NOT_ENOUGH_SPACE_PROMPT:
        McardPage = CARD_PAGE_NOT_ENOUGH_SPACE_CANNOT_SAVE_PROMPT;
        break;
    case CARD_STATE_EXIT_NO_CARD:
    case CARD_STATE_EXIT_DAMAGED:
    case CARD_STATE_EXIT_NOT_ENOUGH_SPACE:
        McardState = CARD_STATE_EXIT;
        break;
    case CARD_STATE_RESTART_NO_CARD:
    case CARD_STATE_RESTART_DAMAGED:
    case CARD_STATE_RESTART_NOT_ENOUGH_SPACE:
        restart_card_check:
            McardState = CARD_STATE_SHOW_CHECKING;
            break;
    default:
        value = update_card_message_(&McardState, (card_page *)&McardPage);
        if (value == 0)
        {
            McardPage = CARD_PAGE_NONE;
            McardRetry = 0;
            setup_card_screen_(CARD_SCREEN_RESOURCES_RELEASE);
            if (McardState < CARD_STATE_SHOW_CHECKING)
            {
                McardState = CARD_STATE_PREPARE_CHECK;
                return 1;
            }
            McardState = CARD_STATE_PREPARE_CHECK;
            return -1;
        }
        break;
    }

    McardState += draw_card_help_(McardPage, pad);
    return 0;
}
