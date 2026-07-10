#include "common.h"
#include "main.exe.h"

/*
 * GetVectorRotation (0x8003975c, 0xac bytes) — the aim-direction helper: given
 * two world points, writes the pitch (*rx) and yaw (*ry) that aim `from` at
 * `to`. Yaw is `ratan2` of the negated horizontal deltas; pitch is `ratan2` of
 * the vertical delta over the horizontal distance (`SquareRoot0` of dx²+dz²).
 *
 * The out-params are written as full words here. Callers (ReqItemArrow.c,
 * ReqItemLightningBolt.c) declare their own externs taking `u16 *`/`short *`
 * and read only the low half back with `lhu` — a per-TU narrowing, not this
 * function's own signature.
 *
 * Statement order is load-order significant: dx (vx), then dz (vz), then dy
 * (vy) — the deltas are computed in that order even though dy is consumed last.
 */

extern s32 SquareRoot0(s32 x);
extern s32 ratan2(s32 y, s32 x);

void GetVectorRotation(VECTOR *from, VECTOR *to, s32 *rx, s32 *ry)
{
    s32 dz;
    s32 dx;
    s32 dy;

    dx = to->vx - from->vx;
    dz = to->vz - from->vz;
    dy = to->vy - from->vy;
    *ry = ratan2(-dx, -dz);
    *rx = ratan2(dy, SquareRoot0((dx * dx) + (dz * dz)));
}
