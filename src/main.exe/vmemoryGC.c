#include "common.h"
#include "main.exe.h"
#include "vmemory.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void * vmemoryGC(void *pt);
 *     VALLOC.C:259, 40 src lines, frame 56 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s2       void * pt
 *     reg   $s3       struct VMhead * vhp
 *     reg   $a3       struct VMhead * svhp
 *     stack sp+16     struct VMhead vh
 *     reg   $s1       unsigned long * vmpt
 *     reg   $s6       unsigned long size
 *     reg   $s2       void * pt
 *     reg   $s2       void * pt
 *     reg   $a0       struct VMhead * svhp
 *     reg   $s0       struct VMhead * vhp
 *     reg   $s1       void * pt
 *     reg   $a0       struct VMhead * svhp
 *     reg   $s0       struct VMhead * vhp
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *virtual_memory_pool;
 * END PSX.SYM */

extern char msg_double_memory_release[]; /* DOUBLE MEMORY RELEASE */
extern void *valloc(u32 size);
extern void *memcpy(void *dst, void *src, u32 n);

static inline void free_block(void *pt, u32 cmask)
{
    struct VMhead *header;
    struct VMhead *next;
    struct VMhead *prev;
    struct VMhead *n2;
    u32 mask;
    u32 sz;
    s32 s;

    if (pt == 0)
        return;

    do
    {
        header = (struct VMhead *)pt - 1;
    } while (0);
    mask = 0x80000000;
    if ((header->size & mask) == 0)
        SystemOut(msg_double_memory_release);

    sz = header->size & cmask;
    header->size = sz;

    next = header->next;
    if (next != 0)
    {
        s = next->size;
        if ((~s & mask) != 0)
        {
            header->size = sz + (s + 2);
            header->next = next->next;
        }
    }

    prev = (struct VMhead *)virtual_memory_pool;
    if (prev != 0)
    {
    search:
        n2 = prev->next;
        if (n2 != header)
        {
            prev = n2;
            if (prev != 0)
                goto search;
        }
        if (prev != 0)
        {
            s = prev->size;
            if (s >= 0)
            {
                prev->size = s + (header->size + 2);
                prev->next = header->next;
            }
        }
    }
}

