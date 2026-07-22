#include "common.h"
#include "main.exe.h"
#include "humanoid.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetDirection(long dx, long dz, short roty);
 *     HUMAN.C:381, 9 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $a0       long dx
 *     param $a1       long dz
 *     param $a2       short roty
 * END PSX.SYM */

/*
 * GetDirection (0x8002972c, 0x68 bytes) — angle-difference-to-target,
 * wrapped into [-0x800, 0x7FF] (a 0x1000/4096 = one full turn representation,
 * same units as SVECTOR rotation components). Same "Humanoid control" TU as
 * is_character_state_present_on_stage_.c; called by turn_towards_player_.
 *
 * Retail widened the demo's `short roty` parameter to s32. Giving the retail
 * definition a short parameter adds an absent sign-extension pair in the
 * callee; callers that narrow a computed angle do so explicitly at the call.
 *
 * `diff` (the raw ratan2()-roty subtraction) stays WIDE (s32) and feeds the
 * two wrap corrections (0x1000-diff / diff+0x1000) directly; a SEPARATE
 * s16 truncated copy (`sdiff`) is compared against the range constants. The
 * asm proves the split: the comparisons read a sign-extended $a0 (one
 * sll+sra, CSE'd across both slti's), while the arithmetic reads the
 * original untruncated $v1 — if `diff` itself were s16, the arithmetic
 * would have to reuse the SAME truncated register instead of a second one.
 * `result` defaults to `diff` (the mid-range case) as the FIRST statement,
 * unconditionally, before either test — this default-then-override shape
 * (not if/else-if/else) is what lets reorg hoist the default assignment
 * into the first branch's delay slot, harmless on the "too high" path
 * since it's overwritten before use there. The final (short)-return
 * truncation then applies to whichever value `result` holds: reorg
 * speculatively duplicates it into the second test's delay slot (valid on
 * the "mid-range" not-taken path, where `result` is already the default)
 * as well as the shared join reached by the "too high"/"too low" paths.
 */

s16 GetDirection(s32 dx, s32 dz, s32 roty)
{
    s32 diff;
    s16 sdiff;
    s16 result;

    diff = ratan2(-dx, -dz) - roty;
    sdiff = diff;
    result = diff;
    if (sdiff >= 0x801)
    {
        result = 0x1000 - diff;
    }
    else if (sdiff < -0x7FF)
    {
        result = diff + 0x1000;
    }
    return result;
}
