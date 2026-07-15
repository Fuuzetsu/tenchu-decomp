#include "common.h"
#include <psxsdk/libgs.h>
#include "game_types.h"

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
 * STATUS: NON_MATCHING — 8 of 268 bytes differ (right length: matchdiff's
 * whole-image count equals the function-local count). Was 25 bytes; closed
 * to 8 this session by fixing two real structural mismatches (see below) —
 * the remaining 8 are a dump-confirmed sched1/sched2 tie (see RESIDUAL).
 *
 * SuccessionAttack (0x8002fabc, 0x10c bytes) — same "think" TU as
 * Think3chase.c/Think3escape.c/Think3firstattack.c/Think1trace.c (s16
 * return; gp-relative Me_THINK_C/Distance/Degree/EngageLevel — see gpsyms).
 * Called (as a static helper, per PSX.SYM) from Think3area/Think3attack/
 * Think3hitaway to decide whether the current attack motion may chain into
 * a follow-up.
 *
 * Guard: only continues if the character's current animation frame
 * (Me_THINK_C->something_about_current_animation->frames_since_animation_
 * start, the same field Think1sleep.c proves) equals BattleDB[idx].contfrm
 * — `idx` is Me_THINK_C's field @0x8C (already proven
 * `index_s32o_animation_collection` by another matched function; Ghidra's
 * own name for it here is "warid" — plausible, but not adopted without
 * `tools/callmatch.py --verify`).
 *
 * `BattleType` is declared locally (not a shared header) straight from
 * `reference/psxsym-types.h`'s proven 8-short layout — `contfrm` @0x8, the
 * offset the asm's `lh v0,8(v0)` confirms after the `sll v0,v0,4` (0x10
 * stride) index scale.
 *
 * The `iVar1 % iVar2` (`rand() % (EngageLevel+1)`) division-guard trap
 * sequence (`break 7`/`break 6`) needs maspsx `--expand-div` for this file
 * (added to Build.hs's `extra`/permute.py's `MASPSX_EXTRA` — same lever as
 * Think3escape/GetAreaMapLevel/bow_shoot_logic): WITHOUT it the trap guard
 * is silently dropped from the assembled output even though cc1 emitted it,
 * a ~150-byte-diff red herring that looks like a missing C construct.
 *
 * `uVar3` must stay `u16` (NOT the `u8` autorules suggests, which "wins" by
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
 * TWO REAL STRUCTURAL FIXES applied this session (25 -> 8 bytes), found by
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
 *
 * RESIDUAL (8 bytes, same length, dump-confirmed permuter-immune): the
 * ABS(Degree)-vs-`d` compare block (0x8002fb1c-0x8002fb30). Target order is
 * `sll d(hi); lh Degree; sra d(lo); bgez Degree,L; nop; negu Degree; slt
 * v1,Degree,d; bnez v1,L2` — i.e. `d`'s `sra` completes BEFORE the `bgez`
 * test, using the Degree-load's own load-delay slot as its filler, and the
 * `slt`'s result lands in $v1 (Degree's own dying register). Ours instead
 * fills the Degree-load's delay slot with a `nop` and steals `d`'s `sra`
 * into `bgez`'s delay slot instead, and the `slt` lands in a fresh $v0.
 * `tools/rtldump.py --pass all`'s `.sched`/`.sched2` dumps show this is a
 * genuine backward-list-scheduling TIE at the RTL level: in basic block
 * "3 from 61 to 70", insns 61 (`d`'s sll), 62 (`d`'s sra) and 67 (Degree's
 * combined `lh`-sign-extend) all report `priority = 1, ref_count = 1` —
 * exactly equal — so which of {61,62} vs 67 the scheduler treats as the
 * Degree-load's mandatory 1-cycle-latency filler vs the bgez's delay-slot
 * filler is decided by an internal tie-break this session did not fully
 * reverse-engineer, not by any C-level distinction. Tried and rejected:
 * `d: int -> s16/u32` (autorules, worse: 14/6 differing lines), inlining
 * `d` at its use (autorules, worse: 14), reading `deg` directly instead of
 * `d` at the comparison (LENGTH MISMATCH, +4 bytes) — none touch the
 * scheduler's tie-break. A bounded permuter run (300s, `--stop-on-zero
 * -j4`) found no zero. Matches the cookbook's "min/clamp reload-vs-reuse is
 * a coloring tie" class, one level up (a scheduling tie, not a coloring
 * one) — do not keep guessing respellings; the next lever, if any, is
 * reading GCC 2.8.1's `sched.c` `rank_for_schedule`/`schedule_block` tie-
 * break for equal-priority ready-list entries.
 */
extern character_state *Me_THINK_C;
extern s32 Distance;
extern s16 Degree;
extern s16 EngageLevel;
extern int rand(void);

typedef struct
{
    s16 mid;
    s16 power;
    s16 atks;
    s16 atke;
    s16 contfrm;
    s16 revise;
    s16 ilus;
    s16 ilue;
} BattleType;

extern BattleType BattleDB[];

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SuccessionAttack", SuccessionAttack);
#else /* NON_MATCHING */

short SuccessionAttack(long dist, short deg)
{
    int iVar1;
    int iVar2;
    s16 uVar3;

    uVar3 = 0;
    if (Me_THINK_C->something_about_current_animation->frames_since_animation_start !=
        BattleDB[Me_THINK_C->index_s32o_animation_collection].contfrm)
    {
        return 0;
    }
    if (Distance < dist)
    {
        int d = deg;
        iVar1 = (int)Degree;
        if (iVar1 < 0)
        {
            iVar1 = -iVar1;
        }
        if (iVar1 < d) goto LAB_8002fb84;
    }
    iVar1 = rand();
    iVar2 = EngageLevel + 1;
    if (iVar1 % iVar2 != 0)
    {
        return uVar3;
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

#endif /* NON_MATCHING */
