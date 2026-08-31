#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "humanoid.h"
#include "game_globals.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think4abandon(void);
 *     THINK_4.C:14, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $v1       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Attrib;
 *     extern short SR;
 *     extern long EmergencyNotice;
 * END PSX.SYM */

/*
 * Think4abandon (0x8002f254, 0x1D4 bytes) — think-level-4 "give up the
 * chase" handler, same THINK_4.C TU as Think4contact/Think4chase (both
 * parked). Clears the chase target (some_other_x/z_position), then either
 * plays an idle bark while an ECHIGOYA-family enemy (character_kind &
 * 0xF0 == PAGE_BOSS) is alert (SR in {1,2}, motion looped back to frame 0, 1-in-
 * 60 roll), or — for everyone else — turns to face Degree while alert
 * (EmergencyNotice != 0) or runs a short SR state machine that
 * settles into a stand/give-up motion once the alert timer expires.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `(u16)(SR - 1) < 2` (cast the DIFFERENCE, not `(u16)SR - 1`) is what
 *    reproduces the target's `sltiu`; casting only the operand promotes
 *    back to a signed int subtraction and gives `slti` instead — same
 *    length, wrong instruction.
 *  - Both `if ((Me_THINK_C->type & PAGE_MASK) == PAGE_BOSS) {...} return turn_towards_player_(...)&~0x5FFF;`
 *    and the sibling `if (EmergencyNotice!=0) {...} return turn_towards_player_(...)
 *    &~0x5FFF;` need their OWN independent `return` statement (not a shared
 *    `goto` to one trailing label). Two adjacent, textually-identical
 *    `return` statements let jump2's cross-jump merge them into ONE
 *    physical call site placed early (right after the kind==0x80 block);
 *    routing both through an explicit shared label instead pins that one
 *    physical copy at the LABEL's own textual position (the function's
 *    end), which is a real 8-byte layout regression — the shared-tail
 *    lever only fires on independently-written identical returns, not on
 *    an explicit `goto` to one.
 *  - The outer `if (EmergencyNotice == 0) {BIG} else {SMALL}`
 *    needed inverting to `if (EmergencyNotice != 0) {SMALL} else
 *    {BIG}` — the "if (cond) A; else B; makes A the fall-through and
 *    negates cond" rule: target's own branch tests `EmergencyNotice==0` directly and
 *    branches AWAY to the complex (BIG) body while the trivial (SMALL) body
 *    falls through, so the source has the arms the opposite way from
 *    Ghidra's literal polarity.
 *  - The final SR dispatch remains a test-order-vs-body-order split. An
 *    inverse `SR >= 2` guard lets the low-range body fall through in place;
 *    the glimpse and seen edges remain explicit because their bodies must
 *    stay after that low block. A full switch changes the test order.
 *  - `something_about_current_animation` (game_types.h, offset 0x5C) is
 *    the same struct as item.h's `MotionManager` under this TU's own
 *    (weaker) name — `frames_since_animation_start`@0x2 is `count`.
 *    `some_other_x_position`/`some_other_z_position` (0x80/0x84) are
 *    item.h Humanoid's `chase[0]`/`chase[1]` under this TU's name (first
 *    function to prove chase[] through the Humanoid view).
 */

extern Humanoid *Me_THINK_C;
extern long EmergencyNotice;
extern s16 turn_towards_player_(s32 x_diff, s32 z_diff);
extern int rand(void);

s16 Think4abandon(void)
{
    u16 cleared;
    s16 result;

    cleared = ATTRIB_BITS & ~(ATTR_SEARCH | ATTR_PHASE);
    Me_THINK_C->chase[HUMANOID_CHASE_Z] = 0;
    Me_THINK_C->chase[HUMANOID_CHASE_X] = 0;
    if ((Me_THINK_C->type & PAGE_MASK) == PAGE_BOSS)
    {
        if ((u16)(SR - 1) < 2)
        {
            Attrib = cleared | PHASE_ALERT;
            if (Me_THINK_C->motion->count == 0)
            {
                s32 r;

                r = rand();
                if (r % 60 == 0)
                {
                    Sound(Me_THINK_C, CHAR_VOICE_ALERT);
                }
            }
        }
        return (turn_towards_player_(0, 0) & ~0x5FFF);
    }
    else if (EmergencyNotice != 0)
    {
        if (SR == SR_SEEN)
        {
            Attrib = cleared | PHASE_ALERT;
        }
        return (turn_towards_player_(0, 0) & ~0x5FFF);
    }
    else
    {
        if (Me_THINK_C->think[3] == Think4abandon)
        {
            result = (turn_towards_player_(0, 0) & ~0x5FFF);
            if (result != 0)
            {
                return result;
            }
        }
        if (SR == SR_SEEN)
        {
            goto sr_seen;
        }
        if (SR >= 2)
        {
            if (SR == SR_GLIMPSE)
            {
                goto sr_glimpse;
            }
            return 0;
        }

        if (SR >= SR_NONE)
        {
            return 0;
        }
        if (SR < SR_GONE)
        {
            return 0;
        }
        /* Target lost: stand down (0x80f sheathes and returns to
         * idle — ActSTATE) with the give-up voice line. */
        Attrib = cleared;
        SetNowMotion(Me_THINK_C, MOT_STATE_SHEATHE, 1);
        Sound(Me_THINK_C, CHAR_VOICE_REACTION);
        return 0;

    sr_glimpse:
        /* Only a glimpse: drop to suspicious and stand down. */
        Attrib = cleared | PHASE_SUSPICIOUS;
        SetNowMotion(Me_THINK_C, MOT_STATE_SHEATHE, 1);
        return 0;

    sr_seen:
        Attrib = cleared | PHASE_ALERT;
        return 0;
    }
}
