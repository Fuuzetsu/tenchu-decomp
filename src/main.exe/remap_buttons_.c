#include "common.h"
#include "main.exe.h"
#include "padcmd.h"

s32 remap_buttons_(s16 pad)
{
    s32 i;
    s32 selected_index;
    u16 acc;
    s32 test;
    u8 *mapped_button;

    mapped_button = ButtonAssign;
    acc = pad;
    selected_index = (s32)ControlScheme * BUTTONS_PER_CONTROL_SCHEME;
    i = 0;
    do
    {
        test = pad & ButtonAssign[i];
        if (test != 0)
        {
            mapped_button = &ButtonAssign[selected_index];
            acc = acc | *mapped_button;
        }
        else
        {
            mapped_button = &ButtonAssign[selected_index];
            acc = acc & ~*mapped_button;
        }
        i++;
        selected_index++;
    } while (i < BUTTONS_PER_CONTROL_SCHEME);
    return (s16)acc;
}
