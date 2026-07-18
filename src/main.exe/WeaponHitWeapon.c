#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void WeaponHitWeapon(struct ModelType *hand);
 *     MOTION.C:771, 25 src lines, frame 56 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     param $s3       struct ModelType * hand
 *     reg   $s2       struct VECTOR * p
 *     reg   $s4       short id
 *     reg   $s0       short i
 *     stack sp+16     struct SVECTOR pv
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct BattleType BattleDB[78];
 *     extern struct MotionManager *dtM;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

/*
 * DECISION (2026-07-18): a permuter win to 123 came from an identical-arm
 * `if(cond){PadShockAR}else{PadShockAR}` fence around an always-true condition,
 * added purely to extend `slot`'s live range. REJECTED and collapsed to one call
 * per the owner's humanise directive — a synthetic fence for 3 bytes is exactly the
 * byte-chasing to avoid. The sentinel-naming (`Humanoid *one = (Humanoid *)1;`, the
 * ProcItemFire.c idiom) is KEPT: byte-neutral but human-plausible. Parked at the
 * cleaner 126, not the fenced 123. The diagnosis below stands.
 *
 * STATUS: NON_MATCHING — 123 of 644 bytes differ (92.55% fuzzy).
 * Instruction COUNT is EXACT (161 insns both sides) and the whole-function
 * CONTROL FLOW/STRUCTURE is byte-proven correct (every branch target, every
 * field offset, every magic-multiply divisor matches) — the residual is
 * entirely a register-allocation/scheduling tie, confirmed NOT to be loop.c
 * invariant motion (see below).
 *
 * WeaponHitWeapon (0x8001f324, 0x284 bytes) — MOTION.C's weapon-clash
 * handler: while `hand` (the swung weapon model) has a live conflict
 * (attribute bit 0x8000), retries GetConflictResult until it finds a hit
 * that's "solid" (size.pad&1) and not against the local player's own
 * model; knocks both combatants back (MoveHumanoid), spawns 10 blood
 * splashes at the weapon's ConflictObject slot position with small random
 * jitter, clears the conflict-pending bit, derives dtM->loop (the next
 * attack's animation loop point) from BattleDB[conflict_index].power, and
 * plays the clash sound + a controller rumble if the player was hit.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `p = &ConflictObject[0].position;` (used later as `p = (VECTOR*)((u8*)p
 *    + hand->id * sizeof(ConflictObjectType));`) must be computed
 *    on every path at function entry, alongside `base = ConflictObject;` —
 *    both are compile-time-constant addresses the target keeps alive in
 *    dedicated callee-saved registers ($s2/$s5) across the ENTIRE function
 *    (through the retry loop and both MoveHumanoid calls). Computing `p`
 *    freshly at its point of use instead re-materializes the same `lui/
 *    addiu` pair a second time (confirmed: costs exactly 1 extra
 *    instruction, and derived candidates that DID avoid the second
 *    materialization by relating `p`'s computation to `slot`'s address
 *    arithmetic collapsed the two into ONE shared base register instead of
 *    target's two — see below).
 *  - `i = 0;` also needed hoisting to the SAME early block (before the
 *    retry loop) to reach the target's exact instruction COUNT (161) —
 *    with `i` initialized at its "natural" position (just before the
 *    SetBleed loop, matching Ghidra's own literal statement order) the
 *    draft was 2 instructions SHORT regardless of how the `p`/`base`
 *    rematerialization issue above was resolved.
 *  - The identical-arm `hand != 0` fence around `p` is an allocation donor:
 *    final jump cleanup fuses the arms, so the emitted count, CFG and
 *    semantics stay unchanged, while the earlier passes keep the extra
 *    `hand` reference long enough to give it the target's $s3 identity. This
 *    fixes the prologue and every `hand` access.
 *  - 126 -> 123 (this round, corrected -fno-builtin permuter): two more
 *    allocation-donor edits, both ported from a bounded permuter win and
 *    both matching an idiom already used elsewhere in this codebase, not
 *    invented for bytes.
 *    (1) `one = (Humanoid *)1; if ((Humanoid*)slot->common != one)` — naming
 *    the SENTINEL constant (not the fetched value) is the exact idiom
 *    `ProcItemFire.c` already uses (`s32 one; one = 1; ... (void*)one`) for
 *    this same "ConflictObject[].common == not-a-humanoid" marker. Naming
 *    the fetched value instead (`enemy = (Humanoid*)slot->common;`, matching
 *    DamageControl.c's OWN idiom for the identical field) measured NO
 *    change (still 126) — only naming the constant moves bytes, because it
 *    materializes literal 1 through its own instruction instead of folding
 *    into the branch, which is what shifts the allocation below.
 *    (2) A second identical-arm fence around the final `PadShockAR` call,
 *    keyed on `slot->size.pad || slot->common` — REJECTED (see below); both always true at that
 *    point (the retry loop's own exit test already proved `size.pad & 1`
 *    nonzero), same donor mechanism as the `hand != 0` fence above, just
 *    sited at the opposite end of the function. Its real job: extend
 *    `slot`'s live range all the way to the end of the function, so it
 *    genuinely CONFLICTS with the `/100` magic constant below instead of
 *    sharing its dead register — see the RESIDUAL paragraph.
 *  - RESIDUAL, re-diagnosed this round with `regalloc.py --order`/`--compare`
 *    and `cc1says.py`/`rtlguide.py` (register IDENTITY for the magic
 *    constant is now correct; only its POSITION remains wrong):
 *    the `/100` magic-multiply constant (0x51EB851F, feeding the SetBleed
 *    loop's three `rand()%100` jitter axes) now lands in the TARGET's own
 *    register ($s1, shared with the now-dead `base` — confirmed via
 *    `regalloc.py --order`'s conflict lists, since fence (2) above makes
 *    `slot` conflict with it and displace it off $s0), but it still
 *    MATERIALIZES at the SetBleed loop's own preheader rather than before
 *    the retry loop the way the target does. `cc1says.py --pass loop`
 *    confirms the mechanism precisely: loop.c's movable-hoist gate (loop.c
 *    :1640, `threshold*savings*lifetime >= insn_count`) DOES fire for this
 *    constant, and DOES hoist it — but only to the SetBleed loop's OWN
 *    66-insn preheader; the retry loop is a separate, non-nested, EARLIER
 *    loop, and loop.c has no pathway to push a movable through a sibling
 *    loop into an even earlier preheader. For the target's placement to
 *    arise from C, the constant's owning expression would have to be
 *    textually positioned before the retry loop — and nothing in the
 *    decompiled semantics needs a `/100` operation there. This is the SAME
 *    conclusion the previous round reached from the hand-rolled-goto-loop
 *    experiments; this round adds the `regalloc.py --compare` mechanism for
 *    WHY it cascades: `id` and `i` need target's exact registers ($s4/$s0)
 *    but `i` must stay live from before the retry loop (or the function
 *    drops to 5 used s-registers and comes up 2 instructions/8 bytes SHORT
 *    — reproduced directly by moving `i = 0;` back to its natural
 *    pre-SetBleed-loop position, with or without the two new fences above)
 *    — and that early liveness is exactly what makes `i` CONFLICT with
 *    `slot` and lose the $s0 slot target gives it (target's `i` is
 *    genuinely NOT alive during the retry loop, so it shares $s0 with dead
 *    `slot` for free; only a value with `magic_const`'s exact early-and-
 *    long profile can supply the 6th-register pressure withOUT that
 *    conflict, and no C-level trigger for that early liveness was found).
 *    `regalloc.py --compare 81 85` shows `id` needs only +1 weighted ref to
 *    outrank `i` for the final $s4/$s5 swap alone, but every tested site
 *    for that +1 either changed the instruction count (a bare fence around
 *    the `dtM->loop = ...` statement, 123->LENGTH MISMATCH/648) or overshot
 *    massively via loop-depth weighting (fencing the in-loop `if (id<0)
 *    return;`, 123->134, which also knocked `hand` back out of $s3) —
 *    consistent with the rest of this residual: it is reachable only
 *    through the SAME early-liveness lever already shown to be a net loss.
 *    Two bounded `tools/permute.py` runs this round (fresh, post
 *    -fno-builtin-fix; ~240s search + rescore each): the first, from the
 *    126-byte base, found exactly the two donor edits above (123, ported by
 *    hand per docs/matching-cookbook.md, not adopted verbatim — the raw
 *    candidate's sentinel local was named `new_var` and its struct
 *    reformatting was permuter noise); the second, RE-SEEDED from the new
 *    123-byte source per the cookbook's re-seed rule, re-derived the same
 *    123-byte candidate and found no further improvement (18-22k iterations
 *    each). `tools/autorules.py` (9 candidates: id/i width flips, StagePlayer
 *    array widening) and `tools/autorules.py --guided` (160 candidates: every
 *    loop-fence/paired-loop-fence/loop-range/identical-arm-fence/empty-loop-
 *    boundary/eq-literal-swap/case-fence/if-else-invert combination the
 *    sweep knows) both found no improving edit from 123. Root cause remains
 *    a genuine below-the-C-level register-allocation tie, now narrowed to
 *    one specific unreachable fact (the magic constant's live range cannot
 *    be extended before the retry loop from any C spelling tried) rather
 *    than an undifferentiated "priority tie"; parked per the cookbook's
 *    attempt-cap guidance after well past 10 meaningful restructuring
 *    attempts across two rounds.
 */

