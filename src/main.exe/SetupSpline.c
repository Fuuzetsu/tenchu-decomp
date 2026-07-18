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
 * consumer of that field's true pointee. The `time`/`t` split (raw
 * short kept for the two `sh ...dd0.pad` stores, a separate `int`-ish copy
 * sign-extended once and reused for both `!= 0` zero-tests, matching the
 * "share one sign-extended pseudo" cookbook rule) recovers the target's
 * register-based `sll/sra` instead of a spurious `lh` reload.
 *
 * The final register allocation comes from the source's data identity, not an
 * alias fence: write each `key0` directly from `locate`/`rotate[i]`, derive
 * `key1` from the field just written, and put the loop's key0 store before its
 * `dd0.pad` store. GCC CSEs the field load while retaining the distinct next-key
 * pseudo; the direct loop assignment occupies a0 and naturally colors `spc` a1.
 * The demo PSX.SYM address-to-line records independently show these statement
 * boundaries (ACTION.C:349-358). This matches all 240 retail bytes.
 */
extern void UpdateSplineControl(SplineControlType *spc);

void SetupSpline(MotionManager *mmp)
{
    short time;
    int t;
    short i;
    SplineControlType *spc;

    time = mmp->motion->time;
    spc = (SplineControlType *)mmp->control;
    spc->key0 = mmp->motion->locate;
    t = time;
    spc->dd0.pad = time;
    if (t != 0) {
        spc->key1 = spc->key0 + 1;
        UpdateSplineControl(spc);
    }
    for (i = 0; i < mmp->n; i++) {
        s32 control_offset;

        control_offset = i * sizeof(SplineControlType) + sizeof(SplineControlType);
        spc = (SplineControlType *)((u8 *)mmp->control + control_offset);
        spc->key0 = mmp->motion->rotate[i];
        spc->dd0.pad = time;
        do {
            if (t != 0) {
                spc->key1 = spc->key0 + 1;
                UpdateSplineControl(spc);
            }
        } while (0);
    }
}
