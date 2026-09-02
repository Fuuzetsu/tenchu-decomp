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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * mmp->control carries the SplineControlType pointer allocated by
 * SetupMotionManager. The signed `time` halfword is shared directly by both
 * control records and both zero tests.
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
    short i;
    SplineControlType *spc;

    time = mmp->motion->time;
    spc = mmp->control;
    spc->key0 = mmp->motion->locate;
    spc->dd0.pad = time;
    if (time != 0)
    {
        spc->key1 = spc->key0 + 1;
        UpdateSplineControl(spc);
    }
    for (i = 0; i < mmp->n; i++)
    {
        spc = &mmp->control[i + 1];
        spc->key0 = mmp->motion->rotate[i];
        spc->dd0.pad = time;
        if (time != 0)
        {
            spc->key1 = spc->key0 + 1;
            UpdateSplineControl(spc);
        }
    }
}
