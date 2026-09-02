#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * Canonical handwritten assembly (docs/gte-policy.md). This retail-only
 * helper evaluates one Hermite basis row with the GTE and reads MAC1-MAC3
 * directly; the SDK exposes no C macro for that result path.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/eval_spline_gte_", eval_spline_gte_);
#else
void eval_spline_gte_(SVECTOR *out, SplineControlType *spc, SVECTOR *basis)
{
    s32 b0;
    s32 b1;
    s32 b2;
    s32 b3;
    s32 x;
    s32 y;
    s32 z;

    b0 = basis->vx;
    b1 = basis->vy;
    b2 = basis->vz;
    b3 = basis->pad;

    x = (spc->key0->x * b0 + spc->key1->x * b1 + spc->dd0.vx * b2) >> 12;
    y = (spc->key0->y * b0 + spc->key1->y * b1 + spc->dd0.vy * b2) >> 12;
    z = (spc->key0->z * b0 + spc->key1->z * b1 + spc->dd0.vz * b2) >> 12;

    out->vx = (s16)(x + (s32)((u32)(spc->ds1.vx * b3) >> 12));
    out->vy = (s16)(y + (s32)((u32)(spc->ds1.vy * b3) >> 12));
    out->vz = (s16)(z + (s32)((u32)(spc->ds1.vz * b3) >> 12));
}
#endif
