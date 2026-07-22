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

/*
 * vgetfreesize (0x80016ce8, 0x44 bytes) — same TU as vinit.c
 * (virtual_memory_pool/valloc/vfree/vgetmaxsize/vgetfreesize/vcalloc all
 * cluster together): sums the `size` (word count) of every FREE block (top
 * bit clear — the in-use flag from vinit.c) in the pool's singly-linked free
 * list, returning the total in BYTES (word count << 2).
 */

unsigned long vgetfreesize(void)
{
    VMhead *p;
    s32 sum;

    sum = 0;
    for (p = virtual_memory_pool; p != 0; p = p->next)
    {
        if (!(p->size & 0x80000000))
            sum += p->size;
    }
    return sum << 2;
}
