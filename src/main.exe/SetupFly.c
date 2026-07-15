#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void SetupFly(struct param_fly *pfly, struct VECTOR *start, struct VECTOR *end, int yw, int yh, int time);
 *     ITEM.C:792, 39 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
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
 * A jump2-erased identical-arm alias separates the long-lived output base
 * (`fly`) from the incoming `pfly` formal.  Unlike the old all-in-one inline
 * helper, the direct vector copies leave `start` and `end` in a1/a2 until the
 * distance call; retail's late argument moves can therefore fill the end-copy
 * load delay slots.  The alias itself takes the target's s2 throughout.
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
 *    `void SetupFly(void *param, VECTOR *start, VECTOR *end, s32 a4, s32 a5,
 *    s32 a6)` — Ghidra drops `yh`/`time`, the two STACK-passed args past the
 *    four register ones (cookbook: Ghidra undercounts stack args). Kept the
 *    first parameter `long *pfly` here (matching Ghidra's own indexing
 *    style) rather than inventing `struct param_fly *` — nothing in this
 *    function needs the union's other members, and the three callers' own
 *    externs already keep it `void *` (respell per-TU; don't fight it).
 *  - `pfly->mode` (byte @0x28) is set to 0 as the FIRST statement, before
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
extern int GetVectorDistance(VECTOR *v1, VECTOR *v2);

static inline long SubFlyJitter(long mid, long half, long range)
{
    if (0 < range)
    {
        return mid - (rand() % range + half);
    }
    return mid - half;
}

void SetupFly(long *pfly, VECTOR *start, VECTOR *end, s32 yw, s32 yh, s32 time)
{
    long len;
    long v3;
    long v8;
    long midx;
    long midy;
    long midz;
    long current_z;
    long x_product;
    long y_pair;
    long y_range;
    long *fly;

    if (pfly != 0)
    {
        fly = pfly;
    }
    else
    {
        fly = pfly;
    }
    *((u8 *)fly + 0x28) = 0;
    fly[0] = start->vx;
    fly[1] = start->vy;
    fly[2] = start->vz;
    fly[3] = end->vx;
    fly[4] = end->vy;
    fly[5] = end->vz;
    len = GetVectorDistance(start, end);
    if (0 < time)
    {
        *((u8 *)fly + 0x24) = len / time;
        if ((*((u8 *)fly + 0x24) & 0xff) != 0)
        {
            goto skip_default;
        }
    }
    *((u8 *)fly + 0x24) = 1;
skip_default:
    x_product = len * (yw / 2);
    *((u8 *)fly + 0x25) = *((u8 *)fly + 0x24);
    if (x_product < 0)
    {
        x_product = x_product + 0xfff;
    }
    len = len * (yh / 2);
    yw = x_product >> 12;
    if (len < 0)
    {
        len = len + 0xfff;
    }
    yh = len >> 12;
    do
    {
        do
        {
            midx = (fly[0] + fly[3]) / 2;
            v8 = yw << 1;
        } while (0);
    } while (0);
    if (0 < v8)
    {
        len = midx + (rand() % v8 - yw);
    }
    else
    {
        len = midx - yw;
    }
    y_pair = fly[1] + fly[4];
    midy = y_pair / 2;
    v3 = yh / 2;
    y_range = yh - v3;
    fly[6] = len;
    len = SubFlyJitter(midy, v3, y_range);
    midz = (fly[2] + fly[5]) / 2;
    v8 = yw << 1;
    fly[7] = len;
    if (0 < v8)
    {
        current_z = midz + (rand() % v8 - yw);
    }
    else
    {
        current_z = midz - yw;
    }
    fly[8] = current_z;
    *((u8 *)fly + 0x24) = *((u8 *)fly + 0x24) - 1;
}
