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
 * STATUS: NON_MATCHING — 126 of 644 bytes differ (90.68% fuzzy).
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
 *    fixes the prologue and every `hand` access, reducing the linked residual
 *    from 138 to 126 bytes and raising fuzzy similarity from 85.71% to 90.68%.
 *  - RESIDUAL: with the instruction count now exact, the remaining diff is
 *    a register-identity mismatch cascading from ONE root instruction: the
 *    `/100` magic-multiply constant (0x51EB851F, feeding the SetBleed
 *    loop's three `rand()%100` jitter axes) materializes in the target
 *    BEFORE the retry loop even begins (right after `base`/`p`'s own
 *    setup) but in every draft tried, only ever materializes at the
 *    SetBleed loop's OWN preheader (confirmed via `tools/rtldump.py
 *    --pass loop`, immediately before that loop's code_label rather than
 *    before the retry loop). This rules OUT loop.c
 *    invariant motion as the mechanism (loop.c only ever hoists to the
 *    OWNING loop's immediate preheader; there is no loop.c pathway to
 *    reach further back through an intervening, textually-earlier loop or
 *    straight-line calls) — tried and confirmed NO EFFECT: converting the
 *    retry section to a hand-rolled goto-loop (removes ITS OWN loop notes
 *    entirely — the SetBleed loop's own hoist point still didn't move),
 *    converting the SetBleed loop to a hand-rolled goto-loop too (the
 *    constant materializes even LATER, inline before its first use, since
 *    there's then no loop.c involvement at all), reordering the four early
 *    local initializations (`base`/`p`/`i`) in every declaration/statement
 *    permutation tried. In the current draft `hand` is now correct in $s3,
 *    but `base`/`p`/`id`/`i` occupy $s1/$s2/$s5/$s4 instead of the target's
 *    $s2/$s5/$s4/$s0; the late multiplier shares $s0 with the dead `slot`,
 *    whereas retail gives the multiplier $s1 and reuses $s0 for `slot`/`i`.
 *    Two bounded `tools/permute.py` runs (~300s each,
 *    `--stop-on-zero -j4`, ~18-29k iterations), the second AFTER the
 *    instruction count was already exact, plateaued at best scores 235 and
 *    425 respectively — never approaching 0. A fresh guided audit found no
 *    further improving path in a 160-candidate loop-fence beam or a
 *    220-candidate full beam; a one-shot outer range regressed 126 -> 135,
 *    explicit signed-64 remainder arithmetic grew to 704 bytes, and early
 *    literal donors grew to 648/660 bytes. Root cause is a genuine
 *    below-the-C-level register-allocation PRIORITY tie (which of several
 *    similarly-weighted long-lived pseudos gets the lowest-numbered
 *    callee-saved register first) that this session could not find a
 *    source-level lever for; parked per the cookbook's attempt-cap
 *    guidance after well past 10 meaningful restructuring attempts.
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
        if ((Humanoid *)slot->common != (Humanoid *)1)
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
