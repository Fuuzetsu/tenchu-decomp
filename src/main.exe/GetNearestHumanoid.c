#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * GetNearestHumanoid(struct Humanoid *human, short distance);
 *     HUMAN.C:408, 24 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
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
 *     param $a0       struct Humanoid * human
 *     param $a1       short distance
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

/*
 * GetNearestHumanoid (0x80029864) returns the nearest other live, visible,
 * type-filtered Humanoid within `distance`, or 0. It searches only while the
 * input Humanoid is at ground level (map.height == 0). The type/attribute
 * mask tests intentionally use unsigned halfword loads; status/life use
 * signed halfword loads, matching their comparisons and item.h declarations.
 *
 * The height check MUST be an early-return guard clause, not an `if (height
 * == 0) { for (...) }` wrapper, even though both spell the same behaviour and
 * both fold to the same `bnez -> epilogue` branch. cc1's post-reload
 * `reload_cse_regs` (toplev.c:3501, after .greg, before sched2) rewrites a
 * constant SET_SRC into a copy from the first hard register (scanning regno 0
 * upward) already recorded as holding that value -- unconditionally; there is
 * no cost comparison, and $zero never qualifies because no insn ever sets it.
 * With the wrapper form nothing separates `best = 0` (s4) from the loop's
 * `i = 0`, so s4 is still recorded as 0 and the preheader degrades to
 * `move s2,s4`. reload_cse_regs forgets every register value at a CODE_LABEL,
 * and the early return's `if_false` label lands exactly between the two
 * zeroes, so s4 is forgotten and `i = 0` stays `move s2,zero` as in the
 * target. jump2 cross-jumping then merges the two identical `return best`
 * tails, which is why the final asm is otherwise identical.
 *
 * The indexed `for` is intentional: cc1 strength-reduces HumanGroup[i] to
 * the target's s1 walking pointer while keeping i in s2. `__builtin_abs`
 * keeps each absolute value as one RTL operation until reorg, which lets the
 * first dx multiply issue before dz is formed and hides both MULT hazards.
 * Reusing dx as the 0x90 type-mask threshold gives that first abs result the
 * target's distinct v1 output lifetime; dx is overwritten before its
 * distance use. Flattening the guard loses that lifetime and coalesces the
 * abs input/output in v0.
 */

extern s16 Humans;

Humanoid *GetNearestHumanoid(Humanoid *human, short distance)
{
    Humanoid *cur;
    Humanoid *best;
    s32 dx, dz;
    s32 dist;
    s32 best_dist;
    int i;

    best = 0;
    best_dist = 20000;
    if (human->map.height != 0)
    {
        return best;
    }
    for (i = 0; i < Humans; i = i + 1)
    {
        cur = HumanGroup[i];
        if (cur != human && cur->status != 0x11 &&
            (cur->attribute & 0x80) == 0)
        {
            dx = 0x90;
            if ((cur->type & 0xf0) != dx && -1 < cur->life)
            {
                dx = __builtin_abs(cur->locate->vx - human->locate->vx);
                dz = __builtin_abs(cur->locate->vz - human->locate->vz);
                dist = SquareRoot0(dx * dx + dz * dz);
                if (dist < distance && dist < best_dist)
                {
                    best_dist = dist;
                    best = cur;
                }
            }
        }
    }
    return best;
}
