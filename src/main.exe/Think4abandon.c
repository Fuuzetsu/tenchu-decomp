#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think4abandon(void);
 *     THINK_4.C:14, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $v1       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Attrib;
 *     extern short SR;
 * END PSX.SYM */

/*
 * Think4abandon (0x8002f254, 0x1D4 bytes) — think-level-4 "give up the
 * chase" handler, same THINK_4.C TU as Think4contact/Think4chase (both
 * parked). Clears the chase target (some_other_x/z_position), then either
 * plays an idle bark while an ECHIGOYA-family enemy (character_kind &
 * 0xF0 == 0x80) is alert (SR in {1,2}, motion looped back to frame 0, 1-in-
 * 60 roll), or — for everyone else — turns to face Degree while alert
 * (FRAMES_UNTIL_END_OF_ALERT != 0) or runs a short SR state machine that
 * settles into a stand/give-up motion once the alert timer expires.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `(u16)(SR - 1) < 2` (cast the DIFFERENCE, not `(u16)SR - 1`) is what
 *    reproduces the target's `sltiu`; casting only the operand promotes
 *    back to a signed int subtraction and gives `slti` instead — same
 *    length, wrong instruction.
 *  - Both `if (kind==0x80) {...} return turn_towards_player_(...)&~0x5FFF;`
 *    and the sibling `if (FRAMES!=0) {...} return turn_towards_player_(...)
 *    &~0x5FFF;` need their OWN independent `return` statement (not a shared
 *    `goto` to one trailing label). Two adjacent, textually-identical
 *    `return` statements let jump2's cross-jump merge them into ONE
 *    physical call site placed early (right after the kind==0x80 block);
 *    routing both through an explicit shared label instead pins that one
 *    physical copy at the LABEL's own textual position (the function's
 *    end), which is a real 8-byte layout regression — the shared-tail
 *    lever only fires on independently-written identical returns, not on
 *    an explicit `goto` to one.
 *  - The outer `if (FRAMES_UNTIL_END_OF_ALERT == 0) {BIG} else {SMALL}`
 *    needed inverting to `if (FRAMES_UNTIL_END_OF_ALERT != 0) {SMALL} else
 *    {BIG}` — the "if (cond) A; else B; makes A the fall-through and
 *    negates cond" rule: target's own branch tests `FRAMES==0` directly and
 *    branches AWAY to the complex (BIG) body while the trivial (SMALL) body
 *    falls through, so the source has the arms the opposite way from
 *    Ghidra's literal polarity.
 *  - The final SR dispatch (1 / <2 / ==2 / else) is a genuine test-order-
 *    vs-body-order split: tests run 1, <2, ==2, fall-through-else, but
 *    bodies are laid out low-range first, ==2 second, ==1 LAST (right
 *    before the shared `return 0`, needing no trailing jump) — an explicit
 *    `if (cond) goto label;` ladder with labels in the target's own body
 *    order reproduces this (nested if/else-if does not).
 *  - `something_about_current_animation` (game_types.h, offset 0x5C) is
 *    the same struct as item.h's `MotionManager` under this TU's own
 *    (weaker) name — `frames_since_animation_start`@0x2 is `count`.
 *    `some_other_x_position`/`some_other_z_position` (0x80/0x84) are
 *    item.h Humanoid's `chase[0]`/`chase[1]` under this TU's name (first
 *    function to prove chase[] through the character_state view).
 */

extern character_state *Me_THINK_C;
extern u16 Attrib;
extern s16 SR;
extern s32 FRAMES_UNTIL_END_OF_ALERT;
extern s16 turn_towards_player_(s32 x_diff, s32 z_diff);
extern void Sound(character_state *cs, int seid);
extern s16 SetNowMotion(Humanoid *human, s16 mid, s16 move);
extern int rand(void);

s16 Think4abandon(void)
{
    u16 uVar4;
    s16 result;

    uVar4 = Attrib & 0xFFEC;
    Me_THINK_C->some_other_z_position = 0;
    Me_THINK_C->some_other_x_position = 0;
    if ((Me_THINK_C->character_kind & 0xF0) == 0x80)
    {
        if ((u16)(SR - 1) < 2)
        {
            Attrib = uVar4 | 2;
            if (Me_THINK_C->something_about_current_animation->frames_since_animation_start == 0)
            {
                s32 r;

                r = rand();
                if (r == (r / 60) * 60)
                {
                    Sound(Me_THINK_C, 0xD);
                }
            }
        }
        return (s16)(turn_towards_player_(0, 0) & ~0x5FFF);
    }
    else
    {
        if (FRAMES_UNTIL_END_OF_ALERT != 0)
        {
            if (SR == 1)
            {
                Attrib = uVar4 | 2;
            }
            return (s16)(turn_towards_player_(0, 0) & ~0x5FFF);
        }
        else
        {
            if (((Humanoid *)Me_THINK_C)->think[3] == (think_func_)Think4abandon)
            {
                result = (s16)(turn_towards_player_(0, 0) & ~0x5FFF);
                if (result != 0)
                {
                    return result;
                }
            }
            if (SR == 1)
            {
                goto sr_eq_1;
            }
            if (SR < 2)
            {
                goto sr_low;
            }
            if (SR == 2)
            {
                goto sr_eq_2;
            }
            return 0;

        sr_low:
            if (0 <= SR)
            {
                return 0;
            }
            if (SR < -2)
            {
                return 0;
            }
            Attrib = uVar4;
            SetNowMotion((Humanoid *)Me_THINK_C, 0x80F, 1);
            Sound(Me_THINK_C, 0xE);
            return 0;

        sr_eq_2:
            Attrib = uVar4 | 1;
            SetNowMotion((Humanoid *)Me_THINK_C, 0x80F, 1);
            return 0;

        sr_eq_1:
            Attrib = uVar4 | 2;
            return 0;
        }
    }
}

