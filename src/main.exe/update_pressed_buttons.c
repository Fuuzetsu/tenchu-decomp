#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * Maintains one controller's current/previous/new input words and the recent
 * non-zero input history. `frames_since_new_input` is signed deliberately:
 * the increment-and-negative test is the original input-disable gate.
 * Reading `currently_pressed` back in the change test is load-bearing: it
 * keeps the field store ahead of the previous/new-input updates and matches
 * the target's store/reload schedule.
 */
s16 update_pressed_buttons(PADtype *buttons, u16 pressed)
{
    s16 i;
    u16 previously_pressed;

    if (buttons->time < 6 && ++buttons->time < 0) {
        pressed = 0;
    }

    previously_pressed = buttons->data;
    buttons->data = pressed;
    buttons->sdata = previously_pressed;
    buttons->trig = pressed & ~previously_pressed;

    if (buttons->sdata != buttons->data &&
        buttons->data != 0) {
        if (buttons->time > 5) {
            buttons->stream[0] = 0;
        }

        for (i = 3; i > 0; i--) {
            buttons->stream[i] =
                buttons->stream[i - 1];
        }

        buttons->time = 0;
        buttons->stream[0] = buttons->data;
    }

    return pressed;
}
