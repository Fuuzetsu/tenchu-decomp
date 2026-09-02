#include "common.h"
#include "main.exe.h"
#include "vmemory.h"

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
