#include "common.h"
#include "menu.exe.h"
#include "cardmenu.h"
#include "persistent.h"
#include "../main.exe/adt.h"

extern char msg_save_version_mismatch[];

short UpdateStartupCardCheck(int buttons)
{
    short result;

    setup_card_screen_(CARD_SCREEN_RESOURCES_ACQUIRE);
    switch (StartupCardState)
    {
    case CARD_STATE_FORMAT_PROMPT:
        StartupCardState = STARTUP_CARD_FINISHED;
        break;
    case CARD_STATE_CARD_READY:
        result = check_card_file_(McardFile);
        switch (result)
        {
        case CARD_RESULT_SUCCESS:
            StartupCardState = STARTUP_CARD_BEGIN_LOAD;
            break;
        case CARD_RESULT_FILE_NOT_FOUND:
            StartupCardState = STARTUP_CARD_NO_SAVE;
            StartupCardPage = CARD_PAGE_NONE;
            StartupCardWaitFrames = 60;
            break;
        default:
            StartupCardState = CARD_STATE_SHOW_CHECKING;
            break;
        }
        break;
    case STARTUP_CARD_BEGIN_LOAD:
        StartupCardPage = MENU_CARD_PAGE_READING;
        goto advance_state;
    case STARTUP_CARD_LOAD:
        if (LoadCard(0, (u8 *)McardFile) == CARD_RESULT_SUCCESS)
            StartupCardState = STARTUP_CARD_VALIDATE_SAVE;
        else
            StartupCardState = STARTUP_CARD_LOAD_FAILED;
        break;
    case STARTUP_CARD_LOAD_FAILED:
        StartupCardPage = MENU_CARD_PAGE_READ_FAILED;
        break;
    case STARTUP_CARD_VALIDATE_SAVE:
        if (MENU_STATE->magic != MENU_STATE_MAGIC)
        {
            AdtMessageBox(msg_save_version_mismatch, MENU_STATE->magic);
            InitMenuPersistentState();
        }
        StartupCardState++;
        break;
    case STARTUP_CARD_CHECK_SPACE:
        /* Create a temporary file to test whether a complete save fits. */
        result = SaveCard(0, (u8 *)McardFile, MENU_STATE,
                          TENCHU_PERSISTENT_STATE_SIZE, 0);
        switch (result)
        {
        case CARD_RESULT_SUCCESS:
            StartupCardState = STARTUP_CARD_DELETE_PROBE;
            break;
        case CARD_RESULT_FULL:
            StartupCardState = STARTUP_CARD_FULL;
            break;
        default:
            StartupCardState = CARD_STATE_SHOW_CHECKING;
            break;
        }
        break;
    case STARTUP_CARD_FULL:
        StartupCardPage = MENU_CARD_PAGE_FULL_WARNING;
        break;
    case STARTUP_CARD_EXIT_FULL:
        StartupCardState = CARD_STATE_EXIT;
        break;
    case STARTUP_CARD_DELETE_PROBE:
        if (DeleteCard((u8 *)McardFile) == CARD_RESULT_SUCCESS)
            StartupCardState = STARTUP_CARD_PROBE_DELETED;
        else
            StartupCardState = CARD_STATE_SHOW_CHECKING;
        break;
    case STARTUP_CARD_RETRY_LOAD:
        StartupCardState = CARD_STATE_SHOW_CHECKING;
        break;
    case STARTUP_CARD_LOAD_WAIT_1:
    case STARTUP_CARD_LOAD_WAIT_2:
    case STARTUP_CARD_PROBE_DELETED:
    advance_state:
        StartupCardState++;
        break;
    default:
        result = update_card_message_(&StartupCardState, (card_page *)&StartupCardPage);
        if (result == 0)
        {
            StartupCardPage = CARD_PAGE_NONE;
            StartupCardRetry = 0;
            setup_card_screen_(CARD_SCREEN_RESOURCES_RELEASE);
            MENU_STATE->GameRetry =
                (MENU_STATE->GameRetry & MENU_STATE_RETAIN_ON_LOAD) | MENU_STATE_VISITED;
            if (StartupCardState < 0)
            {
                StartupCardState = CARD_STATE_PREPARE_CHECK;
                return 1;
            }
            StartupCardState = CARD_STATE_PREPARE_CHECK;
            return -1;
        }
        break;
    }
    StartupCardState += draw_card_help_(StartupCardPage, buttons);
    return 0;
}

