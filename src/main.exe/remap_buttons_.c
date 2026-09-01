#include "common.h"
#include "main.exe.h"
#include "padcmd.h"

/*
 * MATCHED: remap_buttons_ (0x8001b2f4, 112 bytes) applies the selected
 * eight-button ControlScheme row. For each raw row-zero bit it sets or clears
 * the corresponding selected-row bit, preserving all other bits.
 *
 * Matching constraints:
 *  - pad is the signed-16 test source; acc is a separate u16 accumulator and
 *    is cast back through s16 for the function's wide return.
 *  - The clear mask is a raw u8 table load complemented after integer
 *    promotion, producing nor/and rather than a narrowed immediate mask.
 *  - i and row are distinct loop-carried counters. The do/while starts at
 *    i == 0 and advances both through one control-scheme row.
 *  - Keep test as an explicit pseudo. It is a pad-bit result, so using it as
 *    the loop bound would be semantically wrong even if a diff score improved.
 *  - rp is a named pointer with a defined dead initializer, then an assignment
 *    at the start of each branch. The mutually exclusive definitions let it
 *    take $v0; delay-slot reorg merges the identical address calculation into
 *    the condition branch's delay slot. Defining it once before the branch
 *    overlaps the condition and cascades the remaining allocations.
 *  - Do not inline ButtonAssign[row] in both arms. Two textual indexed uses
 *    cross loop.c's strength-reduction threshold and turn row into a byte
 *    pointer, making the function one instruction longer. The named pointer
 *    keeps row as the target's integer counter.
 */
extern u8 ButtonAssign[N_BUTTON_ASSIGNMENTS];
/* s16 here vs main.c's u16 is byte-required: this TU's read is a
 * signed lh (measured — the u16 form flips it to lhu). */
extern s16 ControlScheme;

s32 remap_buttons_(s16 pad)
{
    s32 i;
    s32 row;
    u16 acc;
    s32 test;
    u8 *rp;

    rp = ButtonAssign;
    acc = pad;
    row = (s32)ControlScheme * BUTTONS_PER_CONTROL_SCHEME;
    i = 0;
    do
    {
        test = pad & ButtonAssign[i];
        if (test != 0)
        {
            rp = &ButtonAssign[row];
            acc = acc | *rp;
        }
        else
        {
            rp = &ButtonAssign[row];
            acc = acc & ~*rp;
        }
        i++;
        row++;
    } while (i < BUTTONS_PER_CONTROL_SCHEME);
    return (s16)acc;
}
