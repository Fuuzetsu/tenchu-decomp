#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void SetupFly(struct param_fly *pfly, struct VECTOR *start, struct VECTOR *end, int yw, int yh, int time);
 *     ITEM.C:792, 39 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
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
 * STATUS: NON_MATCHING — linked length 656 vs carve extent 664 (8 bytes / 2
 * instructions SHORT), plus register-coloring differences that cascade once
 * the length is off. Two distinct residuals, both codegen artifacts:
 *
 * (1) LENGTH (the 2 missing instructions): the Y and Z "current point" jitter
 *     each compute `mid ± jitter` and the target DUPLICATES that add/subtract
 *     into BOTH conditional branches — the else branch (`jitter = -hx`) folds
 *     to a single `subu mid,hx`, the if branch stays `addu mid,jitter`, and
 *     because the two tails are now different instructions cc1 keeps them
 *     duplicated (2 extra insns vs our single post-merge store). Writing the
 *     store/value inside each branch (`if(...) v=mid-hx; else v=mid+...;`) DOES
 *     reproduce the duplication and fixes the length exactly (664), but then
 *     the extra per-branch pressure displaces `pfly` from $s2 to $s4 and the
 *     whole register map shifts (414 differing bytes) — strictly worse. The
 *     single-store form kept here is 8 short but 116 diffs. Neither is
 *     reachable to 0 from C: the duplication is cc1's cross-jump/combine
 *     interaction, not a source shape.
 * (2) REGISTER COLORING: even at the right length `pfly` colors to $s4, the
 *     target uses $s2 (yw and pfly swap their callee-saved slots). No source
 *     lever found; `regalloc.py` confirms both are only callee-saved because
 *     live across GetVectorDistance, and cc1's find_reg picks the number.
 *
 * Everything structural IS reproduced: 6-arg prototype, the mode/speed byte
 * stores, the `dist/time` speed clamp via the byte's own `& 0xff` truncation,
 * the pre-shifted hy and hy/2 asymmetric split, the X/Y/Z jitter polarity,
 * and `--expand-div` for the variable divisions. The permuter aborts on this
 * TU (rand() typemap gap) as with SetBlood/SetHinoko.
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
void SetupFly(long *pfly, VECTOR *start, VECTOR *end, s32 yw, s32 yh, s32 time)
{
    long v1;
    long v3;
    long v7;
    long v8;
    long midx;
    long midy;
    long midz;

    *((u8 *)pfly + 0x28) = 0;
    pfly[0] = start->vx;
    pfly[1] = start->vy;
    pfly[2] = start->vz;
    pfly[3] = end->vx;
    pfly[4] = end->vy;
    pfly[5] = end->vz;
    v1 = GetVectorDistance(start, end);
    if (0 < time)
    {
        *((u8 *)pfly + 0x24) = v1 / time;
        if ((*((u8 *)pfly + 0x24) & 0xff) != 0)
        {
            goto skip_default;
        }
    }
    *((u8 *)pfly + 0x24) = 1;
skip_default:
    v7 = v1 * (yw / 2);
    *((u8 *)pfly + 0x25) = *((u8 *)pfly + 0x24);
    if (v7 < 0)
    {
        v7 = v7 + 0xfff;
    }
    v1 = v1 * (yh / 2);
    v7 = v7 >> 12;
    if (v1 < 0)
    {
        v1 = v1 + 0xfff;
    }
    v1 = v1 >> 12;
    midx = (pfly[0] + pfly[3]) / 2;
    v8 = v7 << 1;
    if (v8 < 1)
    {
        v8 = -v7;
    }
    else
    {
        v8 = rand() % v8 - v7;
    }
    midy = (pfly[1] + pfly[4]) / 2;
    v3 = v1 / 2;
    v1 = v1 - v3;
    pfly[6] = midx + v8;
    if (0 < v1)
    {
        v3 = rand() % v1 + v3;
    }
    midz = (pfly[2] + pfly[5]) / 2;
    v8 = v7 << 1;
    pfly[7] = midy - v3;
    if (v8 < 1)
    {
        v7 = -v7;
    }
    else
    {
        v7 = rand() % v8 - v7;
    }
    pfly[8] = midz + v7;
    *((u8 *)pfly + 0x24) = *((u8 *)pfly + 0x24) - 1;
}
#endif
