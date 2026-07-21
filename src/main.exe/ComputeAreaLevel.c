#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long ComputeAreaLevel(struct AreaNodeType *node, long x, long z);
 *     CONFLICT.C:83, 20 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
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
 *     param $t0       struct AreaNodeType * node
 *     param $a1       long x
 *     param $a2       long z
 *     reg   $a0       short yy
 * END PSX.SYM */

/*
 * ComputeAreaLevel (0x8001986c) — height query for a single AreaNodeType leaf:
 * splits the node's rect into a division-bitmask grid (z-span/x-span each cut
 * in quarters), tests whether that grid cell is enabled in node->division,
 * and if so returns node->y plus a linear slope contribution along whichever
 * axis node->attribute's top two bits select (0x4000 = slope along x,
 * 0x8000 = slope along z, else flat).
 *
 * Matching notes (docs/matching-cookbook.md; all verified against the bytes;
 * PSX.SYM only names ONE extra local ("yy") — wrong here, the asm needs four
 * more (dz/zspan/dx/xspan), each read TWICE (the mask test and a case body)):
 *  - Real runtime divisions (variable divisor) — this file needs
 *    maspsx --expand-div (Build.hs maspsxGpExterns).
 *  - x1/z1/x2/z2/division read `lhu` (Ghidra's own (uint)(ushort) casts
 *    confirm this TU's unsigned uses), while y/dy/attribute stay plain
 *    `short`; explicit casts preserve those reads on the shared original
 *    AreaNodeType.
 *  - **`x & (1 << n)` for a non-constant `n` ALWAYS canonicalizes to
 *    `(x >> n) & 1` under this cc1** (verified standalone: any direct
 *    `division & (1<<shift)` compiles to srav+andi, never sllv+and,
 *    regardless of how the shift expression is spelled). Naming the shifted
 *    mask first (`int mask = 1 << shift; if (division & mask)`) is the ONLY
 *    way to keep the literal `sllv`+`and` shape the target uses — this is a
 *    real reusable rule, not specific to this function.
 *  - **The `<<2` scale must be a separate reused variable's OWN use, not an
 *    inline `(short)(diff) << 2` subexpression**, or fpeephole fuses the
 *    truncating sra with the shift into one `sra x,x,14` — one instruction
 *    SHORTER than the target. The fusion is suppressed here only because
 *    `dz`/`dx` (the plain truncated diffs) are each read a SECOND time later
 *    (in the case_4000/case_8000 slope multiply), so cc1 must keep the
 *    unshifted value alive in its own register instead of folding it away.
 *  - **Two guard-goto ladder, THEN a modify-shared-variable-and-fall-through
 *    tail**, not two independent `return EXPR;` guards: `if (attr==0x4000)
 *    goto case_4000; if (attr==0x8000) goto case_8000; goto tail; case_4000:
 *    yy = yy + slope; goto tail; case_8000: yy = yy + slope; tail: return
 *    yy;` reproduces the target's SINGLE shared return tail (cross-jump
 *    merges the two case bodies' truncate-and-return code with the default's
 *    only when every path funnels through one `return yy;` — three separate
 *    `return` statements each compiled their OWN full tail instead, 10+
 *    bytes over target).
 *  - **The `return 0x80000000;` guard must be a `goto ret_min;` to a label at
 *    the very END of the function, past the main tail** — inline as
 *    `if (cond) return 0x80000000;` right at the test floats the constant's
 *    `lui` into the guard branch's delay slot (6-byte tie: a stray `lui`
 *    appears at the guard and a `nop` appears where the target has the `lui`,
 *    at the real ret_min site). Matches the general "labeled return body
 *    pins a duplicated return constant" rule, here for a guard clause rather
 *    than a loop.
 */

long ComputeAreaLevel(AreaNodeType *node, long x, long z)
{
    short dz, zspan;
    short dx, xspan;
    short yy;
    int mask;

    dz = z - (u16)node->z1;
    zspan = (u16)node->z2 - (u16)node->z1 + 1;
    dx = x - (u16)node->x1;
    xspan = (u16)node->x2 - (u16)node->x1 + 1;

    mask = 1 << (((dz << 2) / zspan) * 4 + ((dx << 2) / xspan));
    if (((u16)node->division & mask) == 0)
        goto ret_min;

    yy = node->y;

    if ((node->attribute & 0xC000) == 0x4000)
        goto case_4000;
    if ((node->attribute & 0xC000) == 0x8000)
        goto case_8000;
    goto tail;

case_4000:
    yy = yy + dx * node->dy / xspan;
    goto tail;
case_8000:
    yy = yy + dz * node->dy / zspan;

tail:
    return yy;

ret_min:
    return 0x80000000;
}
