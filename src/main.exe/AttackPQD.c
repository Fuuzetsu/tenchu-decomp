#include "common.h"
#include "sound.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackPQD(short sfrm, short efrm);
 *     MOTION.C:849, 23 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short sfrm
 *     param $a1       short efrm
 * END PSX.SYM */

/*
 * AttackPQD (0x80027688, 0xa8 bytes) — weapon holster/draw swap: on a
 * matching motion-frame trigger (dtM->count, MotionManager's proven
 * `count` field) or a wildcard trigger (efrm == MOTION_FRAME_ANY), draws
 * the holstered weapon at weapon[3] into weapon[0] (the active slot) and
 * clears weapon[3]; on the OTHER trigger frame (dtM->count == sfrm), parks
 * the active weapon[0] in weapon[3] and draws weapon[2] into the hand
 * instead. Either swap plays a sound
 * (Sound(human, seid), seid=1 for draw / 0 for holster) — unless the
 * source slot was already empty, in which case it's a silent no-op.
 *
 * Humanoid's weapon[4] array (equipped melee/ranged ornaments — reference/
 * ghidra_types.h's fuller independently-built Humanoid struct, "right/left
 * active + right/left inactive" per game_types.h's Humanoid sibling
 * comment) was hidden inside item.h's opaque pad2b span; extended item.h
 * to reveal it (report: item.h's Humanoid.pad2b[0x34]@0x78 split into
 * pad2b[0x1C]@0x78 + weapon[4]@0x94 + illusion[2]@0xA4, matching
 * game_types.h's already-proven boundaries for this same region).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `human = Me_MOTION_C;` cached ONCE (a local, not a repeated global
 *    re-read) is what keeps it in one register for the whole function —
 *    Ghidra's rendering inconsistently mixes the "human" and "Me_MOTION_C"
 *    names for the SAME loaded value; the asm only ever loads it once.
 *  - `count = dtM->count;` is ALSO cached once at the top (Ghidra's own
 *    `temp_v1` rendering) and reused by BOTH the outer test and the
 *    CASE_B test — the two tests are in different basic blocks, so a
 *    fresh re-read of `dtM->count` at each site would reload; only a
 *    genuine shared local reproduces the single `lh`.
 *  - `weapons = human->weapon;` (the array's base, `human+0x94`) is ALSO
 *    hoisted unconditionally to the top (its `addiu` sits in the FIRST
 *    branch's delay slot — computed before either arm is chosen). BOTH
 *    weapon[2] AND weapon[3] are reached exclusively through `weapons`
 *    (`weapons[2]`/`weapons[3]`) in EVERY branch, never through
 *    `human->weapon[2/3]` directly, even in the branch where that index is
 *    the "own" one being tested — only weapon[0] (offset 0) stays on
 *    `human` directly (folds the 0x94 displacement into human's own
 *    register). Trust the register the raw asm actually uses, not
 *    Ghidra's inconsistent per-statement naming of the identical address.
 *  - Register-allocation tie (permuter, bisected — see below): CASE_A's
 *    "own slot" store must be a CHAINED assignment,
 *    `held = (weapons[2] = human->weapon[0]);`, not the seemingly-equivalent
 *    `held = human->weapon[0]; weapons[2] = held;` — the chained form is what
 *    makes cc1 reuse $v0 for weapon[0]'s value (clobbering the
 *    already-tested weapon[3] value and forcing ITS reload into a fresh
 *    register for the swap), matching the target's `lw`+reload shape.
 *    CASE_B needs the opposite: a PLAIN, temp-free inline read at the
 *    store site (`human->weapon[0] = weapons[2];`, no `stowed`) — adding a
 *    symmetric temp/chain here (the naive mirror of CASE_A's fix)
 *    regressed the WHOLE function (even un-did CASE_A's own fix and the
 *    unrelated `count`/`weapons` register choice), since global-alloc's
 *    priority ordering is whole-function, not per-branch. Found via
 *    `tools/permute.py --stop-on-zero` (one ~2 min run, score 0 on the
 *    first `--stop-on-zero` hit) after the plain temp-per-branch draft
 *    landed on a 20-byte pure register-coloring residual that neither
 *    statement order nor declaration order moved; the winning candidate
 *    also had two dead `if (!weapons) {}` / bare `;` no-ops that bisection
 *    showed were NOT load-bearing (removed here).
 *  - `if (count == efrm || efrm == MOTION_FRAME_ANY)` keeps Ghidra's
 *    literal polarity (De-Morgan lever: an `||`'s THEN body is reached by
 *    the first disjunct's taken branch OR the second disjunct's fallthrough
 *    — already the asm's shape, no inversion needed here, unlike a plain
 *    single-condition if/else).
 *  - `seid` is plain `s32` (not `s16`): it's only ever a call argument
 *    (never stored/compared), and the asm materializes it with a full-word
 *    `addiu`/`addu` from zero, not a halfword store — no narrowing.
 */

extern Humanoid *Me_MOTION_C;

void AttackPQD(s16 sfrm, s16 efrm)
{
    Humanoid *human;
    s16 count;
    OrnamentType **weapons;
    OrnamentType *held;
    OrnamentType *stowed;
    s32 seid;

    human = Me_MOTION_C;
    count = dtM->count;
    weapons = human->weapon;
    if (count == efrm || efrm == MOTION_FRAME_ANY)
    {
        if (weapons[WEAPON_SLOT_INACTIVE_1] == 0)
            return;
        seid = CHAR_SE_WEAPON_CHANGE_B;
        held = (weapons[WEAPON_SLOT_INACTIVE_0] = human->weapon[WEAPON_SLOT_ACTIVE_0]);
        stowed = weapons[WEAPON_SLOT_INACTIVE_1];
        human->weapon[WEAPON_SLOT_ACTIVE_0] = stowed;
        weapons[WEAPON_SLOT_INACTIVE_1] = 0;
    }
    else
    {
        if (count != sfrm)
            return;
        if (weapons[WEAPON_SLOT_INACTIVE_0] == 0)
            return;
        seid = CHAR_SE_WEAPON_CHANGE_A;
        held = human->weapon[WEAPON_SLOT_ACTIVE_0];
        weapons[WEAPON_SLOT_INACTIVE_1] = held;
        human->weapon[WEAPON_SLOT_ACTIVE_0] = weapons[WEAPON_SLOT_INACTIVE_0];
        weapons[WEAPON_SLOT_INACTIVE_0] = 0;
    }
    Sound(human, seid);
}
