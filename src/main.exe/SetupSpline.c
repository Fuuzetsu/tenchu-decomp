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
 * STATUS: NON_MATCHING — 149 of 240 bytes differ. Length is otherwise
 * exactly right (60 instructions both sides) EXCEPT for one still-missing
 * 2-instruction pattern that recurs at BOTH call sites (the non-loop block
 * and the loop body): before `UpdateSplineControl(spc)`, the target does
 * `move v0,key_reg; move a0,spc_reg; addiu v0,v0,8` (copy-then-add) where
 * this draft's cc1 fuses key+1 straight into `addiu v0,key_reg,8` (one
 * instruction, semantically identical, 2 shorter) because `spc` already
 * colors into $a0 from its very first use (the freed entry-argument
 * register) instead of $a1 the way the target's allocation does — so no
 * `move a0,a1` call-setup copy is ever needed, and the paired `move v0,v1`
 * doesn't appear either. Tried and falsified: reordering every statement
 * that touches `time`/`key`/`spc` relative to each other (4 orderings),
 * reassigning `key = key + 1` in place instead of `key + 1` inline, an
 * explicit `MotionDataType *motion = mmp->motion;` temp, a `do{}while(0)`
 * wrapper around the call block, and autorules' full local-type sweep (its
 * one non-reverted win, `t: int->s16`, is kept — its OTHER suggested win,
 * `time: short->s8`, was REJECTED per the cookbook's "don't narrow a proven
 * s16 field's access width" rule: it turns the target's `lhu` into a wrong
 * `lbu`). Needs a decomp.me/psyq4.3 session or a fresh idea for what forces
 * `spc` into $a1 (not $a0) from the start; not a register-swap tie the
 * permuter would crack (this is 4 whole instructions short, not same-length).
 */
extern void UpdateSplineControl(SplineControlType *spc);

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SetupSpline", SetupSpline);
#else /* NON_MATCHING */
void SetupSpline(MotionManager *mmp)
{
    short time;
    s16 t;
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
        spc->key1 = key + 1;
        UpdateSplineControl(spc);
    }
    for (i = 0; i < mmp->n; i++) {
        key = mmp->motion->rotate[i];
        spc = (SplineControlType *)mmp->control + i + 1;
        spc->dd0.pad = time;
        spc->key0 = key;
        if (t != 0) {
            spc->key1 = key + 1;
            UpdateSplineControl(spc);
        }
    }
}
#endif /* NON_MATCHING */
