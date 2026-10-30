#include "common.h"
#include "mission_menu.h"
#include "card.h"

enum trial_card_delete_phase
{
    CARD_DELETE_PROMPT = 0,
    CARD_DELETE_ACCEPTED,
    CARD_DELETE_CANCELLED,
    CARD_DELETE_CONFIRM = 4,
    CARD_DELETE_CONFIRMED,
    CARD_DELETE_CONFIRM_CANCELLED,
    CARD_DELETE_BEGIN = 10,
    CARD_DELETE_WAIT_1,
    CARD_DELETE_WAIT_2,
    CARD_DELETE_FILE,
    CARD_DELETE_FINISH_WAIT = 20,
    CARD_DELETE_FINISHED,
    CARD_DELETE_ACKNOWLEDGED,
    CARD_DELETE_FAILED = 25,
    CARD_DELETE_FAILURE_ACKNOWLEDGED,
    CARD_DELETE_COMPLETE = 99
};

/* Require both confirmation pages before deleting the selected Mxxxxxxx file.
 * The completion message is delayed by thirty update calls. */
short UpdateTrialCardDelete(u_long pressed, const TrialMissionHeader *header)
{
    char filename[128];

    setup_card_screen_(CARD_SCREEN_RESOURCES_ACQUIRE);
    switch (TrialCardDeleteState)
    {
    case CARD_DELETE_PROMPT:
        TrialCardDeletePage = TRIAL_CARD_PAGE_DELETE_PROMPT;
        break;
    case CARD_DELETE_ACCEPTED:
        TrialCardDeleteState = CARD_DELETE_CONFIRM;
        break;
    case CARD_DELETE_CONFIRM:
        TrialCardDeletePage = TRIAL_CARD_PAGE_CONFIRM_DELETE;
        break;
    case CARD_DELETE_CONFIRMED:
        TrialCardDeleteState = CARD_DELETE_BEGIN;
        break;
    case CARD_DELETE_BEGIN:
        TrialCardDeletePage = TRIAL_CARD_PAGE_DELETING;
        /* Let the message reach the display before synchronous card I/O. */
    case CARD_DELETE_WAIT_1:
    case CARD_DELETE_WAIT_2:
        TrialCardDeleteState++;
        break;
    case CARD_DELETE_FILE:
        sprintf(filename, fmt_trial_card_mission_name, header->checksum);
        if (DeleteCard((u8 *)filename) == CARD_RESULT_SUCCESS)
            TrialCardDeleteState = CARD_DELETE_FINISH_WAIT;
        else
            TrialCardDeleteState = CARD_DELETE_FAILED;
        break;
    case CARD_DELETE_FINISH_WAIT:
        if (TrialCardDeleteWait++ < 30)
            break;
        TrialCardDeleteWait = 0;
        TrialCardDeleteState++;
        break;
    case CARD_DELETE_FINISHED:
        TrialCardDeletePage = TRIAL_CARD_PAGE_DELETED;
        break;
    case CARD_DELETE_CANCELLED:
    case CARD_DELETE_CONFIRM_CANCELLED:
    case CARD_DELETE_ACKNOWLEDGED:
        TrialCardDeleteState = CARD_DELETE_COMPLETE;
        break;
    case CARD_DELETE_FAILED:
        TrialCardDeletePage = TRIAL_CARD_PAGE_DELETE_FAILED;
        break;
    case CARD_DELETE_FAILURE_ACKNOWLEDGED:
        TrialCardDeleteState = -1;
        break;
    default:
        TrialCardDeletePage = 0;
        setup_card_screen_(CARD_SCREEN_RESOURCES_RELEASE);
        if (TrialCardDeleteState < 0)
        {
            TrialCardDeleteState = 0;
            return 1;
        }
        TrialCardDeleteState = 0;
        return -1;
    }
    TrialCardDeleteState += draw_card_help_(TrialCardDeletePage, pressed);
    return 0;
}
