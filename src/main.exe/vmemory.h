#ifndef TENCHU_VMEMORY_H
#define TENCHU_VMEMORY_H

#include "ram_layout.h"

typedef struct PoolBlock
{
    s32 size; /* word count, sign bit reserved as an in-use flag */
    struct PoolBlock *next;
} PoolBlock;

extern PoolBlock *virtual_memory_pool;

/* Compiler output proves that these two spellings cannot currently be one
 * object while also satisfying both contracts.  Retail's apparent raw
 * constants emit LUI/ORI; an ABI symbol emits relocation-bearing LUI/ADDIU.
 * Keep that necessary build-lane choice here, while allocator functions use
 * the same human-facing VMEM_DEFAULT_* names. */
#ifdef TENCHU_RELOCATABLE
extern PoolBlock MemoryPool[];
extern u8 MemoryPoolCapacity[];
#define VMEM_DEFAULT_POOL MemoryPool
#define VMEM_DEFAULT_CAPACITY ((u32)MemoryPoolCapacity)
#else
#define VMEM_DEFAULT_POOL ((PoolBlock *)TENCHU_MEMORY_POOL_FLOOR)
#define VMEM_DEFAULT_CAPACITY TENCHU_RETAIL_MEMORY_POOL_CAPACITY
#endif

#endif /* TENCHU_VMEMORY_H */
