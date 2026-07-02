#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/get_held_buttons", get_held_buttons);

/*
 * WIP: arithmetic-perfect, but the instruction scheduling / register allocation
 * does not yet byte-match. See docs/case-study-get-held-buttons.md.
 *
 * The body below is functionally correct (confirmed vs m2c) and produces the
 * right MIPS arithmetic (the *14 / *56 strength-reduced multiplies, the lhu),
 * but gcc 2.8.1 schedules the two index terms in the opposite order and picks
 * different registers than the target. The target computes the OUTER index
 * (arg0 >> 4) first because it reuses $s0 for the inner index, and materialises
 * the full &HELD_BUTTONS address (lui+addiu) grouped with the inner term. No
 * source rewrite reproduced that exact allocation across ~25 variants — this is
 * a decomp-permuter job, not a maspsx one (there is no div / gp-rel / nop here).
 *
 * buttons_held get_held_buttons(s32 arg0)
 * {
 *     FUN_8001ada4();
 *     return HELD_BUTTONS[arg0 >> 4][arg0 & 3].held;
 * }
 */
