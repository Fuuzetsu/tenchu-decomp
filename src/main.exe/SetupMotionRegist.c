#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct MotionRegistType * SetupMotionRegist(struct MotionRegistType *mrp);
 *     ACTION.C:126, 9 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct MotionRegistType * mrp
 * END PSX.SYM */

/*
 * SetupMotionRegist (0x8001c3b0) — resolve each MotionRegistType row's `id`
 * to a MotionDataType pointer via SearchMotion, stopping at the sentinel
 * row (mid == MOTION_ID_NONE). game_types.h's MotionRegistType
 * (mid/id/motion, 8 bytes) is proven by
 * SetupMotionManager.c/PlayMotion.c/SetNowMotion.c; the *8
 * per-row stride here shows up as `(i << 16) >> 13` (sign-extend-then-
 * scale-by-8 folded into one shift amount).
 */
extern MotionDataType *SearchMotion(s16 id);

MotionRegistType *SetupMotionRegist(MotionRegistType *mrp)
{
    short i;

    i = 0;
    while (mrp[i].mid != MOTION_ID_NONE)
    {
        mrp[i].motion = SearchMotion(mrp[i].id);
        i++;
    }
    return mrp;
}