short UpdateLoadCardScreen(int buttons)
{
    short result;

    setup_card_screen_(CARD_SCREEN_RESOURCES_ACQUIRE);
    switch (LoadCardState)
    {
    case CARD_STATE_NO_CARD:
        LoadCardPage = MENU_CARD_PAGE_NO_CARD;
        break;
    case CARD_STATE_FORMAT_PROMPT:
        LoadCardPage = MENU_CARD_PAGE_UNFORMATTED;
        break;
    case LOAD_CARD_ACK_UNFORMATTED:
        LoadCardState = LOAD_CARD_CANNOT_LOAD;
        break;
    case CARD_STATE_CARD_READY:
        LoadCardPage = MENU_CARD_PAGE_READING;
        /* fallthrough */
    case LOAD_CARD_WAIT_1:
    case LOAD_CARD_WAIT_2:
        LoadCardState++;
        break;
    case LOAD_CARD_READ:
        result = LoadCard(0, (u8 *)McardFile);
        switch (result)
        {
        case CARD_RESULT_SUCCESS:
            LoadCardState = LOAD_CARD_COMPLETE;
            break;
        case CARD_RESULT_FILE_NOT_FOUND:
            LoadCardState = LOAD_CARD_NO_SAVE;
            break;
        default:
            LoadCardState = LOAD_CARD_FAILED;
            break;
        }
        break;
    case LOAD_CARD_FAILED:
        LoadCardPage = MENU_CARD_PAGE_READ_FAILED;
        break;
    case LOAD_CARD_COMPLETE:
        if (MENU_STATE->magic != MENU_STATE_MAGIC)
        {
            AdtMessageBox(msg_save_version_mismatch, MENU_STATE->magic);
            InitMenuPersistentState();
        }
        LoadCardPage = MENU_CARD_PAGE_READ_COMPLETE;
        break;
    case LOAD_CARD_NO_SAVE:
        LoadCardPage = MENU_CARD_PAGE_NO_SAVE;
        break;
    case LOAD_CARD_CANNOT_LOAD:
        LoadCardPage = MENU_CARD_PAGE_CANNOT_LOAD;
        break;
    case LOAD_CARD_EXIT_NO_SAVE:
    case LOAD_CARD_EXIT_CANNOT_LOAD:
        LoadCardState = CARD_STATE_EXIT;
        break;
    case LOAD_CARD_RETRY:
    case LOAD_CARD_RESTART_CHECK:
        LoadCardState = CARD_STATE_SHOW_CHECKING;
        break;
    default:
        result = update_card_message_(&LoadCardState, (card_page *)&LoadCardPage);
        if (result == 0)
        {
            LoadCardPage = CARD_PAGE_NONE;
            setup_card_screen_(CARD_SCREEN_RESOURCES_RELEASE);
            MENU_STATE->GameRetry =
                (MENU_STATE->GameRetry & MENU_STATE_RETAIN_ON_LOAD) | MENU_STATE_VISITED;
            if (LoadCardState < 0)
            {
                LoadCardState = CARD_STATE_PREPARE_CHECK;
                return 1;
            }
            LoadCardState = CARD_STATE_PREPARE_CHECK;
            return -1;
        }
        break;
    }
    LoadCardState += draw_card_help_(LoadCardPage, buttons);
    return 0;
}

/* MENU string storage. Keep the retail spelling and development paths. */
char path_demo_start_card_j[] = "K:\\WORK\\CDIMAGE\\DEMO\\start\\card_j.txt";
char path_demo_start_mcard_tim[] = "K:\\WORK\\CDIMAGE\\DEMO\\start\\mcard.tim";
char path_demo_start_mbuttonj_tim[] = "K:\\WORK\\CDIMAGE\\DEMO\\start\\mbuttonj.tim";
char path_demo_start_xtoselj_tim[] = "K:\\WORK\\CDIMAGE\\DEMO\\start\\xtoselj.tim";
