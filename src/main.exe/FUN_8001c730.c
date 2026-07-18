#include "common.h"
#include "main.exe.h"
#include "item.h"

/*
 * ASM-CANONICAL (owner decision 2026-07-19; docs/gte-policy.md).
 *
 * FUN_8001c730 (0x8001c730, 0xdc bytes) is the retail GTE rewrite of the
 * scalar Hermite evaluator at the end of the demo's GetSpline
 * (0x8001b39c).  `basis` points at one row of HermiteTable[33][4].  The
 * helper loads basis[0..2] as GTE V0, arranges key0/key1/dd0 as the GTE's
 * 3x3 rotation matrix, executes MVMVA, then adds the fourth Hermite term
 * `(basis[3] * ds1.xyz) >> 12` and narrows the results to an SVECTOR.
 * The readable C below records that exact value-level operation.
 *
 * This helper did not exist in the symbol-bearing demo.  Retail places it at
 * the end of ACTION.C's function run, immediately before MOTION.C, and
 * GetSpline is its only observed caller.  It is also the only function outside
 * the already-classified handwritten draw handlers that reads GTE MAC1/2/3
 * directly into CPU registers.  PsyQ INLINE_C.H provides gte_rtv0_b(), but its
 * corresponding gte_stlvnl() writes through memory with SWC2; it has no
 * MAC1/2/3-to-C-register macro capable of producing this tail.
 *
 * Compiler-led reconstruction confirmed the split.  Human-shaped direct-field
 * C reaches every operation, and fixed-register diagnostics can force the
 * prefix through the first multiply byte-exact, but only by encoding the
 * target's t0..t5 plan in C.  The remaining tail is hand-scheduled across both
 * multiply latency and the GTE result read: the plausible volatile macro form
 * forces the third MFLO before the MFC2 reads, while the target deliberately
 * reads MAC1..3 first; a nonvolatile invented macro crosses that boundary but
 * cc1 consumes the first two products early.  Retaining either register pins
 * or a fake GTE macro would disguise assembly as C, so the original assembly
 * is the canonical source form and the #else body is documentation only.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8001c730", FUN_8001c730);
#else
void FUN_8001c730(SVECTOR *out, SplineControlType *spc, SVECTOR *basis)
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
