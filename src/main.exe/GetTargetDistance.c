#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetTargetDistance(struct Humanoid *human, short *deg);
 *     HUMAN.C:394, 10 src lines, frame 40 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct Humanoid * human
 *     param $a1       short * deg
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 31 of 208 bytes differ (right length: the residual
 * doesn't shift anything downstream).
 *
 * GetTargetDistance (0x80029794, 0xd0 bytes) — same "Humanoid control" TU as
 * GetMoveSpeed.c/MoveHumanoid.c/GetHumanoid.c (HUMAN.C). Computes the planar
 * (x,z) distance from `human` to `human->target`, and *deg the signed turn
 * needed to face it (in the same 0x8000-scaled unit Degree/GetMoveSpeed's ry
 * use), biased +-0x1000 depending on whether the raw angle difference falls
 * in the "closest to zero" wedge.
 *
 * `human->target->locate.coord.t[0]/[2]` reach a GsCOORDINATE2's embedded
 * MATRIX.t[] world-position (ModelType.locate @0x00, MATRIX.t[] @0x14 within
 * it — 0x18/0x20 total, matching the asm's displacements).
 *
 * `vy` is a genuine narrow (`u16`) local: `human->rotate->vy` (SVECTOR.vy,
 * signed in the shared header — MoveHumanoid reads the SAME field with
 * `lh`) copied straight into a 16-bit destination reads `lhu` here (the
 * narrowing-use rule: the sign bits are dead once the value only ever feeds
 * a 16-bit copy). Declaring the temp `s32` (not `u16`) and casting only at
 * the assignment (`vy = (u16)human->rotate->vy;`) was required — an `s32`
 * temp assigned straight from the SIGNED field, or a `u16` temp, both cost
 * an extra `andi 0xffff` at the later use (a call-surviving u16 read wants
 * the "int t1 = cap;" idiom from the cookbook's spilled-u16-locals rule, not
 * a narrow-typed temp).
 *
 * RESIDUAL (31 bytes, same length): purely the `if (sVar4 < 0x801) { if
 * (-0x800 < sVar4) goto skip; } else { sVar4 = -sVar4; }` block. Target puts
 * the sign-extended compare value in $a0 and the raw (store/arithmetic)
 * value in $v1 throughout, with the OUTER if's THEN arm (containing the
 * nested if+goto) reached by a taken branch and the ELSE arm (single
 * assignment) as the fallthrough — literal-Ghidra-polarity C instead
 * compiles the textbook way (else reached by branch, if-arm fallthrough),
 * which is 4 bytes shorter but colors $v1 as the compare value and $a0 as
 * the store value (swapped) with a different internal block layout.
 * Inverting the outer test's polarity (`if (0x800 < sVar4) {negate} else
 * {nested-if}`, the cookbook's "opposite polarity" lever) was tried and
 * made it WORSE (75+ bytes, wrong length) — this isn't that family. Also
 * tried: reading the comparisons from `iVar3` instead of `sVar4` (no
 * effect — cc1 CSEs them identically), an explicit self-truncating
 * `iVar3 = (s16)iVar3;` reassignment (no effect either).
 * `tools/autorules.py` found no width-based win. One bounded permuter run
 * (~400s, --stop-on-zero -j4) did not reach 0; not re-run per the
 * attempt-cap guidance.
 */
extern s32 ratan2(s32 a, s32 b);
extern s32 SquareRoot0(s32 x);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/GetTargetDistance", GetTargetDistance);
#else /* NON_MATCHING */

long GetTargetDistance(Humanoid *human, short *deg)
{
    s32 dx, dz;
    s32 lVar2;
    s32 vy;
    s32 iVar3;
    s16 sVar4;

    dx = human->target->locate.coord.t[0] - human->locate->vx;
    dz = human->target->locate.coord.t[2] - human->locate->vz;
    vy = (u16)human->rotate->vy;
    lVar2 = ratan2(-dx, -dz);
    iVar3 = lVar2 - vy;
    sVar4 = (s16)iVar3;
    if (sVar4 < 0x801)
    {
        if (-0x800 < sVar4) goto skip;
    }
    else
    {
        sVar4 = -sVar4;
    }
    sVar4 = sVar4 + 0x1000;
skip:
    *deg = sVar4;
    return SquareRoot0(dx * dx + dz * dz);
}

#endif /* NON_MATCHING */
