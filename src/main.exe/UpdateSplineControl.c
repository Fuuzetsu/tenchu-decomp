#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateSplineControl(struct SplineControlType *spc);
 *     ACTION.C:365, 19 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t0       struct SplineControlType * spc
 *     reg   $a2       struct MotionElementType * key0p
 *     reg   $a3       struct MotionElementType * key1n
 * END PSX.SYM */

void UpdateSplineControl(SplineControlType *spc)
{
    MotionElementType *key0p;
    MotionElementType *key1n;
    s32 dt;
    s32 slope1;
    s32 slope2;

    key0p = spc->key0;
    if (spc->key0->time != 0)
    {
        key0p--;
    }
    key1n = spc->key1;
    if (spc->key1->time < spc->dd0.pad)
    {
        key1n++;
    }
    {
        s16 diff;

        diff = spc->key1->time - spc->key0->time;
        dt = (s8)diff << 8;
    }
    slope1 = (s16)(dt / (spc->key1->time - key0p->time));
    slope2 = (s16)(dt / (key1n->time - spc->key0->time));
    spc->dd0.vx = (s16)(slope1 * (spc->key1->x - key0p->x) >> 8);
    spc->dd0.vy = (s16)(slope1 * (spc->key1->y - key0p->y) >> 8);
    spc->dd0.vz = (s16)(slope1 * (spc->key1->z - key0p->z) >> 8);
    spc->ds1.vx = (s16)(slope2 * (key1n->x - spc->key0->x) >> 8);
    spc->ds1.vy = (s16)(slope2 * (key1n->y - spc->key0->y) >> 8);
    spc->ds1.vz = (s16)(slope2 * (key1n->z - spc->key0->z) >> 8);
}
