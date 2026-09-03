#include "common.h"
#include "main.exe.h"
#include "vmemory.h"

/*
 * Retail VALLOC.C emits its three substantial allocator routines before the
 * smaller helpers. The manifest retains the earlier demo source-line order.
 */

extern char msg_out_of_memory[];       /* "OUT OF MEMORY\nREQUEST=%d\nFREE=%d(%d)\n" */
extern char msg_double_memory_release[]; /* "DOUBLE MEMORY RELEASE" */


/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void * valloc(unsigned long size);
 *     VALLOC.C:76, 50 src lines, frame 1080 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t0       unsigned long size
 *     stack sp+24     struct VMhead vh
 *     reg   $a2       struct VMhead * vhp
 *     reg   $s1       unsigned long * vmpt
 *     stack sp+32     struct VMhead vh
 *     stack sp+40     unsigned char [1024] str
 *     reg   $a3       unsigned long maxsize
 *     reg   $a0       struct VMhead * vhp
 *     reg   $a1       unsigned long freesize
 *     reg   $v1       struct VMhead * vhp
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *virtual_memory_pool;
 * END PSX.SYM */

void *valloc(u32 size)
{
    struct VMhead vh;   /* split tmp, sp+0x18 */
    struct VMhead *vhp; /* search-loop copy of the cursor */
    u32 *vmpt;          /* cursor AND result — returned after SystemOut, hence $s1 */
    u32 off;
    u32 mask;
    u32 tag;

    if (virtual_memory_pool == 0)
    {
        struct VMhead vh; /* nested shadow, sp+0x20 */

        virtual_memory_pool = VMEM_DEFAULT_POOL;
        vh.size = VMEM_DEFAULT_CAPACITY;
        vh.next = 0;
        *(struct VMhead *)virtual_memory_pool = vh;
    }

    if ((size & 3) != 0)
        size += 4;
    size = size >> 2;

    vmpt = (u32 *)virtual_memory_pool;
    if (vmpt != 0)
    {
        off = (size << 2) + VMEM_HEADER_BYTES;
        mask = VMEM_BLOCK_IN_USE;
        tag = size | mask;
        do
        {
            vhp = (struct VMhead *)vmpt;
            if (!(vmpt[0] & mask) && size <= vmpt[0])
            {
                if (vmpt[0] - size < VMEM_MIN_SPLIT_SLACK)
                {
                    /* The unreachable empty loop preserves a control-flow boundary whose source form is unknown. */
                    vmpt[0] = vmpt[0] | mask;
                    vmpt = vmpt + VMEM_HEADER_WORDS;
                    goto search_done;
                    do
                    {
                    } while (0);
                }
                else
                {
                    vh.size = vhp->size - size - 2;
                    vh.next = vhp->next;
                    vmpt = (u32 *)((u8 *)vmpt + off);
                    vhp->size = tag;
                    vhp->next = (struct VMhead *)vmpt;
                    *(struct VMhead *)vmpt = vh;
                    vmpt = (u32 *)(vhp + 1);
                }
                break;
            }
            vmpt = (u32 *)vhp->next;
        } while (vmpt != 0);
    search_done:
        if (vmpt != 0)
            goto done; /* bnez straight to the shared epilogue-return */
    }

    {
        u8 str[1024]; /* sp+0x28 */
        u32 maxsize;
        u32 freesize;
        maxsize = 0;
        {
            struct VMhead *vhp;

            for (vhp = (struct VMhead *)virtual_memory_pool; vhp != 0;
                 vhp = vhp->next)
            {
                if (!(vhp->size & VMEM_BLOCK_IN_USE) &&
                    maxsize < (u32)vhp->size)
                    maxsize = vhp->size;
            }
        }

        freesize = 0;
        maxsize <<= 2;
        {
            struct VMhead *vhp;

            for (vhp = (struct VMhead *)virtual_memory_pool; vhp != 0;
                 vhp = vhp->next)
            {
                if (!(vhp->size & VMEM_BLOCK_IN_USE))
                    freesize += vhp->size;
            }
        }

        sprintf((char *)str, msg_out_of_memory, size << 2, maxsize, freesize << 2);
        SystemOut(str);
    }
done:
    return vmpt;
}

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
                return newp;
            }
        }

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
    return newp;
}

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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void vinit(void *adr, unsigned long size);
 *     VALLOC.C:33, 8 src lines, frame 8 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       void * adr
 *     param $a1       unsigned long size
 *     stack sp+0      struct VMhead vh
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *virtual_memory_pool;
 * END PSX.SYM */

void vinit(void *adr, u32 size)
{
    struct VMhead vh;

    virtual_memory_pool = adr;
    if (adr == 0)
        virtual_memory_pool = VMEM_DEFAULT_POOL;

    if (size != 0)
        vh.size = (size >> 2) - 2;
    else
        vh.size = VMEM_DEFAULT_CAPACITY;
    vh.next = 0;
    *(struct VMhead *)virtual_memory_pool = vh;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long vgetmaxsize(void);
 *     VALLOC.C:45, 12 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *virtual_memory_pool;
 * END PSX.SYM */

unsigned long vgetmaxsize(void)
{
    struct VMhead *p;
    u32 max;
    u32 size;

    max = 0;
    for (p = (struct VMhead *)virtual_memory_pool; p != 0; p = p->next)
    {
        size = p->size;
        if (!(size & VMEM_BLOCK_IN_USE) && max < size)
        {
            max = size;
        }
    }
    return max << 2;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long vgetfreesize(void);
 *     VALLOC.C:61, 11 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *virtual_memory_pool;
 * END PSX.SYM */

unsigned long vgetfreesize(void)
{
    struct VMhead *p;
    u32 sum;

    sum = 0;
    for (p = (struct VMhead *)virtual_memory_pool; p != 0; p = p->next)
    {
        if (!(p->size & VMEM_BLOCK_IN_USE))
            sum += p->size;
    }
    return sum << 2;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void * vcalloc(unsigned long size, unsigned char c);
 *     VALLOC.C:203, 9 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long size
 *     param $a1       unsigned char c
 * END PSX.SYM */

void *vcalloc(u32 size, u8 c)
{
    void *allocation;

    allocation = valloc(size);
    memset(allocation, c, size);
    return allocation;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void vfree(void *pt);
 *     VALLOC.C:216, 27 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       void * pt
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *virtual_memory_pool;
 * END PSX.SYM */

void vfree(void *pt)
{
    struct VMhead *header;
    struct VMhead *next;
    struct VMhead *prev;
    struct VMhead *pnext;
    u32 mask;
    u32 sz;
    s32 s;

    if (pt == 0)
        return;

    header = (struct VMhead *)pt - 1;
    mask = VMEM_BLOCK_IN_USE;
    if ((header->size & mask) == 0)
        SystemOut(msg_double_memory_release);

    sz = header->size & VMEM_BLOCK_SIZE_MASK;
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
        pnext = prev->next;
        if (pnext != header)
        {
            prev = pnext;
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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long vsize(void *pt);
 *     VALLOC.C:247, 8 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       void * pt
 * END PSX.SYM */

unsigned long vsize(void *pt)
{
    return ((((struct VMhead *)pt) - 1)->size & VMEM_BLOCK_SIZE_MASK) << 2;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SystemOut(unsigned char *string);
 *     VALLOC.C:305, 3 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned char * string
 * END PSX.SYM */

void SystemOut(unsigned char *string)
{
    for (;;)
    {
    }
}
