#include "common.h"
#include "main.exe.h"
#include "vmemory.h"

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
