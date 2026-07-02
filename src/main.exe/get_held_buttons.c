#include "common.h"
#include "main.exe.h"

/*
 * Returns the held-buttons word for controller [arg0 >> 4][arg0 & 3].
 *
 * The pointer temporary and the do/while(0) are not cosmetic: they are what
 * makes GCC 2.8.1 pick the exact register allocation / instruction schedule of
 * the original (found with decomp-permuter). Writing the natural
 * `return HELD_BUTTONS[arg0 >> 4][arg0 & 3].held;` computes the indices in the
 * other order and no longer byte-matches. See docs/matching-get-held-buttons.md.
 */
buttons_held get_held_buttons(s32 arg0)
{
    buttons_held *held;
    FUN_8001ada4();
    do {
        held = &HELD_BUTTONS[arg0 >> 4][arg0 & 3].held;
        return *held;
    } while (0);
}
