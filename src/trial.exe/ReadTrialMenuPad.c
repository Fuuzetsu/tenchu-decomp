#include "common.h"
#include "trial.exe.h"

/* STATUS: NON_MATCHING. Both bodies are 84 bytes. The C draft loads the
 * initial snapshot into v1 and copies it to s0; retail uses the reverse pair.
 * Four instruction bytes differ. Keep the original routine in the exact build. */
#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/trial.exe/asm/nonmatchings/ReadTrialMenuPad", ReadTrialMenuPad);
#else
/* Keep the held-button snapshot unchanged while polling is inhibited. */
u_long ReadTrialMenuPad(short port, u_long *previous_buttons, short inhibit)
{
    u_long previous = *previous_buttons;
    u_long current = previous;

    if (!inhibit)
        current = GetRealPad(port);
    *previous_buttons = current;
    return (previous ^ current) & current;
}
#endif
