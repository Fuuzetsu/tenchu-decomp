#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short Think4contact(void);
 *     THINK_4.C:36, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     reg   $a1       short pad
 *     reg   $s1       long xx
 *     reg   $s2       long zz
 *     reg   $s1       long vx
 *     reg   $s2       long vz
 *     reg   $v0       short deg
 *     reg   $s0       short pad
 *
 * Globals it touches, as the original declared them:
 *     extern short SR;
 *     extern short Attrib;
 *     extern short Degree;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 183 of 384 bytes differ (right length in every
 * OTHER respect: matchdiff's whole-image count off this function is exactly
 * accounted for by its own residual, nothing downstream drifts once this
 * file's INCLUDE_ASM stub is in effect).
 *
 * Think4contact (0x8002f428, 0x180 bytes) — think-handler, same "think" TU
 * as Think4chase.c/Think4abandon.c. When SR==1 (a stunned/interrupted
 * state?), clears it to attrib-mode 2 and returns 0. Otherwise, while no
 * chase target is set (both some_other_x/z_position are 0): for the first
 * 0x5B ticks, steer towards Degree via turn/character_rotation_speed
 * (0x2000 right, -0x8000 left, 0 once aligned); past that, defer to
 * Think4abandon. Once a chase target IS set: steer towards it via
 * turn_towards_player_, and once within 1000 units (or actscnt wrapped to
 * 0), clear the chase target and actcnt. ALL BYTE-PROVEN except the
 * residual below (confirmed by reading the raw `.s`, not just Ghidra).
 *
 * `FUN_8002b990` is `turn_towards_player_` at the same address (see
 * Think1random.c's header note) — Ghidra's own rendering here already
 * shows the real (dx,dz) arguments, confirming the sibling calls' true
 * signature.
 *
 * `result` (Ghidra's `sVar2`) is ONE s32 pseudo for the whole function
 * (game_types.h's `character_rotation_speed`/Degree branch sets it in a
 * callee-saved register even though THAT branch alone never crosses a
 * call — the SAME variable crosses turn_towards_player_/SquareRoot0 in the
 * chase-active branch, so cc1 keeps it in $s0 everywhere); only the
 * shared final `return result;` truncates to s16 (the SR==1 path's
 * `return 0;` and the `return Think4abandon();` tail call each get their
 * own return-site handling — a literal 0 needs no truncation, a genuine
 * `return call();` gets its own sll/sra immediately after the jal — this
 * part IS byte-identical to target already).
 *
 * The `0x5B <= actcnt` guard needed the OPPOSITE polarity from Ghidra's
 * literal `if (actcnt < 0x5B) {BIG} else {return Think4abandon();}`: since
 * the success arm is the much LONGER one, cc1 places
 * `return Think4abandon();` as the physically-first (fallthrough) body and
 * the big block as the branch target — the "flip back when the SUCCESS arm
 * is the longer one" cookbook exception. Fixed and byte-identical.
 *
 * RESIDUAL (the whole 183-byte gap, 8 bytes short of target's length): the
 * turn/Degree 3-way dispatch (0x2000 / -0x8000 / 0) that picks `result`.
 * Both `if (turn<Degree) {A} else {B; if(...) {C}}` (Ghidra's literal
 * nesting) and an `if/else-if/else` chain and an explicit goto-ladder
 * (`if(turn<Degree) goto A; B; if(...) C; goto join; A: …; join:`) compile
 * to IDENTICAL bytes here (verified by trying all three) — cc1 normalizes
 * them to the same internal form. The difference from target: our compile
 * has EVERY one of the three outcomes branch DIRECTLY to the function's
 * shared final merge (one `beqz`/`bnez` each, no extra jump); the target
 * instead spends an extra unconditional `j` for the `turn<Degree` (0x2000)
 * arm specifically (it's the fallthrough of the first test, so its body —
 * one `li` — still needs its own jump to the far merge, since it isn't
 * naturally adjacent to it) while its OWN "both tests false" (result stays
 * 0) arm computes its `sll v0,s0,0x10` truncation INLINE in a guard's delay
 * slot and jumps to a EARLIER point in the shared tail (skipping the
 * shared `sll`), rather than merging with the other two arms. This is a
 * jump.c-level block-layout/threading decision (which candidate delay-slot
 * filler and merge point cc1's `reorg`/jump-threading picks for a 3-way
 * value-producing dispatch), not a source-shape lever: tried and
 * BYTE-IDENTICAL to each other (not just close) — `s16 result` vs `s32
 * result`; the do{}while(0) priority-boost wrapper around the whole
 * dispatch; declaring `Degree <= turn` (De-Morgan complement) instead of
 * `turn < Degree`. `tools/permute.py Think4contact -- --stop-on-zero -j4`
 * (~3100 iterations, ~3 min) never beat the base score once. Root-causing
 * further would need reading cc1's jump.c/reorg.c against an RTL dump
 * (`cc1 -dj`/`-dg`), not source experimentation — parked per the cookbook's
 * attempt-cap guidance. decomp.me (psyq4.3 preset) would be the next
 * arbiter if revisited.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/Think4contact", Think4contact);
#else
extern s16 SR;
extern s16 Attrib;
extern s16 Degree;
extern s16 Think4abandon(void);
extern s32 SquareRoot0(s32 x);

s16 Think4contact(void)
{
    s32 result;

    if (SR == 1)
    {
        Attrib = (Attrib & 0xFFFC) | 2;
        return 0;
    }

    if (Me_THINK_C->some_other_x_position == 0 && Me_THINK_C->some_other_z_position == 0)
    {
        if (0x5B <= Me_THINK_C->actcnt)
        {
            return Think4abandon();
        }
        else
        {
            Me_THINK_C->actcnt = Me_THINK_C->actcnt + 1;
            if (Me_THINK_C->character_rotation_speed < Degree)
            {
                result = 0x2000;
            }
            else
            {
                result = 0;
                if (Degree < -Me_THINK_C->character_rotation_speed)
                {
                    result = -0x8000;
                }
            }
        }
    }
    else
    {
        s32 dx, dz;

        Me_THINK_C->actscnt = Me_THINK_C->actscnt + 1;
        dx = Me_THINK_C->some_other_x_position - Me_THINK_C->some_kind_of_current_position->vx;
        dz = Me_THINK_C->some_other_z_position - Me_THINK_C->some_kind_of_current_position->vz;
        result = turn_towards_player_(dx, dz);
        if (SquareRoot0(dx * dx + dz * dz) < 1000 || Me_THINK_C->actscnt == 0)
        {
            Me_THINK_C->some_other_z_position = 0;
            Me_THINK_C->some_other_x_position = 0;
            Me_THINK_C->actcnt = 0;
        }
    }
    return result;
}
#endif
