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
 * STATUS: NON_MATCHING - 10 of 504 bytes differ (4 instructions), at the
 * CORRECT length. Was 167/504 before this round.
 *
 * The previous park's header ("18 differing instruction lines") was stale: it
 * measured 167 bytes / 49 differing instruction lines. Two of its three claims
 * were refuted by measurement this round; both of its recorded negatives were
 * null experiments. Details below, since they cost several rounds:
 *
 * REFUTED 1 - "the target reads point[idx].pad TWICE, un-CSE'd ... a named
 * temp for either side has zero effect: cc1 still recognizes the identical
 * address and CSEs". The observation is right, the conclusion is wrong. cse1
 * DOES share one pseudo for both reads - the two loads are COMBINE's doing,
 * not cse1's. cc1 loads a short field into an HImode pseudo and extends per
 * use; combine fuses `(set (reg:HI N) (mem:HI))` + sll + sra into a single
 * `lh` ONLY when reg N has one use (that is why `lh v0,8(s4)` for
 * point->range works). With two uses it cannot fuse, so the load stays `lhu`
 * and the compare pays a hand-rolled re-sign - the old residual exactly.
 * The lever is combine's `added_sets_2`: when the sign-extend is the load's
 * FIRST use, combine fuses it into `lh` AND retains the original `lhu` for
 * the later `|=` use. Two loads, one address, one `lw`. So the fix is purely
 * WHICH READ COMES FIRST - `sentinel = ...pad;` before the `pad |= ...pad;`.
 * The park's temp arm held that fixed (temp on the OR side / compare left
 * second), so it could not have worked. See the new cookbook rule.
 *
 * REFUTED 2 - the whole "delay-slot fill / reorg" framing of the ladder. The
 * empty slot at 0x800290b8 was a SYMPTOM. `tools/siblingdiff.py --demo` is
 * the oracle that settles it: the demo build (0x80026130, 464 bytes) is
 * byte-identical to retail through the tail, but LACKS the entire
 * `abs(degree) > 500` block (the 464->504 delta) and has NO `move a1,v1` -
 * and its `bnez` delay slot holds `negu v0,a0`, which is exactly what our
 * build produced. So retail's `move a1,v1` is not a reorg decision at all;
 * it is an extra INSTRUCTION that only exists because retail added the abs
 * test, and reorg then prefers it (an insn from before the branch) over
 * stealing `negu`. Get the move and the slot, the `nop`, `negu` and
 * `lh a0,6(s5)` placement all follow. Do not chase the slot directly.
 *
 * The mechanism for `move a1,v1` (this is the reusable part): our RTL dump
 * shows `short` locals are plain HImode pseudos here (`reg/v:HI` - MIPS
 * PROMOTE_MODE is not in play), so a read sign-extends and a WRITE truncates.
 * `(set (reg:HI var) (subreg:HI (reg:SI expr)))` is a real `move`. That is
 * what `t = ang - roty;` already compiled to at 0x800290a0 (`subu v1,v0,s0`
 * + `move a1,v1`) - the same shape the merge needs. Our old
 * `degree = (s16)t;` was a short->short copy (both HImode), which cc1
 * coalesces to nothing, so the writeback never appeared. Splitting it into
 * an SImode temp - `d32 = t; degree = d32;` - restores it, and pointing the
 * two turn-tests at `d32` (not `degree`) keeps cc1 from emitting a second
 * `move v1,v0` to bridge them. With that, the ENTIRE degree-ladder matches.
 *
 * WHAT IS LEFT (4 instructions, all one root cause):
 *     0x80029134  T `li a0,-1`        O `nop`
 *     0x8002915c  T `lh v0,10(v1)`    O `lh v1,10(v1)`
 *     0x80029160  T `lhu v1,10(v1)`   O `li v0,-1`
 *     0x80029164  T `bne v0,a0,..`    O `bne v1,v0,..`
 * The target hoists `li a0,-1` into the load-use slot of `lhu v0,0(s2)` at
 * 0x80029134, which frees the slot our `li v0,-1` occupies for the second
 * (`lhu`) load. `tools/schedtrace.py` shows why this is hard: the `li -1`
 * (insn 286) has LOG_LINKS (nil), so it sits at priority 1 - the FLOOR -
 * where "max_priority = 1 is the initialiser and MIPS defines no
 * ADJUST_PRIORITY", and floor insns are ordered by LUID alone. So its
 * schedule position is a pure source-POSITION lever, not a priority one, and
 * the compare that materialises it must stay last (it is the branch). Tried
 * and MEASURED (all rebuilt + matchdiff'd, none is a no-op unless noted):
 *   - `pad |= (u16)...pad;` to force a zero_extend: NO-OP (tools/nullcheck.py
 *     exit 1) - cc1 erases a same-width HImode->HImode cast. Do not retry.
 *   - store `trcl->index = idx` moved BETWEEN the two reads: real, and it
 *     does yield `lhu`+`lh` (cse1 invalidates ALL memory at a store - there
 *     is no alias-set refinement here), but it also duplicates
 *     `lw v0,4(s2)`; hoisting the pointer into a local fixes that but then
 *     sched1 PINS the store between the loads, while the target's store is
 *     at 0x8002913c above both. So that is not the original's mechanism.
 *   - `s32 endmark = -1;` at the block head to give the constant an early
 *     LUID: no change (still 508).
 *   - two bounded permuter runs (300s, -j4): plateaued 10 -> 9. The 9-byte
 *     candidate is not obviously better structurally; not applied.
 * NOTE the two-load version (`pad = pad | trcl->point[idx].pad;` here) is
 * STRUCTURALLY correct but 508 bytes, because the un-hoisted `li` costs the
 * slot; the committed 10-byte variant instead CSEs both reads into one `lh`
 * (semantically identical - `or` only uses the low 16 bits) to hold the
 * length. If you fix the `li` hoist, restore the two-load spelling: those
 * two edits are +1 and -1 and should land the match together.
 *
 * autorules WARNING: do not run it from a length-mismatched baseline. Its
 * greedy score treats "restored the length" as a win regardless of
 * correctness - it applied `degree: s16->s8` (1004->154) here, which is
 * semantically broken (degree holds +-0x1000) and emits `sll ..,0x18` /
 * `sra ..,0x18` where the target has 0x10. Rejected.
 *
 * ControlTraceLine (0x80028fb0) - steer a character along its trace line:
 * distance to the current waypoint drives a "turn toward it" pad-like
 * result (`pad`, 0x1000 base), degree-of-turn escalates the result the same
 * way AttackAnimal's actmode ladder does, and reaching the waypoint (dist <=
 * point->range) advances the index (wrapping the whole result to -0x1000 at
 * the sentinel).
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `pad = 0x1000;` is assigned (in the null-trace guard's delay slot)
 *    BEFORE the null check - matches Ghidra's literal order.
 *  - `human->attribute` (item.h, s16) is read via `lhu` for the pure `& 0x400`
 *    test - a narrowing bitwise use, no field-type change needed.
 *  - `s16 cnt = trcl->count; trcl->count = cnt + 1; if (cnt <= 0) goto skip;`
 *    - cc1 compares the PRE-increment value via the `sll 16; blez` idiom.
 *  - `roty` must be `u16` (not `s16`) - a global copied into a same-width
 *    unsigned local, so its `int` promotion in `t = ang - roty;` is a natural
 *    zero-extend needing no extra sign-extend/mask.
 *  - `t` must be the NARROW `short`, not `s32`: the `move a1,v1` at
 *    0x800290a0 IS `t = ang - roty` (HImode var <- SImode expr) and only a
 *    HImode `t` produces it.
 *  - The degree ladder tests `0x801 <= a0` FIRST (De-Morgan-inverted from
 *    Ghidra's literal polarity) because that is the arm the asm places
 *    inline; the `a0 < -0x7FF` arm and the implicit default follow.
 *  - `pad |= 0x8000;` / `pad |= 0x2000;` are real `ori`s, also De-Morgan
 *    inverted (the `human->turn <= degree` arm is inline first).
 *  - The waypoint-reached block re-reads `trcl->point[idx]` (the ARRAY BASE
 *    field, freshly reloaded) for the new index - NOT through `point`.
 *  - PSX.SYM's 11 locals are the DEMO's, whose body lacks the abs test; the
 *    retail ladder provably needs the extra SImode temp (`d32`). `sentinel`
 *    likewise has no PSX.SYM counterpart. Both are honest reconstructions
 *    that reproduce the target's instructions, not recovered names.
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
    s32 sentinel;
    s32 d32;

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
        d32 = t;
        degree = d32;
        if (human->turn <= d32)
        {
            pad |= 0x2000;
        }
        else if (d32 <= -human->turn)
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
        sentinel = trcl->point[idx].pad;
        d32 = trcl->point[idx].pad;
        pad = pad | d32;
        if (sentinel == -1)
        {
            trcl->index = 0;
            return -0x1000;
        }
    }
    return pad;
}
#endif /* NON_MATCHING */
