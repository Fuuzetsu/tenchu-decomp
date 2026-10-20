#ifndef MENU_CARDMENU_H
#define MENU_CARDMENU_H

#include "../main.exe/memcard.h"

/* Both menus share the common card-check states, then use their own
 * load/acknowledgement sequences. */
enum
{
    STARTUP_CARD_NO_SAVE = 5,
    STARTUP_CARD_BEGIN_LOAD = 50,
    STARTUP_CARD_LOAD_WAIT_1,
    STARTUP_CARD_LOAD_WAIT_2,
    STARTUP_CARD_LOAD,
    STARTUP_CARD_LOAD_FAILED = 55,
    STARTUP_CARD_RETRY_LOAD,
    STARTUP_CARD_VALIDATE_SAVE,
    STARTUP_CARD_CHECK_SPACE = 60,
    STARTUP_CARD_FULL,
    STARTUP_CARD_EXIT_FULL,
    STARTUP_CARD_DELETE_PROBE,
    STARTUP_CARD_PROBE_DELETED,
    STARTUP_CARD_FINISHED = 99
};

enum
{
    LOAD_CARD_ACK_UNFORMATTED = 31,
    LOAD_CARD_WAIT_1 = 41,
    LOAD_CARD_WAIT_2,
    LOAD_CARD_READ,
    LOAD_CARD_FAILED,
    LOAD_CARD_RETRY,
    LOAD_CARD_COMPLETE,
    LOAD_CARD_NO_SAVE = 50,
    LOAD_CARD_EXIT_NO_SAVE,
    LOAD_CARD_CANNOT_LOAD = 90,
    LOAD_CARD_EXIT_CANNOT_LOAD,
    LOAD_CARD_RESTART_CHECK
};

/* Page numbers verified against START/CARD_J.TXT in the retail volume. */
enum
{
    MENU_CARD_PAGE_NO_CARD = 1,
    MENU_CARD_PAGE_READING = 6,
    MENU_CARD_PAGE_READ_COMPLETE = 8,
    MENU_CARD_PAGE_READ_FAILED = 13,
    MENU_CARD_PAGE_UNFORMATTED = 17,
    MENU_CARD_PAGE_FULL_WARNING = 19,
    MENU_CARD_PAGE_CANNOT_LOAD = 39,
    MENU_CARD_PAGE_NO_SAVE = 41
};

extern card_state StartupCardState;
extern short StartupCardPage;
extern short StartupCardRetry; /* Cleared on exit; no other MENU reader. */
extern short StartupCardWaitFrames; /* Set to 60 when no save exists; unused. */
extern card_state LoadCardState;
extern short LoadCardPage;

short UpdateStartupCardCheck(int buttons);
short UpdateLoadCardScreen(int buttons);

#endif
