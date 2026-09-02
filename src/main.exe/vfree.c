#include "common.h"
#include "main.exe.h"
#include "vmemory.h"

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

extern char msg_double_memory_release[]; /* "DOUBLE MEMORY RELEASE" */

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
        if (pnext == header)
            goto found;
        prev = pnext;
        if (prev != 0)
            goto search;

    found:
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
