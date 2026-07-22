#include "common.h"
#include "main.exe.h"
#include "vmemory.h"

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

/*
 * vgetmaxsize (0x80016c9c, 0x4c bytes) — same TU as vinit.c/vgetfreesize.c
 * (virtual_memory_pool/valloc/vfree/vgetmaxsize/vgetfreesize/vcalloc all
 * cluster together): walks the pool's singly-linked free list (see
 * vinit.c/vgetfreesize.c for VMhead) and returns the size of the LARGEST
 * free block (top bit clear), in BYTES (word count << 2).
 */

unsigned long vgetmaxsize(void)
{
    VMhead *p;
    u32 max;
    u32 size;

    max = 0;
    for (p = virtual_memory_pool; p != 0; p = p->next)
    {
        size = p->size;
        if (!(size & 0x80000000) && max < size)
        {
            max = size;
        }
    }
    return max << 2;
}
