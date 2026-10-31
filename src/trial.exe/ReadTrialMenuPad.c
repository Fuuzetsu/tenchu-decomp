#include "common.h"
#include "trial.exe.h"

/* Keep the held-button snapshot unchanged while polling is inhibited. */
u_long ReadTrialMenuPad(short port, u_long *previous_buttons, short inhibit)
{
    u_long buttons = *previous_buttons;
    u_long current = buttons;
    u_long changed;

    if (!inhibit)
        current = GetRealPad(port);
    changed = buttons ^ current;
    *previous_buttons = current;
    buttons = changed & current;
    return buttons;
}
