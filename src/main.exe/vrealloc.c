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
        vhp->size = vhp->size & VMEM_BLOCK_SIZE_MASK;
        size2 = (u32)vhp->size - size;
        if (size2 >= VMEM_MIN_SPLIT_SLACK)
        {
            struct VMhead *nb;

            vh.size = size2 - VMEM_HEADER_WORDS;
            vh.next = vhp->next;
            vhp->size = size | VMEM_BLOCK_IN_USE;
            nb = (struct VMhead *)((u32 *)vhp + size +
                                   sizeof(*vhp) / sizeof(u32));
            vhp->next = nb;
            if (svhp != 0 && (~svhp->size & VMEM_BLOCK_IN_USE) != 0)
            {
                vh.size += (svhp->size + VMEM_HEADER_WORDS);
                vh.next = svhp->next;
            }
            *vhp->next = vh;
        }
    }
    else
    {
        if (svhp != 0)
        {
            mask = VMEM_BLOCK_IN_USE;
            if ((s32)svhp->size >= 0 &&
                (u32)(vhp->size & VMEM_BLOCK_SIZE_MASK) +
                        (u32)svhp->size + VMEM_HEADER_WORDS >= size)
            {
                vhp->size = vhp->size & VMEM_BLOCK_SIZE_MASK;
                size2 = ((u32)vhp->size + svhp->size) - size;
                if (size2 < VMEM_MIN_GROW_SPLIT_SLACK)
                {
                    vhp->size =
                        (vhp->size + svhp->size + VMEM_HEADER_WORDS) | mask;
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
