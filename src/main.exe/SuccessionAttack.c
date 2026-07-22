#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"
#include "game_globals.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static short SuccessionAttack(long dist, short deg);
 *     THINK_3.C:247, 15 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       long dist
 *     param $a1       short deg
 *
 * Globals it touches, as the original declared them:
 *     extern struct BattleType BattleDB[78];
 *     extern long Distance;
 *     extern short Degree;
 *     extern short EngageLevel;
 * END PSX.SYM */

/*
 * STATUS: MATCHING — pure C, all 268 bytes exact (67 instructions), with
 * the function-local and whole-image byte counts both at zero.
 *
 * SuccessionAttack (0x8002fabc, 0x10c bytes) — same "think" TU as
 * Think3chase.c/Think3escape.c/Think3firstattack.c/Think1trace.c (s16
 * return; gp-relative Me_THINK_C/Distance/Degree/EngageLevel — see gpsyms).
 * Called (as a static helper, per PSX.SYM) from Think3area/Think3attack/
 * Think3hitaway to decide whether the current attack motion may chain into
 * a follow-up.
 *
 * Guard: only continues if the character's current animation frame
 * (Me_THINK_C->motion->count, the MotionManager frame counter — the same
 * field Think1sleep.c proves, and AttackContinuousCheck's `dtM->count`)
 * equals BattleDB[warid].contfrm — `warid` is Me_THINK_C's field @0x8C, the
 * official Humanoid name (item.h) for what was the guessed
 * `index_s32o_animation_collection`.
 *
 * `BattleType` comes from game_types.h (the official 8-short layout, confirmed
 * identical to `reference/psxsym-types.h`) — `contfrm` @0x8, the offset the
 * asm's `lh v0,8(v0)` confirms after the `sll v0,v0,4` (0x10 stride) index
 * scale.
 *
 * The `iVar1 % iVar2` (`rand() % (EngageLevel+1)`) division-guard trap
 * sequence (`break 7`/`break 6`) needs maspsx `--expand-div` for this file
 * (added to Build.hs's `extra`/permute.py's `MASPSX_EXTRA` — same lever as
 * Think3escape/GetAreaMapLevel/bow_shoot_logic): WITHOUT it the trap guard
 * is silently dropped from the assembled output even though cc1 emitted it,
 * a ~150-byte-diff red herring that looks like a missing C construct.
 *
 * `uVar3` must stay 16-bit (NOT the `u8` autorules suggests, which "wins" by
 * 2 bytes but is a FALSE WIN: `uVar3 = 0x8000;`/`= 0x2000;` truncate to 0 in
 * a u8, an outright wrong value — reject per the cookbook's "never accept
 * an autorules win that changes what a value actually holds" caveat, this
 * time for a plain local rather than a struct field).
 *
 * `int d = deg;` declared as the FIRST statement inside the `Distance<dist`
 * block (not `deg` used inline) fixed the function's LENGTH (was 1
 * instruction/4 bytes short): the fallthrough block's first statement gets
 * hoisted into the OUTER guard branch's delay slot, and only `deg`'s own
 * sign-extension — not `Degree`'s load — is independent enough of the
 * ABS-compute to serve as that filler when it is the textually-first
 * reference.
 *
 * FIVE REAL STRUCTURAL FIXES applied across the focused sessions, found by
 * decoding the target's raw instructions address-by-address (not guessing):
 *
 * 1. The tail's `if/else` at LAB_8002fb84 has its arms SWAPPED relative to
 *    Ghidra's rendering: the target's `bnez`/fallthrough shape decodes as
 *    `if (Degree >= 0x12d) { uVar3 = 0x2000; } else { uVar3 |= 0x80; if
 *    (Degree < -300) { uVar3 = -0x8000; } else { goto ret; } } uVar3 |= 0x80;
 *    ret: return uVar3;` — i.e. Ghidra's literal `if (Degree<0x12d){ if
 *    (-0x12d<Degree) return 0x80; ... }` is the INVERSE condition with the
 *    bodies swapped, PLUS the `return 0x80;` is really a `goto` that skips
 *    a SECOND, later `uVar3 |= 0x80;` (the delay slot of the skip-edge's
 *    `beqz` executes an unconditional `ori s0,s0,0x80` every time that arm
 *    is even considered, per MIPS delay-slot semantics — decode the
 *    disassembly with delay slots in mind, not the pseudo-linear reading).
 *    Getting the if/else polarity right fixed the branch layout AND, via a
 *    named `ret:` label + `goto` (not a second literal `return uVar3;` —
 *    two textually-identical `return` tails do NOT cross-jump-merge in this
 *    cc1; route the second through a `goto` to a single shared one, per the
 *    cookbook's Shared-tails section) the shared-return register colouring.
 * 2. `uVar3` must be `s16` (not `u16`): assigning the literal `-0x8000`
 *    (0x8000's bit pattern) to a `u16` still folds to the UNSIGNED
 *    representation internally, so cc1 emits `li s0,0x8000` (`ori`) instead
 *    of the target's `li s0,-32768` (`addiu`) — same 4 bytes, wrong opcode.
 *    `s16` reproduces the target's `addiu` encoding exactly.
 * 3. Store `iVar1 < d` back into `iVar1` before testing it. This deliberately
 *    reuses Degree's dying carrier, reproducing the target's `slt v1,...` and
 *    `bnez v1` instead of allocating a fresh `$v0` (8 -> 6 bytes).
 * 4. Mutate `raw` with `__builtin_abs` before the comparison. GCC keeps this
 *    as one opaque `abssi2` RTL pattern through delayed-branch reorganization,
 *    then emits the target's in-place `bgez v1; nop; negu v1`. A manual
 *    sign-fix exposed the branch early, allowing `fill_simple_delay_slots` to
 *    steal the preceding `sra v0` into its delay slot. A distinct builtin
 *    destination emitted the three-register `move` form and added 8 bytes;
 *    mutating `raw` gives the exact register tie and schedule.
 * 5. Route the modulo-failure edge through the existing typed `ret` label.
 *    With opaque `abssi2` reducing the visible RTL jump count, a literal
 *    `return uVar3` was cross-jump-merged with the initial `return 0`, moving
 *    the early-return block and cascading 100 bytes. `goto ret` preserves the
 *    target's distinct immediate-zero return and shared signed-s16 epilogue.
 */
extern Humanoid *Me_THINK_C;
extern s32 Distance;
extern s16 Degree;
extern s16 EngageLevel;
extern int rand(void);

short SuccessionAttack(long dist, short deg)
{
    int iVar1;
    int iVar2;
    s16 uVar3;

    uVar3 = 0;
    if (Me_THINK_C->motion->count !=
        BattleDB[Me_THINK_C->warid].contfrm)
    {
        return 0;
    }
    if (Distance < dist)
    {
        int d;
        int raw;

        d = deg;
        raw = (int)Degree;
        raw = __builtin_abs(raw);
        iVar1 = raw < d;
        if (iVar1) goto LAB_8002fb84;
    }
    iVar1 = rand();
    iVar2 = EngageLevel + 1;
    if (iVar1 % iVar2 != 0)
    {
        goto ret;
    }
LAB_8002fb84:
    if (Degree >= 0x12d)
    {
        uVar3 = 0x2000;
    }
    else
    {
        uVar3 |= 0x80;
        if (Degree < -300)
        {
            uVar3 = -0x8000;
        }
        else
        {
            goto ret;
        }
    }
    uVar3 |= 0x80;
ret:
    return uVar3;
}
