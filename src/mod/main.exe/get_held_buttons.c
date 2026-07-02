#include "common.h"
#include "main.exe.h"

/*
 * DEMO of a non-matching / grown mod (see docs/modding-and-nonmatching.md).
 *
 * This is a deliberately *bigger* version of get_held_buttons (an extra call) —
 * the kind of edit that would shift the fixed layout and break a normal build.
 * `./Build mod` instead relocates it into the mod region and trampolines the
 * original slot, so the rest of the binary stays byte-identical.
 *
 * Replace the body with whatever you're modding / debugging.
 */
buttons_held get_held_buttons(s32 arg0)
{
    FUN_8001ada4();
    FUN_8001ada4();
    return HELD_BUTTONS[arg0 >> 4][arg0 & 3].held;
}
