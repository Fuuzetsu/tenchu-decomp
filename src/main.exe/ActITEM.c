#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActITEM(void);
 *     MOTION.C:1949, 36 src lines, frame 64 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     stack sp+16     struct PARAM_ITEM_LAUNCH item
 *     reg   $a1       short flag
 *     reg   $a0       short mode
 *     reg   $v0       struct VECTOR * p
 * END PSX.SYM */

/*
 * ActITEM (0x80026074) — trigger motion-timed item use and return to idle.
 *
 * STATUS: NON_MATCHING — 2 of 572 bytes differ (down from an earlier
 * checkpoint's 26; all three calls and physical CFG counts match). Build
 * with `NON_MATCHING=ActITEM ./Build`. On a full match, delete the guards
 * and the _jtbl array.
 *
 * The residual is ONE instruction at target 0x80026134: target leaves the
 * `beq item_type,4,<merge>` guard's delay slot as a bare `nop`; our draft
 * fills it with `move $a1,$0` (the `flag = 0;` below, hoisted into the
 * slot by reorg). Root-caused by reading gcc-2.8.1's own reorg.c
 * (fill_simple_delay_slots' backward scan, reorg.c:3064-3110): the scan
 * walks back from the branch past every insn that touches its operands
 * (v0/v1) and takes the first one that doesn't — `flag = 0;` is the only
 * such insn reachable in this block, so it is ALWAYS stolen, wherever it
 * is placed. Confirmed empirically across 9 source shapes (declaration
 * order, 4 different statement positions for `flag = 0;` including
 * inside only one arm of the StagePlayer check, a `do{}while(0)` fence —
 * regressed the allocation via ref-weighting, an explicit if/else
 * duplicating `flag = 1` — reintroduced eager delay-slot duplication
 * instead, and a De Morgan `&&` merge — cc1 folds it into a range check
 * cc1 never uses): every alternative either regressed or produced a
 * length mismatch; this is the best of the family. Two bounded permuter
 * runs (tools/permute.py, stop-on-zero) and three autorules sweeps found
 * nothing better; the second permuter run's own authoritative rescore
 * confirmed this exact source as the best of everything it tried.
 *
 * The other half of this fix: `flag`/`mode`/`count` (case 2) now land in
 * $a1/$a0/$a2 exactly like the target (previously coalesced $a1 into
 * $a0.) `flag = 0;` right after `item_type = mode;` is not a dead
 * statement — cc1 schedules it to execute while `mode`'s pseudo is still
 * live (immediately after the StagePlayer merge, per
 * `tools/rtldump.py ActITEM --trace <uid>` across all 15 passes), which
 * is what forces `flag` and `mode` into separate hard registers; deleting
 * it collapses them back onto one register (regresses to 26 bytes). See
 * docs/matching-cookbook.md, "TWO REGISTERS FOR ONE VALUE MEANS THE
 * ORIGINAL HAD TWO VARIABLES" and "gcc-2.8.1 has NO coalescing pass".
 *
 * RE-VERIFIED independently this round (the two permuter runs above
 * PRE-DATE the day's `-fno-builtin` fix — d02da20 landed ~4h after this
 * file's 26->2 commit 725ca61, so their negatives were formally void; every
 * conclusion below was re-derived, not assumed):
 *  - `tools/ccsrc.py fill_simple_delay_slots` (reorg.c:2975-3130) read
 *    directly: the backward scan (reorg.c:3083, `prev_nonnote_insn` until
 *    `stop_search_p`) tests only whether a trial SETS a resource the
 *    branch NEEDS (reorg.c:3098-3099); a trial that fails is folded into
 *    the accumulated state and the scan CONTINUES past it — failing the
 *    test does not stop the scan, only a CODE_LABEL/JUMP_INSN/BARRIER
 *    does. There is no `own_thread_p` label-ownership gate in this path
 *    (that's `fill_eager_delay_slots`, StickonCheck's mechanism, not this
 *    one) — confirms the header's citation exactly.
 *  - `tools/sched-deps.py ActITEM --block 8 --verdicts`: sched1 (runs
 *    BEFORE reorg) reorders this block's pre-sched chain `[sll, sra,
 *    flag=0, li-4]` into emitted `[flag=0, sll, sra, li-4]` — moves
 *    `flag = 0;`'s insn (uid 74) to the FRONT of the block, right after
 *    the StagePlayer-merge label. This looks load-bearing but isn't: the
 *    label blocks the backward scan regardless of `flag=0`'s position
 *    inside the block, because `sll`/`sra`/`li` all SET v0 or v1 (the
 *    branch's needed resources) and are skipped, not stopped, by the
 *    scan — `flag=0` is reachable from whichever of the 4 slots it lands
 *    in.
 *  - `tools/regalloc.py ActITEM --local`: self-validated, 0 divergences
 *    across all 45 printed homes — this is NOT a register-home tie
 *    (`flag` already sits in the correct hard register, $a1; the problem
 *    is that an instruction exists at that address at all).
 *  - Tried a 10th source shape, not clearly one of the "4 positions"
 *    above: `flag = 0;` as the FIRST statement of case 2, strictly before
 *    `mode = ITEM_FIRE;` — the one placement that puts it before the
 *    StagePlayer-merge label, in a different reorg-scan block. Measured
 *    directly (not just inferred): LENGTH MISMATCH, 576 vs 572 bytes
 *    (+4) — escaping the block this way costs more than it saves.
 *  - Fresh bounded permuter under the corrected flags (`timeout 240
 *    tools/permute.py ActITEM -- --stop-on-zero -j4`, 21,904 iterations):
 *    authoritative full-link rescore ranks base.c's 2 whole-image bytes
 *    as the best of everything it kept (next: 3, 3, 6, 12 — all worse;
 *    the rest are length-mismatched at 576 bytes). No score-0 candidate.
 *  - Fresh `tools/autorules.py ActITEM`: 12 candidates (type-width flips
 *    on item_type/flag/mode, extern-array retypes of StagePlayer/
 *    SelectedItem, an &&-merge/-split), none improving (`flag: s16->s32`
 *    regresses 2->1012).
 * Conclusion unchanged: 2/572 is the floor for this source structure under
 * this cc1 — a proven sub-C park (reorg.c's own backward-scan predicate
 * has no source-reachable escape here but leaving the block), not a
 * stalled search.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", ActITEM);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActITEM", switchD_800260b8__caseD_1);

/* jump-table pool @ 0x800115d8 (6 words; tables at 0x800115d8) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 handle_char_state_using_item__jtbl[6] = {
    0x800260C0, 0x800261A4, 0x800260E8, 0x8002614C,
    0x80026174, 0x80026174,
};

#else /* NON_MATCHING */

