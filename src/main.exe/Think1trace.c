#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think1trace(void);
 *     THINK_1.C:16, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 * Think1sleep.c/Think2confirm.c/think_setting_go_towards_player.c). Two
 * modes selected by actcnt: while actcnt == 0, count frames in actscnt and
 * (for the first 0x3C frames) steer towards the player by comparing the
 * character's turn rate against Degree; once actscnt reaches 0x3C, flip into
 * actcnt == 1 tracking mode, whose branch here just clears actscnt each call
 * and (when Attrib bit 3 is set) forwards to ControlTraceLine.
 *
 * Like think_setting_go_towards_player.c, this file's own compile reads
 * Attrib as u16 (`lhu`) and does not include main.exe.h (whose `extern s16
 * Attrib;` would conflict) — the handful of externs used here are
 * re-declared locally instead.
 *
 * `old_actscnt` must be a real local: the asm stores actscnt+1 back
 * UNCONDITIONALLY (in the branch's delay slot) before testing whether the
 * OLD value was below 0x3C, so the store and the compare read different
 * values. The later `Me_THINK_C->actscnt < 0x1E` check is a fresh reload of
 * the (already-incremented) field, not old_actscnt again.
 *
 * `result` stays s32 throughout (the -0x8000/0x2000 constants and
 * ControlTraceLine's return are never individually truncated); only the
 * final `return` truncates once to s16, shared by every path.
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

extern character_state *Me_THINK_C;
extern u16 Attrib;
extern s16 Degree;
extern s32 ControlTraceLine(character_state *me);

s16 Think1trace(void)
{
    s32 result;

    result = 0;
    if (Me_THINK_C->actcnt == 0)
    {
        u8 old_actscnt;

        old_actscnt = Me_THINK_C->actscnt;
        Me_THINK_C->actscnt = old_actscnt + 1;
        if (old_actscnt < 0x3C)
        {
            character_state *self;
            s32 turn;
            s32 degree;
            s32 abs_degree;

            if (Me_THINK_C->actcnt || degree)
            {
                self = Me_THINK_C;
                degree = Degree;
                turn = self->character_rotation_speed;
                abs_degree = degree;
            }
            else
            {
                self = Me_THINK_C;
                degree = Degree;
                turn = self->character_rotation_speed;
                abs_degree = degree;
            }
            if (0 > degree)
            {
                abs_degree = -abs_degree;
            }
            if (turn < abs_degree)
            {
                if (self->actscnt < 0x1E)
                {
                    result = -0x8000;
                    if (turn < degree)
                    {
                        result = 0x2000;
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
        if (Attrib & 8)
        {
            result = ControlTraceLine(Me_THINK_C);
        }
    }
    return result;
}
