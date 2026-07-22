#include "common.h"
#include "main.exe.h"
#include "vmemory.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void * vrealloc(void *pt, unsigned long size);
 *     VALLOC.C:130, 69 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s2       void * pt
 *     param $s0       unsigned long size
 *     reg   $a3       struct VMhead * vhp
 *     reg   $a1       struct VMhead * svhp
 *     stack sp+16     struct VMhead vh
 *     reg   $s1       void * newp
 *     reg   $a2       unsigned long size2
 * END PSX.SYM */

/*
 * vrealloc (0x80016738, 0x204 bytes) — same TU/family as valloc.c/vfree.c
 * (virtual_memory_pool's free-list allocator): realloc over the same
 * PoolBlock free list. `pt == 0` is a plain `valloc(size)` (malloc
 * semantics). Otherwise the request is rounded to words exactly like
 * valloc's own rounding, then:
 *   - if the block's CURRENT raw size (header->size, WITH the in-use flag
 *     still set) is < the requested word count — note this is an UNSIGNED
 *     compare against a value with bit31 set, so it is only true when
 *     growing past the block's real capacity — try to grow in place by
 *     absorbing the immediately-following block (`vhp->next`) if it exists,
 *     is free, and together is big enough; otherwise give up: valloc a
 *     fresh block, memcpy min(requested, vsize(pt)) bytes over, vfree the
 *     old block.
 *   - else (already fits): clear the in-use flag, and if the leftover slack
 *     is big enough (>= 0x13 words) split off a fresh free tail block,
 *     itself absorbing `vhp`'s ORIGINAL next block if that one is also
 *     free. If the slack is small (< 0x13), the function returns with the
 *     in-use flag left CLEARED — a quirk (or latent bug) of the original,
 *     confirmed by both the raw asm and Ghidra's own decompilation, not a
 *     transcription error here.
 *
 * Matching notes (the last 74 bytes were a register-allocation knot, solved
 * with cc1 -dg/-dl RTL dumps run standalone — not by respelling the C):
 *  - The grow-coalesce SPLIT tail's size is the raw excess (no `-2`); the
 *    shrink SPLIT tail's size is `excess - 2`. Different constants (0x11 vs
 *    0x13) gate the two splits too — read the raw immediate in each branch,
 *    don't assume they're the same threshold.
 *  - `vh.next` in the shrink-split branch is read fresh off `vhp->next`
 *    (not the cached `svhp`), even though the two are numerically identical
 *    at that point — matches Ghidra's own literal `*(uint*)(pt+-4)` reread.
 *  - The give-up path's `if (pt != 0) { ...memcpy... }` guarding a copy from
 *    a provably-non-null `pt` is real source, preserved literally.
 *  - The slack (`size2`) and the give-up memcpy length CANNOT be one
 *    variable: the target holds the slack in $v1 but flows the length
 *    through $a2, and gcc 2.8.1 never splits a pseudo's live range — one
 *    variable = one hard register. `-dg` showed the shared variable's
 *    memcpy-third-arg copy-preference ($a2) winning the FIRST allocation
 *    (it was the top-priority pseudo) and rotating every register after it.
 *  - The grow branch's `mask` is a real variable assigned BETWEEN the
 *    `svhp != 0` and `svhp->size >= 0` tests — hence the nested-if +
 *    `goto giveup` shape instead of one `&&` chain. That places the
 *    `li 0x80000000` in the second test's basic block: sched1 slots it
 *    into the `lw svhp->size` load-delay stall, reorg then pulls it into
 *    the `bltz` delay slot, and the assembler re-inserts the load-use
 *    hazard nop — reproducing the target's `lw / nop / bltz / lui` exactly.
 *    A literal 0x80000000 in each arm instead compiles TWO `lui`s (cse's
 *    path-following cannot unify constants across the divergent split/
 *    absorb arms) — one instruction long.
 *  - `mask` is ALSO the give-up path's memcpy-length temp (one variable,
 *    disjoint live ranges). This is load-bearing two ways: the memcpy
 *    third-argument copy gives the pseudo an $a2 preference, and global.c's
 *    find_reg makes earlier allocnos AVOID registers that later allocnos
 *    prefer — so `vhp` (allocated first, higher priority) skips the free
 *    $a2 and lands in $a3, after which mask/length takes $a2 in both
 *    regions. Splitting them left vhp/mask swapped (a2/a3) with no
 *    C-level lever at all.
 */

extern void *valloc(u32 size);
extern void vfree(void *pt);
extern void *memcpy(void *dst, void *src, u32 n);

void *vrealloc(void *pt, u32 size)
{
    PoolBlock *vhp;
    PoolBlock *svhp;
    PoolBlock vh;
    void *newp;
    u32 size2;
    u32 mask;

    newp = pt;
    if (pt == 0)
    {
        newp = valloc(size);
        return newp;
    }

    vhp = (PoolBlock *)pt - 1;
    if ((size & 3) != 0)
        size = size + 4;
    size = size >> 2;

    svhp = vhp->next;

    if ((u32)vhp->size >= size)
    {
        vhp->size = vhp->size & 0x7fffffff;
        size2 = (u32)vhp->size - size;
        if (size2 >= 0x13)
        {
            PoolBlock *nb;

            vh.size = size2 - 2;
            vh.next = vhp->next;
            vhp->size = size | 0x80000000;
            nb = (PoolBlock *)((u8 *)vhp + (size << 2) + 8);
            vhp->next = nb;
            if (svhp != 0 && (~svhp->size & 0x80000000) != 0)
            {
                vh.size = vh.size + (svhp->size + 2);
                vh.next = svhp->next;
            }
            *vhp->next = vh;
        }
    }
    else
    {
        if (svhp != 0)
        {
            mask = 0x80000000;
            if (svhp->size >= 0 &&
                (u32)(vhp->size & 0x7fffffff) + (u32)svhp->size + 2 >= size)
            {
                vhp->size = vhp->size & 0x7fffffff;
                size2 = ((u32)vhp->size + svhp->size) - size;
                if (size2 < 0x11)
                {
                    vhp->size = (vhp->size + svhp->size + 2) | mask;
                    vhp->next = svhp->next;
                }
                else
                {
                    PoolBlock *nb;

                    vh.size = size2;
                    vh.next = svhp->next;
                    vhp->size = size | mask;
                    nb = (PoolBlock *)((u8 *)vhp + (size << 2) + 8);
                    vhp->next = nb;
                    *nb = vh;
                }
            }
            else
                goto giveup;
        }
        else
        {
        giveup:
            newp = valloc(size << 2);
            if (pt != 0)
            {
                mask = vsize(pt);
                if (size < mask)
                    mask = size;
                memcpy(newp, pt, mask);
            }
            vfree(pt);
        }
    }
    return newp;
}
