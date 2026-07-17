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
 *
 * REFUTED 3 (this round, by measurement) - the "floor / LUID" framing above,
 * and `tools/schedtrace.py`'s OWN SUMMARY LINE, are both WRONG. schedtrace
 * prints "N insn(s) at priority 1 (the floor). Nothing can lift them:
 * max_priority = 1 is the initialiser and MIPS defines no ADJUST_PRIORITY, so
 * an insn with LOG_LINKS (nil) is at 1 unconditionally." That claim is refuted
 * by schedtrace's own ready lists. For our `li -1`:
 *     ;; insn[ 291]: priority =    1, ref_count =    1     <- the TABLE
 *     ;; ready list at T-2: 288 (3) 291 (7f000001), now 291 288
 * The li's priority is 1 in the table but 0x7f000001 = LAUNCH_PRIORITY in the
 * READY LIST: sched1's generic `adjust_priority` (sched.c) bumps it AT LAUNCH
 * TIME. MIPS defining no ADJUST_PRIORITY macro does not disable it. So the li
 * NEVER competes at the floor, LUID is irrelevant, and it is not a
 * source-POSITION lever. That is why round 1's `s32 endmark = -1;` "early
 * LUID" arm did nothing: it was never a LUID problem. Read the priority in the
 * `ready list at T-k:` lines, NOT the `;; insn[N]:` table.
 *
 * THE REAL MECHANISM (verified 6/6 in this function's own RTL). sched1's
 * `adjust_priority` -> `birthing_insn_p` fires iff the pattern is a
 * `(set (REG) ...)` whose dest is live and has REG_N_SETS == 1 (set EXACTLY
 * ONCE in the whole function); it is then bumped to LAUNCH_PRIORITY. sched's
 * walk is BACKWARD (T-1 is the LAST insn of the block), so a bumped insn is
 * picked FIRST in the walk and lands as LATE as possible - cc1 deliberately
 * shortening the new value's live range. Controls, all in this function:
 *   - insn 11  `(set (reg/v:SI 81) (mem))`      = `trcl`, 1 set  -> 1 -> 7f000001
 *   - insn 14  `(set (reg/v:HI 87) (const_int 4096))` = `pad`, many sets -> stays 1
 *       (structurally IDENTICAL to our `li -1`; REG_N_SETS is the ONLY difference)
 *   - insn 112 `(set (reg:SI 4 a0) ...)`        hard reg        -> stays 1
 *   - the `or`, `(set (subreg:SI (reg/v:HI 87) 0) ...)` -> SUBREG dest is NEVER
 *     birthing (priority 3) - which is exactly why the `or` wins T-2.
 *
 * THE HOIST IS REACHABLE - PROVEN. Give the -1 a pseudo with REG_N_SETS >= 2
 * by reusing an existing multiply-set local (`s32 a0; a0 = t; ... a0 = -1;`):
 * the li is then NOT bumped, drops to the floor, and sched places it at
 * EXACTLY the target's slot - `0x80029134 li a1,-1` where we had the `nop`.
 * The two-load spelling comes with it. So the `li` half is SOLVED.
 *
 * WHY IT STILL DOES NOT MATCH (measured with `tools/regalloc.py --order`).
 * The reuse merges two live ranges into ONE allocno, whose live length jumps
 * 7 -> 18 and whose allocation priority falls to 5555, so it is allocated 7th:
 *     #  pseudo  reg refs live priority
 *     4    p90   a0    4   11     7272
 *     5   p135   a0    3    5     6000
 *     6    p91   a1    5   18     5555   <- the merged `a0` local
 * By then a0 is taken, so it gets a1 and `t` takes a0. The ladder is then a
 * pure a0<->a1 swap (+8B) and block 18 a v0<->v1 swap (which also flips the
 * two loads, since the 2nd load must clobber its own base). Net 508. The two
 * requirements are CONTRADICTORY under every reuse spelling:
 *   - REG_N_SETS >= 2 needs a SECOND set, which must live in another block
 *     (two sets in straight-line block 18 -> the first is dead -> flow deletes
 *     it -> back to 1 set), and
 *   - a second set in another block lengthens the allocno -> it loses hard a0.
 * Every local was checked for the register fit; only the `a0` local lives in
 * hard a0. `absdeg`->v0, `d32`->v1, `t`/`degree`->a1, `dist`->s6, `ang`->v0,
 * `roty`->s0. None can hold the -1 in a0. regalloc.py --order confirms p91 is
 * not reachable by priority. The target's -1 is its OWN pseudo with (by this
 * model) REG_N_SETS != 1, and no natural C spelling found produces that
 * without merging a live range. THAT is the open question - not the LUID.
 *
 * Also tried and MEASURED this round (rebuilt + matchdiff'd each):
 *   - `turn = human->turn;` as a named local, then `turn = -1;` reused for the
 *     sentinel: the -1 DOES land in hard a0 AND hoists correctly. But the very
 *     same non-birthing property floats `lh a0,6(s5)` out of the ladder
 *     (0x800290c8 -> 0x800290c0). 504 bytes but 142 differ. The mechanism cuts
 *     both ways: the -1 must be non-birthing while the `turn` load must stay
 *     birthing. Do not retry without solving that.
 *   - swapping the two reads (`pad |= X;` BEFORE the sign-extend read): 512.
 *     Confirms REFUTED 1's combine rule - the sign-extend read must be FIRST,
 *     because added_sets_2 retains i2 (the `lhu`) at i2's position, so the
 *     retained load is ALWAYS emitted before the fused `lh`.
 *   - `pad |= (u16)...pad;` to force a zero_extend: NO-OP (tools/nullcheck.py
 *     exit 1) - cc1 erases a same-width HImode->HImode cast. Do not retry.
 *   - store `trcl->index = idx` moved BETWEEN the two reads: real, and it
 *     does yield `lhu`+`lh` (cse1 invalidates ALL memory at a store - there
 *     is no alias-set refinement here), but it also duplicates
 *     `lw v0,4(s2)`; hoisting the pointer into a local fixes that but then
 *     sched1 PINS the store between the loads, while the target's store is
 *     at 0x8002913c above both. So that is not the original's mechanism.
 *   - `s32 endmark = -1;` at the block head to give the constant an early
 *     LUID: no change (still 508). Now EXPLAINED: never a LUID problem.
 *   - two bounded permuter runs (300s, -j4): plateaued 10 -> 9. The 9-byte
 *     candidate is not obviously better structurally; not applied.
 * NOTE the two-load version (`pad = pad | trcl->point[idx].pad;` here) is
 * STRUCTURALLY correct but 508 bytes, because the un-hoisted `li` costs the
 * slot; the committed 10-byte variant instead CSEs both reads into one `lh`
 * (semantically identical - `or` only uses the low 16 bits) to hold the
 * length. Those two edits are +1 and -1 and must land together.
 *
 * autorules WARNING: do not run it from a length-mismatched baseline. Its
 * greedy score treats "restored the length" as a win regardless of
 * correctness - it applied `degree: s16->s8` (1004->154) here, which is
 * semantically broken (degree holds +-0x1000) and emits `sll ..,0x18` /
 * `sra ..,0x18` where the target has 0x10. Rejected.
 *
 * THIS ROUND (independent re-derivation, no new bytes - CLOSES the reuse
 * search rather than just re-trying it): a fresh task brief re-proposed
 * "fix the double read first" as though untried. Re-verified both halves
 * with a cleaner idiom, `d32 = *(u16 *)&trcl->point[idx].pad;` (a genuine
 * pointer-retype, not the value-cast `(u16)x` already logged as a no-op):
 *   - Double read ALONE (no `li` hoist): 512 bytes. asmdiff confirms the
 *     split itself WORKS (`lhu a1,10(v1)` + `lh v1,10(v1)` both appear,
 *     cse1 does not re-merge them), but with nothing to hold -1 early, the
 *     tail needs a 5th instruction (`lh`,`lhu`,`li`,`bne`,`or`) instead of
 *     the target's 4. Confirms the header's 508-byte finding for a
 *     different double-read spelling was about the PAIRING, not that one
 *     idiom - fixing the read alone is a real, reproducible regression, not
 *     a spelling artifact. Do not re-propose it as a standalone fix.
 *   - Double read + `a0`-reuse (this round's exact respelling: `s32 a0;
 *     a0 = t; ... ; a0 = -1; if (sentinel == a0)`): 508 bytes. asmdiff shows
 *     the `li` DOES land at the target's exact slot, 0x80029134 - the hoist
 *     mechanism is confirmed working a second time, independently - but as
 *     `li a1,-1`, not `a0`, and the whole ladder above it (0x80029094 to
 *     0x800290c0) trades `a0`<->`a1` throughout, while the tail's base
 *     register flips to `v0` and the `lh`/`lhu` order reverses (the load
 *     that overwrites the base must come second). Byte-identical mechanism
 *     to the already-recorded cascade, reproduced from scratch.
 *   - NEW evidence that CLOSES the search (not just another failed try):
 *     `tools/regalloc.py --order` on the committed baseline lists exactly
 *     two `a0`-preferring pseudos (p128, p138). `tools/rtldump.py
 *     ControlTraceLine --pass lreg` identifies p128 at insn 131,
 *     `(reg:SI 128) = ashiftrt(ashift(t_pseudo,16),16)` - the compiler-
 *     synthesized sign-extended value of `a0` itself, CSE'd across both
 *     ladder comparisons (`0x801<=a0` and `a0<-0x7FF`). It is not an
 *     independent candidate, it is the SAME C variable already tried. There
 *     is no third natural a0 occupant in this function.
 *   - General argument, not just another empirical negative: ANY reuse
 *     needs the constant's pseudo to have REG_N_SETS >= 2 via a genuinely
 *     LIVE (non-dead) earlier set. A pair of sets confined to the tail
 *     block (e.g. a fresh `x = 0; x = -1;` immediately before the branch)
 *     has no intervening read on ANY path, so gcc-2.8.1's flow.c removes
 *     the first set as dead before sched1 ever runs - REG_N_SETS is back to
 *     1 regardless of source distance between the two sets. The only way to
 *     survive that is reusing a value with a REAL separate need elsewhere,
 *     which is exactly what causes the cascade above. The two requirements
 *     (non-dead second set / short live range) are structurally
 *     unsatisfiable together in this function, not just unsolved.
 *   - Fresh bounded permuter run (240s, -j4; tooling since updated with the
 *     90s authoritative-rescore deadline). Authoritative full-link rescore:
 *     base 10/10/504; best candidate 9/9/504
 *     (`.shake/permuter/ControlTraceLine/RESULT.md`,
 *     output-415-1/source.c). REJECTED after verifying by direct build:
 *     it rewrites `if (sentinel == -1)` as
 *     `if (sentinel == (new_var = -1))` with a freshly-introduced
 *     `unsigned char new_var`. `-1` truncates to 255 through that assignment
 *     - confirmed by disassembling the actual candidate build, which emits
 *     `li v0,255` (`240200ff`), not `-1`. That silently changes the
 *     wraparound-sentinel condition from `pad == -1` to `pad == 255`, which
 *     would essentially never fire on real level data - a functional bug,
 *     not a match, despite scoring lower on the proxy metric. This confirms
 *     (with a concrete mechanism) the prior round's "not obviously better
 *     structurally; not applied" caution on the same-shaped candidate.
 * No improvement found this round; still parked at 10/504. Both open levers
 * from the prior round (the double read alone, and the register-reuse
 * hoist) are now independently reproduced AND, for the reuse lever,
 * structurally closed - not just re-affirmed. Do not re-propose either
 * without new RTL evidence naming a DIFFERENT pseudo/register than p128/
 * p138/the a0<->a1 cascade already on record.
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
