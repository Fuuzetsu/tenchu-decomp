#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short HoldMotion(struct MotionManager *mmp);
 *     ACTION.C:242, 26 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s1       struct MotionManager * mmp
 *     reg   $s2       struct MotionDataType * mot
 *     reg   $a1       struct ModelType * object
 *     reg   $s0       short i
 * END PSX.SYM */

/*
 * HoldMotion (0x8001b8d4, 0x150 bytes) — freeze-frame pose: latch the root
 * bone's locate keyframe straight onto object[0]'s world matrix translation
 * (mask bit 0), then for every other masked bone copy its rotate keyframe
 * onto that bone's ModelType.rotate and refresh its coordinate, before
 * disabling the manager (loop = -2, count = 0).
 *
 * Matching notes: `mot->locate` is re-read fresh at each of its three uses
 * (x/z/y) rather than cached in a pointer temp — every intervening `sw`
 * (unproven-alias to cc1's weak per-store analysis) forces the next read to
 * reload; same for `mmp->model` between the object[0]/rotate.pad reads.
 * rotate.vx/vy/vz load their s16 source fields with `lhu` (a same-width
 * short-to-short copy needs no sign extension — cookbook Expressions), while
 * the locate->x/y/z reads widen into the `long` matrix translation and so
 * load `lh`.
 */

short HoldMotion(MotionManager *mmp)
{
    MotionDataType *mot;
    ModelType *object;
    short i;

    mot = mmp->motion;
    if (mmp->mask & 1) {
        object = *mmp->model->object;
        object->locate.coord.t[0] = (s32)mot->locate->x;
        object->locate.coord.t[2] = (s32)mot->locate->z;
        object->locate.coord.t[1] = (s32)mmp->model->rotate.pad * (s32)mot->locate->y >> 12;
    }
    for (i = 0; i < mmp->n; i++) {
        if ((mmp->mask >> i) & 1) {
            object = mmp->model->object[i];
            object->rotate.vx = mot->rotate[i]->x;
            object->rotate.vy = mot->rotate[i]->y;
            object->rotate.vz = mot->rotate[i]->z;
            UpdateCoordinate(object);
        }
    }
    mmp->loop = -2;
    mmp->count = 0;
    return 0;
}
