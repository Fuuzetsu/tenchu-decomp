#include "common.h"
#include "main.exe.h"
#include "memcard.h"

s16 update_card_message_(card_state *state, card_page *message)
{
    s32 cmd;
    enum card_result result;
    card_result card_status;
    card_state next_state;
    card_page next_message;

    next_state = *state;
    next_message = *message;

    switch (next_state)
    {
    case CARD_STATE_SHOW_CHECKING:
        next_message = CARD_PAGE_CHECKING;
        goto increment_state;

    case CARD_STATE_PREPARE_CHECK:
        McardRetryCount = 0;
        next_state = CARD_STATE_CHECK;
        break;

    case CARD_STATE_CHECK:
        /* Retail keeps separate CARD_STATE_CARD_READY stores in both dispatch stages. */
        card_status = ChkCard();
        if (card_status == CARD_RESULT_DAMAGED)
        {
            goto card_damaged;
        }
        if (card_status >= CARD_RESULT_NEW_CARD)
        {
            goto card_status_ge_three;
        }
        switch (card_status)
        {
        default:
            next_state = CARD_STATE_CARD_READY;
            break;
        case CARD_RESULT_NO_CARD:
            goto card_missing;
        }
        goto retry_card_check;

    card_status_ge_three:
        switch (card_status)
        {
        default:
            next_state = CARD_STATE_CARD_READY;
            break;
        case CARD_RESULT_UNFORMATTED:
            goto card_unformatted;
        }
        goto retry_card_check;

    card_missing:
        next_state = CARD_STATE_NO_CARD;
        goto retry_card_check;
    card_damaged:
        next_state = CARD_STATE_DAMAGED;
        goto retry_card_check;
    card_unformatted:
        McardStateFlag = 0;
        next_state = CARD_STATE_FORMAT_PROMPT;

    retry_card_check:
        if (next_state != CARD_STATE_CARD_READY &&
            McardRetryCount++ < CARD_RETRY_LIMIT)
        {
            next_state = CARD_STATE_CHECK;
        }
        break;

    case CARD_STATE_NO_CARD:
        next_message = CARD_PAGE_NO_CARD_SAVE_WARNING;
        break;

    case CARD_STATE_DAMAGED:
        next_message = CARD_PAGE_DAMAGED;
        break;

    case CARD_STATE_FORMAT_PROMPT:
        result = MemCardExist(MEMCARD_CHANNEL_0);
        MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
        next_message = CARD_PAGE_FORMAT_PROMPT;
        if (result == CARD_RESULT_SUCCESS)
        {
            break;
        }
        next_message = CARD_PAGE_NONE;
        /* fallthrough */
    case CARD_STATE_RESTART_FORMAT_FAILED:
    case CARD_STATE_RESTART_SAVE:
        next_state = CARD_STATE_SHOW_CHECKING;
        break;

    case CARD_STATE_BEGIN_FORMAT:
        next_message = CARD_PAGE_FORMATTING;
        McardRetryCount = 0;
        next_state = CARD_STATE_FORMAT_WAIT_1;
        break;

    case CARD_STATE_CANCEL_FORMAT:
        next_state = CARD_STATE_CANNOT_SAVE_PROMPT;
        break;

    case CARD_STATE_CHECK_WAIT_1:
    case CARD_STATE_CHECK_WAIT_2:
    case CARD_STATE_FORMAT_WAIT_1:
    case CARD_STATE_FORMAT_WAIT_2:
        increment_state:
            next_state++;
            break;

    case CARD_STATE_FORMAT:
        next_state = CARD_STATE_FORMAT_FAILED;
        card_status = FormatCard();
        if (card_status == CARD_RESULT_SUCCESS)
        {
            next_state = CARD_STATE_FORMAT_COMPLETE;
        }
        if (next_state != CARD_STATE_FORMAT_COMPLETE &&
            McardRetryCount++ < CARD_RETRY_LIMIT)
        {
            next_state = CARD_STATE_FORMAT;
        }
        break;

    case CARD_STATE_FORMAT_FAILED:
        next_message = CARD_PAGE_FORMAT_FAILED;
        break;

    case CARD_STATE_FORMAT_COMPLETE:
        next_message = CARD_PAGE_FORMAT_COMPLETE;
        break;

    case CARD_STATE_CANNOT_SAVE_PROMPT:
        next_message = CARD_PAGE_CANNOT_SAVE_PROMPT;
        break;

    case CARD_STATE_EXIT_NO_CARD:
    case CARD_STATE_EXIT_DAMAGED:
    case CARD_STATE_EXIT_WITHOUT_SAVE:
        next_state = CARD_STATE_EXIT;
        break;

    default:
        return 0;
    }

    *state = next_state;
    *message = next_message;
    return -1;
}
