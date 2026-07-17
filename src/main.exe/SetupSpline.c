#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetupSpline(struct MotionManager *mmp);
 *     ACTION.C:344, 17 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $a0       struct MotionManager * mmp
 * END PSX.SYM */

/*
 * SetupSpline (0x8001c640, 0xf0 bytes) — seeds the (mmp->n + 1)
 * SplineControlType blocks SetupMotionManager allocated at mmp->control:
 * block 0 brackets the motion's root `locate` keyframe, blocks 1..n bracket
 * each of the n `rotate[]` keyframes. Each block gets key0 = the keyframe,
 * dd0.pad = the shared time delta, and — only when time != 0 — key1 = the
 * NEXT keyframe plus a call to UpdateSplineControl to compute the actual
 * derivative vectors.
 *
 * `iVar2 >> 0x10` indexing both `rotate[]` and the control-block pointer
 * (Ghidra's own `iVar5 * 0x10000; ... iVar2 >> 0x10`) is the short-loop-
 * counter idiom (cookbook Loops): the source counter is a plain `short i`,
 * not `int` — its own sign-extend fuses with the array-index scale, so a
 * for-loop over `short i` reproduces this without hand-rolling the shift.
 * mmp->control (void* in item.h — SetupMotionManager's allocation site
 * never needed the real type) is cast to SplineControlType* here, the first
 * first consumer of that field's true pointee. The `time`/`t` split (raw
 * short kept for the two `sh ...dd0.pad` stores, a separate `int`-ish copy
 * sign-extended once and reused for both `!= 0` zero-tests, matching the
 * "share one sign-extended pseudo" cookbook rule) recovers the target's
 * register-based `sll/sra` instead of a spurious `lh` reload.
 *
 * STATUS: NON_MATCHING — 12 of 240 bytes differ, with the exact target extent
 * (60 instructions), 0x28-byte frame, and physical inventory of four
 * conditional branches, two calls, and one return. The preceding checkpoint
 * differed by 83 bytes and scored 71.67%; this checkpoint scores 80.00% (the
 * earlier draft was 149/240 and 62.07%).
 *
 * Keeping `t` as `int` makes cc1 emit the target's single shared sign-extension
 * pair at entry and retain that value for both zero-tests. At both
 * UpdateSplineControl call sites, assigning the identical-arm call alias back
 * through `spc` before the key1 store is a semantic no-op, but preserves the
 * target's call-copy/carrier instruction shape. That in turn lets the loop
 * offset remain the correct `s32`; no narrowed type is needed to force length.
 *
 * All 12 residual bytes are register fields in six allocator hunks: retail
 * colors the call-site control pointer/key as a1/a0, while this source chooses
 * a0/a1 and the coupled v0/v1 carriers in the opposite order. There are no
 * remaining opcode, immediate, extent, or CFG differences. A bounded guided
 * exact search and 23,004 source permutations both plateaued at this coloring;
 * ~~a further bounded permuter run (--stop-on-zero -j4) also plateaued at 12~~ —
 * **VOID (2026-07-17): THAT RUN NEVER HAPPENED.** The permuter cannot search this
 * function at all. Its own compile of its REWRITTEN source fails —
 * `stdin:941: conversion to non-scalar type requested` / `Unable to compile
 * .shake/permuter/SetupSpline/base.c` — so it evaluates ZERO candidates and exits,
 * and permute.py then printed `authoritative best -> base.c (12 ...)`, which reads
 * exactly like "searched and found nothing". It did not plateau; it crashed.
 * permute.py now REFUSES loudly on this (b4b7742), so the mistake cannot recur.
 *
 * Note the sanity compile could never have caught it: OUR base.c builds fine (the
 * draft builds). The permuter PARSES and REGENERATES that C, and its regenerated
 * source is what will not compile — a decomp-permuter/pycparser limitation, not a
 * defect in this draft. Suspects in this body: the cpp-expanded struct definitions,
 * the nested `spc->dd0.pad`, or the `(spc = call_spc)->key1` assignment-expression
 * deref.
 *
 * SO THIS PARK IS NOT EVIDENCE-BACKED ON THE SEARCH AXIS. 12 bytes across 12 insns,
 * every one a register field (3 clusters, all `operands`) is a PURE register
 * permutation — the permuter's ideal shape, and it has never actually run here.
 * TOOL TICKET: make the permuter able to parse this function (or find the construct
 * and spell it differently), then give it a real bounded run before anyone calls
 * this residual sub-C. Surveyed 2026-07-17: AddEnemy, FUN_800514d8, AdtSelect,
 * StageEndScreen, mission_score_screen, PlayVoice, SetLightningI and SetWire all
 * search fine, so this is not a widespread outage — but it is a real one here.
 *
 * tools/autorules.py reports no improving edit among 12 candidates.
 *
 * MEASURED EVIDENCE (do not re-derive; every number below was measured here):
 *
 * 1. reghist's "delta sum ZERO = pure rename" verdict is MISLEADING for this
 *    function. The instruction COUNTS match, but the four copies are NOT the
 *    target's copies. Retail spells them (per site) `k = key` -> `move v0,<key>`
 *    and `p = spc` -> `move a0,a1`, with `k + 1` coalescing back into k's own
 *    register (`addiu v0,v0,8`). This draft instead emits a pointer round-trip
 *    (`move v0,a0` / `move a0,v0`) plus a direct `addiu v0,v1,8`. Same count,
 *    different identity — so a zero-sum register histogram cannot distinguish a
 *    rename from a same-count/different-identity copy structure.
 *
 * 2. The NATURAL source (no alias scaffolding at all: `spc->key1 = key + 1;
 *    UpdateSplineControl(spc);` at both sites) compiles to 224 bytes — exactly
 *    four copies short of retail's 240 — and colors spc=a0 / loop_key=a1: the
 *    SAME a0/a1 swap as this draft. The swap is therefore NOT caused by the
 *    scaffolding, and the four missing copies are precisely the copies that
 *    would exist if spc lived in a1.
 *
 * 3. The demo build corroborates the shape, not the coloring:
 *    `tools/siblingdiff.py SetupSpline --demo` aligns 43/60 instructions and
 *    shows the demo ALSO emits `move v0,v1` + `addiu v0,v0,8` at site 1. Demo
 *    (232) differs from retail (240) by exactly the two `move a0,a1` — which
 *    exist only because retail puts spc in a1. Demo's site 2 has no copies at
 *    all because there loop_key is already v0 and spc already a0, so both
 *    copies degenerate to `move x,x` and are deleted.
 *
 * 4. Why spc takes a0 (the blocker). `.greg` reports `85 preferences: 4` for
 *    spc, but the preference is not the cause: routing the call through a
 *    separate alias (`UpdateSplineControl(call_spc)`) moves the a0 preference
 *    onto the alias, and spc STILL takes a0 (that variant measures 232). spc
 *    wins a0 by PRIORITY — regalloc.py gives spc 29 refs/26 live -> 44615 vs
 *    loop_key 10/10 -> 30000 — with a0 simply the first free register in the
 *    scan. So a0 must be BLOCKED by a conflicting allocno colored earlier.
 *
 * 5. Why the brief's "split the shared pseudo per site" lever cannot apply
 *    here. Splitting spc into per-site variables demotes it, but site 1's half
 *    does not conflict with loop_key (disjoint regions), so a0 stays free and
 *    the half takes a0 — while retail needs a1 at site 1 too (`lw a1,24(s1)`,
 *    `sw v1,0(a1)`, `sh s3,14(a1)`). Demoting spc can only fix site 2.
 *
 * 6. Why the alias cannot supply the blocking conflict. For the copy `p = spc`
 *    to survive at all, p and spc must CONFLICT; otherwise they share one hard
 *    register and the copy is deleted as `move a0,a0`. Hoisting the alias to
 *    function scope to raise its priority measures 224 and `.greg` shows
 *    exactly this collapse: `85 in 4` and `86 in 4`, with 85 absent from 86's
 *    conflict list. Nothing in this function reads spc after the alias
 *    assignment, so no C spelling of the alias creates the conflict that would
 *    push spc off a0. That is the specific, measured reason this is parked.
 */
