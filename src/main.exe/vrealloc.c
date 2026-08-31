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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * `struct VMhead` free list. `pt == 0` is a plain `valloc(size)` (malloc
 * semantics). Otherwise the request is rounded to words exactly like
 * valloc's own rounding, then:
 *   - if the block's CURRENT raw size (header->size, WITH the in-use flag
 *     still set) is < the requested word count — note this is an UNSIGNED
 *     compare against a value with bit31 set, so for any pointer valloc
 *     actually handed out it is NEVER true (the request tops out around
 *     0x40000000 words): a live block always takes the "already fits" arm,
 *     and the grow path only fires on a header whose flag is clear — try
 *     to grow in place by absorbing the immediately-following block
 *     (`vhp->next`) if it exists, is free, and together is big enough;
 *     otherwise give up: valloc a fresh block, memcpy over, vfree the old
 *     block. The copy length is `min(size, vsize(pt))` with `size` in
 *     WORDS but vsize() in BYTES — retail's own unit mix (it undercopies
 *     to a quarter), preserved as-is.
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
 *  - Locate either split header in allocator words: cast `vhp` to `u32 *`,
 *    add `size`, then add `sizeof(*vhp) / sizeof(u32)`. The left-associated
 *    form preserves retail's base-plus-payload-plus-header address order.
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
    struct VMhead *vhp;
    struct VMhead *svhp;
    struct VMhead vh;
    void *newp;
    u32 size2;
    u32 mask;

    newp = pt;
    if (pt == 0)
    {
        return valloc(size);
    }

    vhp = (struct VMhead *)pt - 1;
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
            struct VMhead *nb;

            vh.size = size2 - 2;
            vh.next = vhp->next;
            vhp->size = size | 0x80000000;
            nb = (struct VMhead *)((u32 *)vhp + size +
                                   sizeof(*vhp) / sizeof(u32));
            vhp->next = nb;
            /* Byte-required spelling: the not-and differs from line 144's
             * (s32)size >= 0 form of the same in-use test (measured). */
            if (svhp != 0 && (~svhp->size & 0x80000000) != 0)
            {
                vh.size += (svhp->size + 2);
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
            if ((s32)svhp->size >= 0 &&
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
                    struct VMhead *nb;

                    vh.size = size2;
                    vh.next = svhp->next;
                    vhp->size = size | mask;
                    nb = (struct VMhead *)((u32 *)vhp + size +
                                           sizeof(*vhp) / sizeof(u32));
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
