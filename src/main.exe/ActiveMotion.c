#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ActiveMotion(struct MotionManager *mmp);
 *     ACTION.C:307, 31 src lines, frame 48 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s2       struct MotionManager * mmp
 *     reg   $s0       short i
 *     reg   $s3       short count
 *     reg   $s1       struct ModelType * object
 *     stack sp+16     struct SVECTOR vect
 * END PSX.SYM */

/*
 * ActiveMotion (0x8001bdfc, 0x1c4 bytes) — per-frame spline playback:
 * bail to HoldMotion for a static (time==0) pose, else bump `count` and, for
 * every masked bone, evaluate its GetSpline bracket at `count` (bone 0's
 * locate+rotate written specially onto the world matrix translation, every
 * other bone only rotate), then wrap `count` back to 0 once it passes the
 * motion's `time` limit.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `v` is a genuinely SEPARATE variable from `count`, not a renaming: `v =
 *    mmp->count;` computes the raw pre-increment value once (feeds both the
 *    `mmp->count = v + 1;` store and, later, `count = v;`), while `count`
 *    itself only needs a callee-saved home from the point it's actually
 *    read again. Funnelling both through ONE variable (`count = mmp->count;
 *    mmp->count = count + 1;`) makes cc1 compute the increment directly in
 *    count's own callee-saved register (no separate temp) — one instruction
 *    short of the target, which keeps the raw read in a caller-saved reg and
 *    copies it to $s3 only later. `i` is a THIRD, genuinely reused variable
 *    (cookbook Register allocation steering) — `i = v;` gives it the same
 *    value for the two bone-0 GetSpline calls, then it's clobbered as the
 *    `for (i = 1; ...)` loop counter; the loop's own GetSpline calls pass
 *    `count` directly, never `i`.
 *  - The function-final `mmp->count = 0; return 0;` MUST be an early
 *    `return 0;`, not `count = 0;` falling through to a single shared
 *    `return count;` — funnelling the tail through `count` forces an extra
 *    sign-extend-into-$v0 step at the shared return (the "shared return
 *    variable copy-preferences its sources" cookbook rule); two early
 *    returns let cc1 target $v0 directly on each path. Same lever fixes the
 *    OUTER dispatch: `if (time==0) { count = HoldMotion(mmp); return count;
 *    }` (an early return, not `else`) rather than falling through to the
 *    shared tail.
 *  - `mmp->control + (i + 1)` needs the EXPLICIT parens: `mmp->control + i +
 *    1` (left-to-right `(control+i)+1`) reads mmp->control BEFORE computing
 *    i's scale, while `+ (i + 1)` computes the `(i+1)*sizeof` scale first and
 *    defers the `mmp->control` field load until right before the final add
 *    — same value either way, different instruction order.
 */

extern short HoldMotion(MotionManager *mmp);
extern void GetSpline(SVECTOR *vect, SplineControlType *spc, short cnt);

short ActiveMotion(MotionManager *mmp)
{
    short i;
    short count;
    short v;
    ModelType *object;
    SVECTOR vect;

    if (mmp->motion->time == 0) {
        count = HoldMotion(mmp);
        return count;
    }
    v = mmp->count;
    mmp->count = v + 1;
    count = v;
    if (mmp->mask & 1) {
        object = *mmp->model->object;
        i = v;
        GetSpline(&vect, mmp->control, i);
        object->locate.coord.t[0] = (s32)vect.vx;
        object->locate.coord.t[2] = (s32)vect.vz;
        object->locate.coord.t[1] = (s32)mmp->model->rotate.pad * (s32)vect.vy >> 12;
        GetSpline(&object->rotate, mmp->control + 1, i);
        UpdateCoordinate(object);
    }
    for (i = 1; i < mmp->n; i++) {
        if ((mmp->mask >> i) & 1) {
            object = mmp->model->object[i];
            GetSpline(&object->rotate, mmp->control + (i + 1), count);
            UpdateCoordinate(object);
        }
    }
    count = mmp->count;
    if (mmp->motion->time < count) {
        mmp->count = 0;
        return 0;
    }
    return count;
}
