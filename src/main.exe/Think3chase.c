#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think3chase(void);
 *     THINK_3.C:60, 5 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern long Distance;
 *     extern short SR;
 *     extern short EngageLevel;
 *     extern long GameClock;
 *     extern long AttackActionCount;
 *     extern short Degree;
 * END PSX.SYM */

/*
 * Think3chase (0x8002cf74, 0x13c bytes) — think-handler, same "think" TU as
 * Think1trace.c/Think1sleep.c/Think2confirm.c (s16 return convention;
 * gp-relative Distance/SR/EngageLevel/Degree/AttackActionCount — see gpsyms).
 *
 * If close enough (Distance < 10000) and not already in the "-2" SR state,
 * clear SR. If the next scheduled attack action (AttackActionCount) is due
 * (< GameClock) AND the character isn't badly misaimed (abs(Degree) < 500),
 * pick a pad-command result by distance band, reschedule the next attack
 * action, and return early. Otherwise (attack not due, or misaimed), fall
 * through to an INDIRECT CALL: `Me_THINK_C->weapon_kind` (already-proven u16
 * enum field, shared with Humanoid's "wpatk" at the same 0x8E) divided by
 * 16 selects a per-weapon-category handler out of AttackFunc[]; the
 * `(s16)weapon_kind >> 4` cast+shift is cc1's combined sign-extend-and-scale
 * fold (same `sll 16/sra N` shape as GetAttackDBID's array index, here
 * dividing instead of multiplying since the shift is 20 not 12).
 *
 * The indirect call takes ZERO arguments: m2c renders it as
 * `AttackFunc[...](GameClock)`, but the raw asm sets up no a0-a3 before the
 * `jalr` — GameClock is just a stale leftover in $a0 from the earlier
 * "next attack due" check, not a real argument (m2c's over-count warning;
 * Ghidra's own `(*(code *)AttackFunc[...])()` already shows zero args).
 *
 * The three-way Distance dispatch (>= 0xFA1 / >= 0xBB9 / else) needed
 * INVERTED polarity from Ghidra's rendering at BOTH nesting levels: Ghidra
 * shows `if (Distance < 0xfa1) {<0xbb9 dispatch} else {pad.data}`, but the
 * asm places the pad.data body as the physical fallthrough (immediate,
 * smaller) and the <0xfa1 dispatch as a later jump target — the opposite-
 * polarity cookbook rule (swap bodies, negate the test:
 * `if (Distance >= 0xFA1) {pad.data} else if (Distance >= 0xBB9) {SetCommand}
 * else {0x80/0xA0}`). Once inverted this way the length matched exactly
 * once (see STATUS below for the one remaining residual).
 *
 * STATUS: NON_MATCHING — 144 of 316 bytes differ (structurally isolated to
 * 2 extra instructions / 8 bytes at the AttackActionCount update — see
 * `tools/asmdiff.py Think3chase --structural`; the 144-byte figure is that
 * one structural diff cascading through matchdiff's fixed window, not 144
 * independently-wrong bytes).
 *
 * Root cause (confirmed via tools/regalloc.py + reading target's raw .s):
 * for `AttackActionCount = GameClock + EngageLevel * 10;`, the target computes the
 * whole RHS in $v0/$v1 and leaves `result` (the pad-command value, live
 * until the `return` right after) untouched in $a0 the entire time — a
 * single final `sll $v0,$a0,16` at the shared epilogue tail does the s16
 * truncation. Our cc1 instead allocates this SAME computation to $a0/$v1
 * (colliding with `result`), forcing an early rescue of `result` into $v0
 * (an extra `sll $v0,$a0,16` duplicated at BOTH points that reach this
 * statement) before reusing $a0 for the arithmetic — same value, 2
 * instructions (8 bytes) longer, otherwise byte-identical in shape.
 *
 * This is a pure register-color tie, not a wrong instruction: tried (all
 * failed to move it) — `result` as u16/u8/s32; hoisting EngageLevel or
 * GameClock into their own named temp, in either order; commuting the `+`
 * operands; splitting `AttackActionCount = ...; return result;` into all three
 * dispatch arms (hoping cc1's own cross-jump would re-merge it the way the
 * target's does — it didn't; that variant was 92 bytes, worse); tightening
 * `result`'s scope to the innermost block. `tools/autorules.py` found
 * `result: u16->u8` "improves" the count (25->23 lines) but it's a FALSE
 * WIN: u8 narrows the `Me_THINK_C->buttons.currently_pressed` load itself
 * from `lhu` to `lbu`, which is wrong (that field is a proven u16) even
 * though it happens to byte-match this ROM's actual runtime values — reject
 * autorules suggestions that change a proven field's load width, even when
 * they shrink the diff. One bounded decomp-permuter run (~3 min, -j4,
 * --stop-on-zero) plateaued at score 355 (no meaningful movement) — did not
 * re-run per the attempt cap.
 */

extern character_state *Me_THINK_C;
extern s16 SR;
extern s32 Distance;
extern s16 EngageLevel;
extern s32 GameClock;
extern s16 Degree;
extern s32 AttackActionCount; /* next GameClock tick an attack action may fire */
extern u16 SetCommand(some_character_button_values *pad, s32 code);
extern s32 (*AttackFunc[])(void);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/Think3chase", Think3chase);
#else /* NON_MATCHING */

s16 Think3chase(void)
{
    s32 degree;

    if (Distance < 10000 && SR != -2)
    {
        SR = 0;
    }
    if (AttackActionCount + EngageLevel * 30 < GameClock)
    {
        degree = Degree;
        if (degree < 0)
        {
            degree = -degree;
        }
        if (degree < 500)
        {
            u16 result;

            if (Distance >= 0xFA1)
            {
                result = Me_THINK_C->buttons.currently_pressed;
            }
            else if (Distance >= 0xBB9)
            {
                result = SetCommand(&Me_THINK_C->buttons, 0x21);
            }
            else
            {
                result = 0x80;
                if (Distance < 2000)
                {
                    result = 0xA0;
                }
            }
            AttackActionCount = GameClock + EngageLevel * 10;
            return result;
        }
    }
    return AttackFunc[(s16)Me_THINK_C->weapon_kind >> 4]();
}

#endif /* NON_MATCHING */