/*
 * vmemoryGC — same TU/family as valloc.c/vfree.c/vrealloc.c: a compacting
 * "garbage collector" over the free-list pool. valloc()s a fresh block the
 * same size as pt's current block; if the fresh block landed at a LOWER
 * address, moves pt's data there and frees the old pt block (real
 * compaction) and returns the new address. Otherwise the fresh block is no
 * improvement, so it's freed right back and instead the function tries to
 * slide pt DOWN into the free block immediately preceding it in the pool
 * list (found by the same linear walk vfree.c uses), returning the moved
 * (or, on failure, unchanged) address.
 *
 * Matching notes: no `jal vfree` appears anywhere in the target. A single
 * inline helper containing vfree.c's already-matched free+coalesce body is
 * expanded once for `pt` and once for `vmpt`. This is more than source
 * cleanup: the helper's distinct formal/local identities recover the target
 * full-width neighbour loads and reduce the exact-length residual from 241
 * to 96 bytes. The repeated `pt`/`vhp`/`svhp` debug locals are consistent
 * with those two inline expansions. Each expansion keeps vfree's leading
 * null guard; jump2 threads the first one into the following `return vmpt`
 * and the second one into the final compaction block.
 *  - PSX.SYM calls the outer allocation result `vmpt` in $s1 and records only
 *    one outer `svhp` in $a3. Reusing `vmpt` for the final split-tail pointer,
 *    then reusing the final search cursor for `vh.next` after memcpy, carries
 *    those source identities through both disjoint ranges. This fixes the
 *    entire old saved-register cycle and final caller-register tail.
 *  - One zero-trip wrapper around ONLY the inline helper's header assignment
 *    adds no code but gives both expanded header pseudos the one extra weighted
 *    reference needed for target header=$s0 / mask=$s4. With the source reuse
 *    above it replaces the old three-wrapper first-call workaround and reduces
 *    the exact-length residual from 41 to 24 bytes.
 *  - One zero-trip wrapper around the final mask definition keeps that value
 *    across memcpy and restores the exact 195-instruction extent (without it
 *    the function is 776 bytes); it leaves no branch in the optimized asm.
 *    Both surviving wrappers were re-challenged at MATCH and are load-bearing:
 *    unwrapping the helper's header assignment costs 25 bytes. A third wrapper
 *    around the final `vh.size` sum, previously believed to resolve a two-byte
 *    caller-register tie, was re-tested at MATCH and is NOT load-bearing --
 *    removed.
 *  - Parenthesizing the final offset as `base + ((hsz << 2) + 8)` selects the
 *    target addiu-before-addu tree (24 to 16 bytes); reusing `prev` after the
 *    call for `vh.next` closes the final $a0->$a3 tail (16 to 12 bytes).
 *  - Both coalescing sums are vfree.c's proven `A + (B + 2)` spelling.
 *  - Each free-list search keeps only its label backedge. Inverting the
 *    found test lets success fall into coalescing and removes the acyclic
 *    `found` edge without adding LOOP notes.
 *  - **`size = (header->size & cmask) << 2` is what closed the last 12 bytes**
 *    (a saved-register swap: complement mask $s6->$s5, byte size $s5->$s6).
 *    The `and` is NOT in the target and is not meant to be: `<< 2` discards
 *    bit 31 regardless, so combine's force_to_mode deletes it. But flow.c has
 *    already counted the reference, and global.c:604 scores an allocno
 *    `floor_log2(n_refs) * n_refs / live_length`. That fourth ref carries
 *    cmask over the floor_log2 3->4 step (numerator 3 -> 8): 810 -> 2162,
 *    above `size`'s 1194, so cmask is allocated first and takes $s5. It is
 *    also the honest spelling — the header's `size` field reserves its sign
 *    bit as an in-use flag, so masking it off before the shift is what the
 *    original author would write. Same lever as vfree.c's `mask` second use.
 *    Do NOT "simplify" it back to `header->size << 2`: that is the 12-byte
 *    regression.
 *
 * The two rejected alternatives, for the record: a `do{}while(0)` around the
 * cmask definition also flips the pair, but its LOOP_BEG note bars sched2
 * from interleaving the lui/ori up among the prologue saves (the target has
 * it between `sw s5` and `sw ra`), stranding the load-delay slot -> +1 nop,
 * 196 insns. Fencing the inline `sz` use instead gives cmask 5 refs (2702),
 * which overshoots the outer `header`'s 2727 and rotates it plus mask/helper
 * header -> 38 bytes. 4 refs is the only value in the window (1194, 2307).
 *
 * STATUS: MATCH — 780 bytes / 195 instructions, 0 differing bytes.
 */
void *vmemoryGC(void *pt)
{
    struct VMhead *header;
    u32 cmask;
    u32 mask;
    u32 size;
    u32 *vmpt;

    cmask = 0x7fffffff;
    header = (struct VMhead *)pt - 1;
    size = (header->size & cmask) << 2;
    vmpt = valloc(size);
    if (vmpt != 0)
    {
        if ((void *)vmpt < pt)
        {
            memcpy(vmpt, pt, size);
            free_block(pt, cmask);
            return vmpt;
        }
        else
        {
            free_block(vmpt, cmask);
        }
    }

    {
        struct VMhead *prev;
        struct VMhead *n2;
        void *newpt;
        struct VMhead vh;
        s32 sz;

        prev = (struct VMhead *)virtual_memory_pool;
        if (prev != 0)
        {
        search:
            n2 = prev->next;
            if (n2 != header)
            {
                prev = n2;
                if (prev != 0)
                    goto search;
            }
            if (prev != 0)
            {
                sz = prev->size;
                if (sz >= 0)
                {
                    do
                    {
                        mask = 0x80000000;
                    } while (0);
                    newpt = (void *)(prev + 1);
                    vh.size = sz;
                    vh.next = header->next;
                    vmpt = (u32 *)((u8 *)prev + ((header->size << 2) + 8));
                    prev->size = header->size;
                    prev->next = (struct VMhead *)vmpt;
                    memcpy(newpt, pt, size);
                    pt = newpt;
                    prev = vh.next;
                    if (prev != 0 && (~prev->size & mask) != 0)
                    {
                        vh.size += (prev->size + 2);
                        vh.next = prev->next;
                    }
                    *(struct VMhead *)vmpt = vh;
                }
            }
        }
    }
    return pt;
}
