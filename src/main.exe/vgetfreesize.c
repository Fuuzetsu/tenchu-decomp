#include "common.h"
#include "main.exe.h"

/*
 * vgetfreesize (0x80016ce8, 0x44 bytes) — same TU as vinit.c
 * (virtual_memory_pool/valloc/vfree/vgetmaxsize/vgetfreesize/vcalloc all
 * cluster together): sums the `size` (word count) of every FREE block (top
 * bit clear — the in-use flag from vinit.c) in the pool's singly-linked free
 * list, returning the total in BYTES (word count << 2).
 */

typedef struct PoolBlock
{
    s32 size; /* word count, sign bit reserved as an in-use flag by valloc */
    struct PoolBlock *next;
} PoolBlock;

extern PoolBlock *virtual_memory_pool;

u32 vgetfreesize(void)
{
    PoolBlock *p;
    s32 sum;

    sum = 0;
    for (p = virtual_memory_pool; p != 0; p = p->next)
    {
        if (!(p->size & 0x80000000))
            sum += p->size;
    }
    return sum << 2;
}
