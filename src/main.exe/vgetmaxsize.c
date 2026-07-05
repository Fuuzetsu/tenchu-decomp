#include "common.h"
#include "main.exe.h"

/*
 * vgetmaxsize (0x80016c9c, 0x4c bytes) — same TU as vinit.c/vgetfreesize.c
 * (virtual_memory_pool/valloc/vfree/vgetmaxsize/vgetfreesize/vcalloc all
 * cluster together): walks the pool's singly-linked free list (see
 * vinit.c/vgetfreesize.c for PoolBlock) and returns the size of the LARGEST
 * free block (top bit clear), in BYTES (word count << 2).
 */

typedef struct PoolBlock
{
    s32 size; /* word count, sign bit reserved as an in-use flag by valloc */
    struct PoolBlock *next;
} PoolBlock;

extern PoolBlock *virtual_memory_pool;

u32 vgetmaxsize(void)
{
    PoolBlock *p;
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
