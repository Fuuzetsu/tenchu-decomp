#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "humanoid.h"
#include "game_globals.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1trace(void);
 *     THINK_1.C:16, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a0       long xx
 *     reg   $a1       long zz
 *     reg   $t0       short pad
 *     reg   $a0       long vx
 *     reg   $a1       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short Degree;
 *     extern short Attrib;
 * END PSX.SYM */

/*
 * Think1trace (0x8002bfec, 0xd8 bytes) — think-handler (same "think" TU as
 * Think1sleep.c/Think2confirm.c/Think2contact.c). Two
 * modes selected by actcnt: while actcnt == 0, count frames in actscnt and
 * (for the first 0x3C frames) steer towards the player by comparing the
 * character's turn rate against Degree; once actscnt reaches 0x3C, flip into
 * actcnt >= 1 tracking mode, whose branch here increments actcnt each call
 * and (when ATTR_TRACE is set) forwards to ControlTraceLine.
 *
 * Like Think2contact.c, this retail site reads the recovered signed `Attrib`
 * object's raw flag bits with `lhu`, represented by the shared
 * `ATTRIB_BITS` view.
 *
 * `old_actscnt` must be a real local: the asm stores actscnt+1 back
 * UNCONDITIONALLY (in the branch's delay slot) before testing whether the
 * OLD value was below 0x3C, so the store and the compare read different
 * values. The later `self->actscnt < 30` check is a fresh reload of
 * the (already-incremented) field, not old_actscnt again.
 *
 * `result` carries the handler's signed-short pad command on every path; the
 * final return performs the one shared normalization.
 *
 * The `self`/`turn`/`degree`/`abs_degree` block reproduces the target's
 * load-scheduling exactly (`lw self; lh degree` — Degree's independent load
 * fills self's load-delay slot — `lh turn` via self, now 2 insns clear;
 * `bgez`/`negu` on degree) only with the identical body duplicated into
 * `if (Me_THINK_C->actcnt || degree) {A} else {A}` (found by the permuter;
 * `degree` is read before its first assignment in that guard, but it's
 * always overwritten before use — harmless, both arms are byte-identical).
 * Splitting the block into two front-end basic blocks changes cc1's
 * pseudo/CSE numbering enough to pick this schedule; the single
 * straight-line version (no if/else) compiles 8 bytes longer with a stray
 * load-delay nop and puts `result` in $a1 instead of $a2. Likewise
 * `abs_degree = -abs_degree;` (not `-degree`) picks `negu v0,v0` over
 * `negu v0,a1` for the same reason (the negate should read the value
 * already copied into abs_degree's register, not re-read degree's).
 */

extern Humanoid *Me_THINK_C;

s16 Think1trace(void)
{
    s16 result;

    result = 0;
    if (Me_THINK_C->actcnt == 0)
    {
        u8 old_actscnt;

        old_actscnt = Me_THINK_C->actscnt;
        Me_THINK_C->actscnt = old_actscnt + 1;
        if (old_actscnt < 60)
        {
            Humanoid *self;
            s32 turn;
            s32 degree;
            s32 abs_degree;

            if (Me_THINK_C->actcnt || degree)
            {
                self = Me_THINK_C;
                degree = Degree;
                turn = self->turn;
                abs_degree = degree;
            }
            else
            {
                self = Me_THINK_C;
                degree = Degree;
                turn = self->turn;
                abs_degree = degree;
            }
            if (degree < 0)
            {
                abs_degree = -abs_degree;
            }
            if (turn < abs_degree)
            {
                if (self->actscnt < 30)
                {
                    result = -PADLleft;
                    if (turn < degree)
                    {
                        result = PADLright;
                    }
                }
            }
        }
        else
        {
            Me_THINK_C->actcnt = 1;
            Me_THINK_C->actscnt = 0;
        }
    }
    else
    {
        Me_THINK_C->actcnt += 1;
        if (ATTRIB_BITS & ATTR_TRACE)
        {
            result = ControlTraceLine(Me_THINK_C);
        }
    }
    return result;
}