enum
{
    ITEM_MAKIBISHI = 2,
    ITEM_FIRE = 4,
    ITEM_SMOKE = 5,
    ITEM_JIRAI = 6,
    CMODE_NORMAL = 0
};

extern MotionManager *dtM;
extern Humanoid *Me_MOTION_C;
extern Humanoid *StagePlayer;
extern s16 SelectedItem;
extern s16 motID;
extern s16 motMODE;

extern void ReqItemUse(PARAM_ITEM_USE *item);
extern void SetCameraMode(s32 mode);

void ActITEM(void)
{
    PARAM_ITEM_USE item;
    VECTOR *p;
    s32 item_type;
    s16 flag;
    s16 mode;

    flag = 0;
    switch ((s16)(dtM->mid - 0xF00))
    {
    case 0:
        if (dtM->count != 10)
            break;
        flag = 1;
        item.type = ITEM_MAKIBISHI;
        break;

    case 2:
        if (dtM->count != 5)
            break;
        mode = ITEM_FIRE;
        if (Me_MOTION_C == StagePlayer)
            mode = SelectedItem;
        item_type = mode;
        flag = 0;
        if (item_type != ITEM_FIRE)
        {
            if (item_type != ITEM_SMOKE)
                break;
        }
        flag = 1;
        item.type = item_type;
        break;

    case 3:
        if (dtM->count != 5)
            break;
        flag = 1;
        item.type = ITEM_JIRAI;
        break;

    case 4:
    case 5:
        if (dtM->count != 0)
            return;
        if (dtM->loop == 0)
            return;
        dtM->loop = -1;
        return;
    }

    if (flag)
    {
        item.user = Me_MOTION_C;
        p = GetAbsolutePosition(Me_MOTION_C->model->object[2], 0, 0, 0);
        item.start.vx = p->vx;
        item.start.vy = p->vy;
        item.start.vz = p->vz;
        item.end = item.start;
        ReqItemUse(&item);
    }

    if (dtM->count == 0 && dtM->loop != 0)
    {
        if (Me_MOTION_C == StagePlayer)
            SetCameraMode(CMODE_NORMAL);
        if (Me_MOTION_C->attribute & 0x40)
        {
            motID = 0x501;
            motMODE = 1;
        }
        else
        {
            motID = 0;
            motMODE = 1;
        }
    }
}

#endif /* NON_MATCHING */
