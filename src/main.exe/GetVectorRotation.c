#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void GetVectorRotation(struct VECTOR *start, struct VECTOR *end, int *rx, int *ry);
 *     EFFECT.C:532, 7 src lines, frame 40 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct VECTOR * start
 *     param $a1       struct VECTOR * end
 *     param $a2       int * rx
 *     param $a3       int * ry
 * END PSX.SYM */

/*
 * GetVectorRotation (0x8003975c, 0xac bytes) — the aim-direction helper: given
 * two world points, writes the pitch (*rx) and yaw (*ry) that aim `start` at
 * `end`. Yaw is `ratan2` of the negated horizontal deltas; pitch is `ratan2` of
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

void GetVectorRotation(VECTOR *start, VECTOR *end, s32 *rx, s32 *ry)
{
    s32 dz;
    s32 dx;
    s32 dy;

    dx = end->vx - start->vx;
    dz = end->vz - start->vz;
    dy = end->vy - start->vy;
    *ry = ratan2(-dx, -dz);
    *rx = ratan2(dy, SquareRoot0((dx * dx) + (dz * dz)));
}
