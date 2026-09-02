#include "common.h"
#include "main.exe.h"
#include "vmemory.h"

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

extern int sprintf(char *buf, char *fmt, ...);

extern char msg_out_of_memory[]; /* "OUT OF MEMORY\nREQUEST=%d\nFREE=%d(%d)\n" */

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
