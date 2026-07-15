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
 * STATUS: NON_MATCHING — exact 664-byte / 166-instruction extent, with 204
 * differing linked bytes and fuzzy 63.86 (up from 55.26).  The raw aligned
 * residual is 58 lines in 26 blocks; the structural-only view is 31 lines in
 * 9 blocks.  The 0x30 frame and s0-s5+ra save set remain exact.
 *
 * A jump2-erased identical-arm alias separates the long-lived output base
 * (`fly`) from the incoming `pfly` formal.  Unlike the old all-in-one inline
 * helper, the direct vector copies leave `start` and `end` in a1/a2 until the
 * distance call; retail's late argument moves can therefore fill the end-copy
 * load delay slots.  The alias itself takes the target's s2 throughout.
 *
 * Two nested one-shot loops around the Y-magnitude product, plus one around
 * the X midpoint assignment, leave no machine branch.  Their loop notes tune
 * allocation and scheduling after the alias split, recover the exact extent,
 * and reduce the linked residual from the best exact natural form's 415 bytes
 * to 204.  `SubFlyJitter` still makes the Y result a branch return value, so
 * cc1 retains the target's duplicated subtract tails instead of merging them
 * into one post-branch expression.
 *
 * The remaining cycle colors scaled `yh` in s1 instead of retail's s5 and
 * keeps the first scaled-X product in s3 instead of a3.  That feeds the Y
 * range/midpoint rotation, while the symmetric X/Z tails still cross-jump in
 * the opposite physical layout.  Two bounded guided sweeps and focused
 * lifetime-range donors found no smaller semantics-preserving pure-C result;
 * a tempting s16 midpoint win was rejected because VECTOR coordinates are
 * full-width.  No asm, register pinning, volatile access, or undefined-value
 * fence is used.
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

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetupFly", SetupFly);
#else

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
    yw = len * (yw / 2);
    *((u8 *)fly + 0x25) = *((u8 *)fly + 0x24);
    if (yw < 0)
    {
        yw = yw + 0xfff;
    }
    do {
      do {
        yh = len * (yh / 2);
      } while (0);
    } while (0);
    yw = yw >> 12;
    if (yh < 0)
    {
        yh = yh + 0xfff;
    }
    yh = yh >> 12;
    do {
      midx = (fly[0] + fly[3]) / 2;
    } while (0);
    v8 = yw << 1;
    if (v8 < 1)
    {
        v8 = -yw;
    }
    else
    {
        v8 = rand() % v8 - yw;
    }
    midy = (fly[1] + fly[4]) / 2;
    v3 = yh / 2;
    yh = yh - v3;
    fly[6] = midx + v8;
    v3 = SubFlyJitter(midy, v3, yh);
    midz = (fly[2] + fly[5]) / 2;
    v8 = yw << 1;
    fly[7] = v3;
    if (v8 < 1)
    {
        yw = -yw;
    }
    else
    {
        yw = rand() % v8 - yw;
    }
    fly[8] = midz + yw;
    *((u8 *)fly + 0x24) = *((u8 *)fly + 0x24) - 1;
}
#endif
