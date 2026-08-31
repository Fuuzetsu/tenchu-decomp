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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       long dx
 *     param $a1       long dz
 *     param $a2       short roty
 * END PSX.SYM */

/*
 * GetDirection (0x8002972c, 0x68 bytes) — angle-difference-to-target,
 * corrected as written: `0x1000 - diff` reflects the high side, `diff + 0x1000` wraps the low side (a 0x1000/4096 = one full turn representation,
 * same units as SVECTOR rotation components). Same "Humanoid control" TU as
 * is_humanoid_on_stage_.c; called by turn_towards_player_.
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
 * `result` defaults to `diff` (the mid-range case) unconditionally,
 * before either test — this default-then-override shape
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
    if (sdiff > ANGLE_HALF)
    {
        result = ANGLE_FULL - diff;
    }
    else if (sdiff <= -ANGLE_HALF)
    {
        result = diff + ANGLE_FULL;
    }
    return result;
}