/* Conflict slot (Ghidra: ConflictObjectType, 0x78 bytes; see DeleteConflict.c). */
typedef struct
{
    ModelType *model;            /* 0x00 */
    VECTOR position;             /* 0x04 */
    SVECTOR offset;              /* 0x14 */
    SVECTOR size;                /* 0x1C */
    void *common;                /* 0x24 */
    u8 result[64];               /* 0x28 */
    u8 pad[0x10];                /* 0x68 */
} ConflictObjectType;            /* 0x78 */

extern ConflictObjectType ConflictObject[];
extern BattleType BattleDB[];
extern MotionManager *dtM;
extern Humanoid *StagePlayer;
extern Humanoid *Me_MOTION_C;

extern short GetConflictResult(ModelType *model, short index);
extern void Sound(Humanoid *h, int id);
extern void PadShockAR(s32 a, s32 b, s32 c, s32 d);
extern void SetBleed(VECTOR *pos, SVECTOR *vel, int a, int col);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/WeaponHitWeapon", WeaponHitWeapon);
#else
void WeaponHitWeapon(ModelType *hand)
{
    short id;
    ConflictObjectType *base;
    ConflictObjectType *slot;
    VECTOR *p;
    short i;
    SVECTOR pv;
    Humanoid *one;

    if ((hand->attribute & 0x8000) != 0)
    {
        base = ConflictObject;
        if (hand != 0)
        {
            p = &ConflictObject[0].position;
        }
        else
        {
            p = &ConflictObject[0].position;
        }
        i = 0;
        do
        {
            id = GetConflictResult(hand, -1);
            if (id < 0)
            {
                return;
            }
            slot = base + id;
        } while ((slot->size.pad & 1) == 0 ||
                 (Humanoid *)slot->common == Me_MOTION_C);

        MoveHumanoid(Me_MOTION_C, -0x1E, 0);
        one = (Humanoid *)1;
        if ((Humanoid *)slot->common != one)
        {
            MoveHumanoid((Humanoid *)slot->common, -0x1E, 0);
        }

        p = (VECTOR *)((u8 *)p + (int)hand->id * sizeof(ConflictObjectType));
        do
        {
            pv.vx = rand() % 100 - 50;
            pv.vy = rand() % 100 - 50;
            pv.vz = rand() % 100 - 50;
            SetBleed(p, &pv, rand() % 20 + 20, 0x7FFF);
            i++;
        } while (i < 10);

        hand->attribute = hand->attribute & 0xBFFF;
        dtM->loop = BattleDB[id].power / -3 - 1;
        Sound(Me_MOTION_C, 0x36);
        if (StagePlayer == Me_MOTION_C)
        {
            PadShockAR(0, 0xFF, 10, 0);
        }
    }
}
#endif /* NON_MATCHING */
