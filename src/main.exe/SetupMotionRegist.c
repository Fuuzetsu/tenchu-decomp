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
