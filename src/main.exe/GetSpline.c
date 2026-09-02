#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetSpline(struct SVECTOR *vect, struct SplineControlType *spc, short cnt);
 *     ACTION.C:388, 34 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       struct SVECTOR * vect
 *     param $s0       struct SplineControlType * spc
 *     param $s1       short cnt
 * END PSX.SYM */

extern void UpdateSplineControl(SplineControlType *spc);

void GetSpline(SVECTOR *vect, SplineControlType *spc, short cnt)
{
    MotionElementType *key;
    MotionElementType *next;

    key = spc->key1;
    if (key->time < cnt)
    {
        do
        {
            next = key + 1;
            spc->key1 = next;
            key = next;
        } while (next->time < cnt);
        spc->key0 = next - 1;
    }
    else
    {
        key = spc->key0;
        if (key->time <= cnt)
        {
            goto skip;
        }
        do
        {
            next = key - 1;
            spc->key0 = next;
            key = next;
        } while (cnt < next->time);
        spc->key1 = next + 1;
    }
    UpdateSplineControl(spc);
skip:
    SplineFrac = (s16)(((cnt - spc->key0->time) * SPLINE_FRACTION_SCALE) /
                       (spc->key1->time - spc->key0->time));
    if ((s32)SplineFracOld != (s32)SplineFrac)
    {
        SplineFracOld = SplineFrac;
        SplineRow = &SplineTable[SplineFrac];
    }
    eval_spline_gte_(vect, spc, SplineRow);
}
