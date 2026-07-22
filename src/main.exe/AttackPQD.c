#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackPQD(short sfrm, short efrm);
 *     MOTION.C:849, 23 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     param $a0       short sfrm
 *     param $a1       short efrm
 * END PSX.SYM */

/*
 * AttackPQD (0x80027688, 0xa8 bytes) — weapon holster/draw swap: on a
 * matching motion-frame trigger (dtM->count, MotionManager's proven
 * `count` field) or a wildcard trigger (efrm == -1), draws the holstered
 * weapon at weapon[3] into weapon[0] (the active slot) and clears
 * weapon[3]; on the OTHER trigger frame (dtM->count == sfrm), holsters the
 * active weapon[0] into weapon[2] instead. Either swap plays a sound
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
 *  - `ppOVar1 = human->weapon;` (the array's base, `human+0x94`) is ALSO
 *    hoisted unconditionally to the top (its `addiu` sits in the FIRST
 *    branch's delay slot — computed before either arm is chosen). BOTH
 *    weapon[2] AND weapon[3] are reached exclusively through `ppOVar1`
 *    (`ppOVar1[2]`/`ppOVar1[3]`) in EVERY branch, never through
 *    `human->weapon[2/3]` directly, even in the branch where that index is
 *    the "own" one being tested — only weapon[0] (offset 0) stays on
 *    `human` directly (folds the 0x94 displacement into human's own
 *    register). Trust the register the raw asm actually uses, not
 *    Ghidra's inconsistent per-statement naming of the identical address.
 *  - Register-allocation tie (permuter, bisected — see below): CASE_A's
 *    "own slot" store must be a CHAINED assignment,
 *    `t0 = (ppOVar1[2] = human->weapon[0]);`, not the seemingly-equivalent
 *    `t0 = human->weapon[0]; ppOVar1[2] = t0;` — the chained form is what
 *    makes cc1 reuse $v0 for weapon[0]'s value (clobbering the
 *    already-tested weapon[3] value and forcing ITS reload into a fresh
 *    register for the swap), matching the target's `lw`+reload shape.
 *    CASE_B needs the opposite: a PLAIN, temp-free inline read at the
 *    store site (`human->weapon[0] = ppOVar1[2];`, no `tOther`) — adding a
 *    symmetric temp/chain here (the naive mirror of CASE_A's fix)
 *    regressed the WHOLE function (even un-did CASE_A's own fix and the
 *    unrelated `count`/`ppOVar1` register choice), since global-alloc's
 *    priority ordering is whole-function, not per-branch. Found via
 *    `tools/permute.py --stop-on-zero` (one ~2 min run, score 0 on the
 *    first `--stop-on-zero` hit) after the plain temp-per-branch draft
 *    landed on a 20-byte pure register-coloring residual that neither
 *    statement order nor declaration order moved; the winning candidate
 *    also had two dead `if (!ppOVar1) {}` / bare `;` no-ops that bisection
 *    showed were NOT load-bearing (removed here).
 *  - `if (count == efrm || efrm == -1)` keeps Ghidra's literal polarity
 *    (De-Morgan lever: an `||`'s THEN body is reached by the first
 *    disjunct's taken branch OR the second disjunct's fallthrough —
 *    already the asm's shape, no inversion needed here, unlike a plain
 *    single-condition if/else).
 *  - `seid` is plain `s32` (not `s16`): it's only ever a call argument
 *    (never stored/compared), and the asm materializes it with a full-word
 *    `addiu`/`addu` from zero, not a halfword store — no narrowing.
 */

extern Humanoid *Me_MOTION_C;
extern MotionManager *dtM;
extern short Sound(Humanoid *human, int seid);

void AttackPQD(s16 sfrm, s16 efrm)
{
    Humanoid *human;
    s16 count;
    OrnamentType **ppOVar1;
    OrnamentType *t0;
    OrnamentType *tOther;
    s32 seid;

    human = Me_MOTION_C;
    count = dtM->count;
    ppOVar1 = human->weapon;
    if (count == efrm || efrm == -1)
    {
        if (ppOVar1[3] == 0)
            return;
        seid = 1;
        t0 = (ppOVar1[2] = human->weapon[0]);
        tOther = ppOVar1[3];
        human->weapon[0] = tOther;
        ppOVar1[3] = 0;
    }
    else
    {
        if (count != sfrm)
            return;
        if (ppOVar1[2] == 0)
            return;
        seid = 0;
        t0 = human->weapon[0];
        ppOVar1[3] = t0;
        human->weapon[0] = ppOVar1[2];
        ppOVar1[2] = 0;
    }
    Sound(human, seid);
}
