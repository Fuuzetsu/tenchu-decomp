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
    mask = VMEM_BLOCK_IN_USE;
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
            header->size = sz + (s + VMEM_HEADER_WORDS);
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
                prev->size = s + (header->size + VMEM_HEADER_WORDS);
                prev->next = header->next;
            }
        }
    }
}

void *vmemoryGC(void *pt)
{
    struct VMhead *header;
    u32 cmask;
    u32 mask;
    u32 size;
    u32 *vmpt;

    cmask = VMEM_BLOCK_SIZE_MASK;
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
                        mask = VMEM_BLOCK_IN_USE;
                    } while (0);
                    newpt = (void *)(prev + 1);
                    vh.size = sz;
                    vh.next = header->next;
                    vmpt = (u32 *)((u8 *)prev +
                                   ((header->size << 2) + VMEM_HEADER_BYTES));
                    prev->size = header->size;
                    prev->next = (struct VMhead *)vmpt;
                    memcpy(newpt, pt, size);
                    pt = newpt;
                    prev = vh.next;
                    if (prev != 0 && (~prev->size & mask) != 0)
                    {
                        vh.size += (prev->size + VMEM_HEADER_WORDS);
                        vh.next = prev->next;
                    }
                    *(struct VMhead *)vmpt = vh;
                }
            }
        }
    }
    return pt;
}