extern void UpdateSplineControl(SplineControlType *spc);

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupSpline", SetupSpline);
#else /* NON_MATCHING */
void SetupSpline(MotionManager *mmp)
{
    short time;
    int t;
    short i;
    MotionElementType *key;
    SplineControlType *spc;

    time = mmp->motion->time;
    key = mmp->motion->locate;
    spc = (SplineControlType *)mmp->control;
    spc->key0 = key;
    t = time;
    spc->dd0.pad = time;
    if (t != 0) {
        SplineControlType *call_spc;

        if (key != 0)
            call_spc = spc;
        else
            call_spc = spc;
        (spc = call_spc)->key1 = key + 1;
        UpdateSplineControl(spc);
    }
    for (i = 0; i < mmp->n; i++) {
        MotionElementType *loop_key;
        s32 control_offset;

        control_offset = i * sizeof(SplineControlType) + sizeof(SplineControlType);
        loop_key = mmp->motion->rotate[i];
        spc = (SplineControlType *)((u8 *)mmp->control + control_offset);
        spc->dd0.pad = time;
        spc->key0 = loop_key;
        do {
            if (t != 0) {
                SplineControlType *call_spc;

                if (loop_key != 0)
                    call_spc = spc;
                else
                    call_spc = spc;
                (spc = call_spc)->key1 = loop_key + 1;
                UpdateSplineControl(spc);
            }
        } while (0);
    }
}
#endif /* NON_MATCHING */
