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
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
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
 * STATUS: NON_MATCHING — 25 of 268 bytes differ (right length: matchdiff's
 * whole-image count equals the function-local count).
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
 * RESIDUAL (25 bytes, same length): the ABS(Degree)-vs-`d` compare (`slt`)
 * lands in $v1 in the target (reusing Degree's own register) vs $v0 here, a
 * pure register-color tie, plus one 4-byte nop-position difference right
 * before it. The tail's `if (-0x12d < Degree) return 0x80;` is Ghidra's
 * literal (and byte-correct) rendering of what's really a `goto` to the
 * shared `return uVar3 | 0x80;` (uVar3 still 0) — tried spelling it that way
 * explicitly (`goto ret;` + a `ret:` label at the following `return`), which
 * made the diff WORSE (28 bytes) by disturbing the delay-slot duplication
 * reorg already produces from the literal `return 0x80;` form — reverted.
 * `tools/autorules.py` found no further (real) win; one bounded permuter
 * run (~1300 iterations, --stop-on-zero -j4) only reached 465 (baseline
 * 475, no visible correlation to the true 25-byte gap) — not re-run per the
 * attempt-cap guidance.
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
    u16 uVar3;

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
    if (Degree < 0x12d)
    {
        if (-0x12d < Degree)
        {
            return 0x80;
        }
        uVar3 = 0x8000;
    }
    else
    {
        uVar3 = 0x2000;
    }
    return uVar3 | 0x80;
}

#endif /* NON_MATCHING */
