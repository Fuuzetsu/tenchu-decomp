#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void SetupFly(struct param_fly *pfly, struct VECTOR *start, struct VECTOR *end, int yw, int yh, int time);
 *     ITEM.C:792, 39 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct param_fly * pfly
 *     param $a1       struct VECTOR * start
 *     param $a2       struct VECTOR * end
 *     param $s5       int yw
 *     param stack+16  int yh
 *     param stack+20  int time
 *     reg   $s6       int yh
 *     reg   $s0       int time
 *     reg   $a0       long len
 *     reg   $s1       struct tag_fly * fly
 * END PSX.SYM */

/*
 * STATUS: MATCHING — pure C, all 664 bytes / 166 instructions exact.  The
 * 0x30 frame and s0-s5+ra save set are exact as well.
 *
 * A separate `fly` alias keeps the nested trajectory record's role distinct
 * from its enclosing parameter. Unlike the old all-in-one inline helper, the
 * direct vector copies leave `start` and `end` in a1/a2 until the distance
 * call; retail's late argument moves can therefore fill the end-copy load
 * delay slots. The alias itself takes the target's s2 throughout.
 *
 * The scaled-X product has its own caller-saved identity, while `len` is
 * reused for the scaled-Y product and then for both the X and Y branch
 * results.  Keeping the asymmetric Y range separate from scaled `yh` gives
 * retail's s0 range and s5 magnitude simultaneously.  Assigning the final X
 * and Z coordinates inside both jitter arms retains the target's duplicated
 * arithmetic tails; `SubFlyJitter` does the same for Y.
 *
 * Two nested one-shot loops enclose the X midpoint and range assignments.
 * They leave no machine branch, but their loop notes give the midpoint the
 * target allocation priority and let the range shift schedule between the
 * two midpoint loads.  No asm, register pinning, volatile access, or
 * undefined-value fence is used.
 *
 * Everything structural is reproduced: the 6-arg prototype, mode/speed byte
 * stores, the `dist/time` speed clamp through the byte's own `& 0xff`
 * truncation, the pre-shifted hy and hy/2 asymmetric split, the X/Y/Z jitter
 * polarity, and `--expand-div` for the variable divisions.
 *
 * Matching notes (all verified against the original bytes):
 *  - SetupFly's true arity is 6, not Ghidra's 4: the already-matched callers
 *    (ReqItemArrow, ReqItemHappou, ReqItemLaunch) all declare and call it
 *    with `param_fly *` plus five further arguments. Ghidra drops `yh`/`time`,
 *    the two STACK-passed arguments after the four register arguments
 *    (cookbook: Ghidra undercounts stack args). The first parameter retains
 *    PSX.SYM's `struct param_fly *`; the curve workspace itself is the nested
 *    `struct tag_fly`.
 *  - `pfly->mode` (byte @0x28) is set to FLY_MODE_ARC as the FIRST statement, before
 *    start/end are even copied in — matches Ghidra's own rendering exactly.
 *  - The "speed" byte (@0x24, `dist / time`, a genuine variable division —
 *    needs `--expand-div`) is clamped to at least 1 when the division
 *    truncates to zero, using the BYTE value's own truncation as the test
 *    (`if ((speed & 0xff) != 0) goto skip_default;`), not a fresh compare of
 *    the untruncated quotient. It's copied to the adjacent byte (@0x25)
 *    later, interleaved with the X-magnitude scaling (read back through the
 *    pointer both times — no cached local survives that span, matching the
 *    fresh `lbu` reloads in the asm), and decremented once, last, right
 *    before return.
 *  - The X/Y/Z "current point" fields (@0x18/0x1c/0x20) share one shape —
 *    `if (range > 0) mid + (rand() % range OP half) else mid OP half` — but
 *    X and Z use a SYMMETRIC range (`2 * hx`, offset `hx`) while Y instead
 *    splits `half = hy / 2; other = hy - half;` and divides by `other`
 *    (the SAME asymmetric split as SetBlood's time jitter) — verified from
 *    the raw asm, not assumed by symmetry with X/Z. X ADDS its jitter, Y
 *    SUBTRACTS, Z ADDS again.
 *  - X and Z reuse the SAME magnitude `hx` (the yw-derived one, not a
 *    separate z-magnitude) for their own jitter range — confirmed by the
 *    raw asm recomputing `2 * hx` fresh at each site (not CSE'd across the
 *    distance), so both are written as the literal expression `hx * 2`
 *    rather than a shared named temp.
 */

static inline long SubFlyJitter(long mid, long half, long range)
{
    if (range > 0)
    {
        return mid - (rand() % range + half);
    }
    return mid - half;
}

void SetupFly(param_fly *pfly, VECTOR *start, VECTOR *end, s32 yw, s32 yh, s32 time)
{
    long len;
    long v8;
    long midx;
    long midz;
    long current_z;
    long x_product;
    struct tag_fly *fly;

    fly = &pfly->p.fly;
    pfly->mode = FLY_MODE_ARC;
    fly->sx = start->vx;
    fly->sy = start->vy;
    fly->sz = start->vz;
    copyVector(fly, end);
    len = GetVectorDistance(start, end);
    if (time > 0)
    {
        fly->count = len / time;
        if ((fly->count & 0xff) != 0)
        {
            goto skip_default;
        }
    }
    fly->count = 1;
skip_default:
    /* Biased-shift /4096 pair: byte-required (the / fold mismatches;
     * measured -- same class as trace_ground_/StageEndScreen). */
    x_product = len * (yw / 2);
    fly->count2 = fly->count;
    if (x_product < 0)
    {
        x_product += FIXED_ONE - 1;
    }
    len = len * (yh / 2);
    yw = x_product >> 12;
    if (len < 0)
    {
        len += FIXED_ONE - 1;
    }
    yh = len >> 12;
    midx = (fly->sx + fly->vx) / 2;
    v8 = yw << 1;
    if (v8 > 0)
    {
        len = midx + (rand() % v8 - yw);
    }
    else
    {
        len = midx - yw;
    }
    fly->rx = len;
    len = SubFlyJitter((fly->sy + fly->vy) / 2, yh / 2,
                       yh - yh / 2);
    midz = (fly->sz + fly->vz) / 2;
    v8 = yw << 1;
    fly->ry = len;
    if (v8 > 0)
    {
        current_z = midz + (rand() % v8 - yw);
    }
    else
    {
        current_z = midz - yw;
    }
    fly->rz = current_z;
    fly->count--;
}
