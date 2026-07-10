#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void * vrealloc(void *pt, unsigned long size);
 *     VALLOC.C:130, 69 src lines, frame 40 bytes, saved-reg mask 0x80070000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $s2       void * pt
 *     param $s0       unsigned long size
 *     reg   $a3       struct VMhead * vhp
 *     reg   $a1       struct VMhead * svhp
 *     stack sp+16     struct VMhead vh
 *     reg   $s1       void * newp
 *     reg   $a2       unsigned long size2
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 74 of 516 bytes differ; RIGHT LENGTH (129
 * instructions both sides). The residual is confined to which caller-saved
 * register `svhp` (the block immediately after `vhp` in the free list) and
 * a couple of related temporaries land in — a pure register-allocation
 * rotation (mine: a0/v1-ish; target: a1-ish), not a missing/extra/wrong
 * instruction anywhere (asmdiff --structural shows only register-name
 * replacements, zero inserts/deletes). `tools/regalloc.py` shows no copy-chain
 * pinning `svhp`'s pseudo to a hard register — it's a plain "no obvious lever"
 * global-alloc tie. `tools/permute.py` ran ~3200 iterations (--stop-on-zero,
 * -j4, ~10 min bounded): best candidate scored 270 (from a base of 340, the
 * permuter's OWN objective) via aliasing `svhp->size` through a `s32 *`
 * pointer for one read — verified against matchdiff and it does NOT change
 * the real byte count (still 74), confirming the cookbook's warning that the
 * permuter's score is not raw bytes. Parked per the sub-C-level early-stop.
 *
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
 * Matching notes:
 *  - The grow-coalesce SPLIT tail's size is the raw excess (no `-2`); the
 *    shrink SPLIT tail's size is `excess - 2`. Different constants (0x11 vs
 *    0x13) gate the two splits too — read the raw immediate in each branch,
 *    don't assume they're the same threshold.
 *  - `vh.next` in the shrink-split branch is read fresh off `vhp->next`
 *    (not the cached `svhp`), even though the two are numerically identical
 *    at that point — matches Ghidra's own literal `*(uint*)(pt+-4)` reread
 *    instead of reusing puVar7.
 *  - The give-up path's `if (pt != 0) { ...memcpy... }` guarding a copy from
 *    a provably-non-null `pt` (we're already inside the `pt != 0` branch) is
 *    real source, preserved literally by Ghidra too — same "redundant guard
 *    kept as real code" shape as valloc.c's own `virtual_memory_pool != 0`.
 */

typedef struct PoolBlock
{
    s32 size; /* word count, sign bit reserved as an in-use flag by valloc */
    struct PoolBlock *next;
} PoolBlock;

extern void *valloc(u32 size);
extern void vfree(void *pt);
extern u32 vsize(void *pt);
extern void *memcpy(void *dst, void *src, u32 n);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/vrealloc", vrealloc);
#else

void *vrealloc(void *pt, u32 size)
{
    PoolBlock *vhp;
    PoolBlock *svhp;
    PoolBlock vh;
    void *newp;
    u32 size2;

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
        if (svhp != 0 && svhp->size >= 0 &&
            (u32)(vhp->size & 0x7fffffff) + (u32)svhp->size + 2 >= size)
        {
            u32 mask;

            mask = 0x80000000;
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
        {
            newp = valloc(size << 2);
            if (pt != 0)
            {
                size2 = vsize(pt);
                if (size < size2)
                    size2 = size;
                memcpy(newp, pt, size2);
            }
            vfree(pt);
        }
    }
    return newp;
}
#endif
