#include "common.h"
#include "main.exe.h"
#include "padcmd.h"

/*
 * MATCHED: remap_buttons_ (0x8001b2f4, 112 bytes) applies the selected
 * eight-button ControlScheme run. For each canonical-layout bit it sets or
 * clears the corresponding selected-layout bit, preserving all other bits.
 *
 * Matching constraints:
 *  - pad is the signed-16 test source; acc is a separate u16 accumulator and
 *    is cast back through s16 for the function's wide return.
 *  - The clear mask is a raw u8 table load complemented after integer
 *    promotion, producing nor/and rather than a narrowed immediate mask.
 *  - i and selected_index are distinct loop-carried counters. The do/while
 *    starts at i == 0 and advances both through one control-scheme run.
 *  - Keep test as an explicit pseudo. It is a pad-bit result, so using it as
 *    the loop bound would be semantically wrong even if a diff score improved.
 *  - mapped_button starts at the table base, then is assigned at the start of
 *    each branch. The mutually exclusive definitions let it take $v0;
 *    delay-slot reorg merges the identical address calculation into the
 *    condition branch's delay slot. Defining it once before the branch
 *    overlaps the condition and cascades the remaining allocations.
 *  - Do not inline ButtonAssign[selected_index] in both arms. Two textual
 *    indexed uses cross loop.c's strength-reduction threshold and turn
 *    selected_index into a byte pointer, making the function one instruction
 *    longer. The named pointer keeps it as the target's integer counter.
 */
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
