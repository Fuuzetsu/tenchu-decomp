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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * human
 *     param $a1       short * deg
 * END PSX.SYM */

/*
 * GetTargetDistance (0x80029794, 0xd0 bytes) — same "Humanoid control" TU as
 * GetMoveSpeed.c/MoveHumanoid.c/GetHumanoid.c (HUMAN.C). Computes the planar
 * (x,z) distance from `human` to `human->target`, and *deg the signed turn
 * needed to face it (0x1000-per-turn ratan2 units: the > 0x800 arm
 * reflects via `0x1000 - deg2`, the <= -0x800 arm wraps by adding
 * 0x1000).
 *
 * `human->target.model->locate.coord.t[0]/[2]` reach a GsCOORDINATE2's embedded
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
 * The tail bias block is the plain
 * `if (deg2 > ANGLE_HALF) deg2 = ANGLE_FULL - deg2; else if (deg2 <= -ANGLE_HALF)
 * deg2 += ANGLE_FULL;` — the load-bearing spelling is the combined
 * `0x1000 - deg2` subtraction in the first arm (a `deg2 = -deg2;` plus a
 * shared add emits negu+addiu where the target has one li+subu at
 * 0x80029808-0x80029810).
 */

long GetTargetDistance(Humanoid *human, short *deg)
{
    s32 dx, dz;
    s32 angle;
    s32 vy;
    s32 diff;
    s16 deg2;

    dx = human->target.model->locate.coord.t[0] - human->locate->vx;
    dz = human->target.model->locate.coord.t[2] - human->locate->vz;
    vy = (u16)human->rotate->vy;
    angle = ratan2(-dx, -dz);
    diff = angle - vy;
    deg2 = (s16)diff;
    if (deg2 > ANGLE_HALF)
    {
        deg2 = ANGLE_FULL - deg2;
    }
    else if (deg2 <= -ANGLE_HALF)
    {
        deg2 += ANGLE_FULL;
    }
    *deg = deg2;
    return SquareRoot0(dx * dx + dz * dz);
}
