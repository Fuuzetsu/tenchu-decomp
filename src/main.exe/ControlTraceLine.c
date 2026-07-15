#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ControlTraceLine(struct Humanoid *human);
 *     HUMAN.C:526, 33 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s5       struct Humanoid * human
 *     reg   $s4       struct TracePoint * point
 *     reg   $s1       struct TraceLine * trcl
 *     reg   $s2       long dx
 *     reg   $s0       long dz
 *     reg   $s6       long dist
 *     reg   $s3       short pad
 *     reg   $s2       long dx
 *     reg   $s0       long dz
 *     reg   $s0       short roty
 *     reg   $a1       short degree
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 18 differing instruction lines (8 blocks) at the
 * CORRECT overall length (126 insns both sides), all downstream of ONE
 * root cause: the raw asm shows the degree-ladder's SECOND test (`else if
 * (a0 < -0x7FF)`) filling its own branch's delay slot with `sll v0,a1,0x10`
 * — the FIRST instruction of the merge point's `degree = (s16)t;`
 * truncation, stolen forward by reorg because it is safe to run
 * unconditionally (both arms reach the same truncation next, just with `t`
 * possibly reassigned on one of them — the same instruction is then
 * genuinely repeated at the merge point too for the reassigned-`t` case).
 * Our build leaves this exact delay slot a bare `nop` instead — same
 * mechanism as the cookbook's named "guard's DELAY-SLOT FILL tie" class
 * (StickonCheck), just on a different branch. A second, independent
 * residual sits in the waypoint-reached block: the target reads
 * `trcl->point[idx].pad` TWICE, un-CSE'd (one `lh` for the `== -1` compare,
 * one `lhu` for the `pad |=` merge); our build reuses a single `lhu` +
 * explicit resign for both. Tried and reverted: reordering the compare
 * before the OR statement; a named temp for either the OR or the compare
 * side (both zero effect — cc1 still recognizes the identical address and
 * CSEs). A bounded `tools/permute.py ControlTraceLine -- --stop-on-zero -j4`
 * run (~5 min, ~15-20k iterations) never beat baseline (it was run against
 * a slightly stale intermediate source and its best candidates introduced
 * an outright semantic bug — `if (dz < dz)` never firing — that the
 * permuter's own score doesn't distinguish from a real fix; NOT applied).
 *
 * ControlTraceLine (0x80028fb0) — steer a character along its trace line:
 * distance to the current waypoint drives a "turn toward it" pad-like
 * result (`pad`, 0x1000 base), degree-of-turn escalates the result the same
 * way AttackAnimal's actmode ladder does, and reaching the waypoint (dist <=
 * point->range) advances the index (wrapping the whole result to -0x1000 at
 * the sentinel).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `pad = 0x1000;` is assigned (in the null-trace guard's delay slot)
 *    BEFORE the null check — matches Ghidra's literal order (`uVar2 =
 *    0x1000;` before `if (pTVar8 == 0) return 0;`); the two guard-clause
 *    exception doesn't apply (only one path returns early here).
 *  - `human->attribute` (item.h, s16) is read via `lhu` for the pure `& 0x400`
 *    test — a narrowing bitwise use, no field-type change needed.
 *  - `s16 cnt = trcl->count; trcl->count = cnt + 1; if (cnt <= 0) goto
 *    skip;` — cc1 compares the PRE-increment value via the `sll 16; blez`
 *    idiom (no `sra` needed — `blez` only needs the sign bit), all from one
 *    `lhu` load (a narrowing capture reused for a signed test, the
 *    established u16-load/signed-compare disagreement).
 *  - `roty` must be `u16` (not `s16`) — a global copied into a same-width
 *    unsigned local (matches the HUMAN.C-sibling idiom), so its `int`
 *    promotion in `t = ang - roty;` is a natural zero-extend needing no
 *    extra sign-extend/mask instruction (an `s16`/`s32` local both cost 1-2
 *    extra instructions here — verified both ways).
 *  - `t` (the ang-roty difference driving the whole degree ladder) must be
 *    declared the NARROW `short`, not `s32` — a wide accumulator here
 *    forces the register holding it into the "wrong" allocation class for
 *    the ladder's three-way branch, producing an extra `andi 0xffff` AND
 *    scrambling the a1/v1 register choice throughout the ladder; `short t`
 *    matches byte-for-byte instruction-wise (confirmed by permuter probing
 *    this exact lever first).
 *  - The degree ladder tests `0x801 <= a0` FIRST (the `>= 0x801` arm is the
 *    physically-first/inline body, De-Morgan-inverted from Ghidra's
 *    `< 0x801` literal polarity) because that's the arm the asm places
 *    inline; the `else if (a0 < -0x7FF)` arm and the implicit "leave t
 *    alone" default follow.
 *  - `pad |= 0x8000;` / `pad |= 0x2000;` are real `ori`s (not flat
 *    assignments) — Ghidra's flattened `uVar2 = 0x9000` / `= 0x3000`
 *    constant-folds the OR against `pad`'s already-known 0x1000 base; this
 *    ladder is ALSO De-Morgan-inverted from Ghidra's literal polarity (the
 *    `human->turn <= degree` arm is inline first).
 *  - The waypoint-reached block re-reads `trcl->point[idx]` (the ARRAY
 *    BASE field, freshly reloaded) for the new index — NOT through the
 *    `point` local (which still addresses the OLD index) — matching the
 *    asm's fresh `lw trcl->point` rather than reusing $s4.
 */
extern s32 SquareRoot0(s32 val);
extern s32 ratan2(s32 y, s32 x);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ControlTraceLine", ControlTraceLine);
#else
short ControlTraceLine(Humanoid *human)
{
    TraceLine *trcl;
    TracePoint *point;
    s32 dx, dz;
    s32 dist;
    s16 cnt;
    s16 pad;
    u16 roty;
    s32 ang;
    short t;
    s16 a0;
    s16 degree;
    s32 absdeg;
    s16 idx;

    trcl = human->trace;
    pad = 0x1000;
    if (trcl == 0)
    {
        return 0;
    }
    point = trcl->point + trcl->index;
    dx = point->x - human->locate->vx;
    dz = point->z - human->locate->vz;
    dist = SquareRoot0(dx * dx + dz * dz);
    if ((human->attribute & 0x400) != 0)
    {
        trcl->count = -0x1e;
    }
    cnt = trcl->count;
    trcl->count = cnt + 1;
    if (cnt > 0)
    {
        roty = human->rotate->vy;
        ang = ratan2(-dx, -dz);
        t = ang - roty;
        a0 = (s16)t;
        if (0x801 <= a0)
        {
            t = 0x1000 - t;
        }
        else if (a0 < -0x7FF)
        {
            t = t + 0x1000;
        }
        degree = (s16)t;
        if (human->turn <= degree)
        {
            pad |= 0x2000;
        }
        else if (degree <= -human->turn)
        {
            pad |= 0x8000;
        }
        absdeg = degree;
        if (absdeg < 0)
        {
            absdeg = -absdeg;
        }
        if (absdeg > 500)
        {
            pad &= 0xA000;
        }
    }
    if (dist <= point->range)
    {
        idx = trcl->index + 1;
        trcl->index = idx;
        pad = pad | trcl->point[idx].pad;
        if (trcl->point[idx].pad == -1)
        {
            trcl->index = 0;
            return -0x1000;
        }
    }
    return pad;
}
#endif /* NON_MATCHING */
