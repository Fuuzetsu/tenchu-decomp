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
 * exact search and 23,004 source permutations both plateaued at this coloring.
 * The demo's shorter 232-byte implementation passes the control pointer
 * directly in a0, corroborating that retail's extra copies are an allocator/
 * source-identity distinction rather than different pointer arithmetic.
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
