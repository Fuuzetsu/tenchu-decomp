#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * SetupMotionRegist (0x8001c3b0) — resolve each MotionRegistType row's `id`
 * to a MotionDataType pointer via SearchMotion, stopping at the sentinel
 * row (mid == -1). item.h's MotionRegistType (mid/id/motion, 8 bytes) is
 * proven by SetupMotionManager.c/PlayMotion.c/SetNowMotion.c; the *8
 * per-row stride here shows up as `(i << 16) >> 13` (sign-extend-then-
 * scale-by-8 folded into one shift amount).
 */
extern MotionDataType *SearchMotion(s16 id);

MotionRegistType *SetupMotionRegist(MotionRegistType *mrp)
{
    short i;

    i = 0;
    while (mrp[i].mid != -1) {
        mrp[i].motion = SearchMotion(mrp[i].id);
        i = i + 1;
    }
    return mrp;
}
