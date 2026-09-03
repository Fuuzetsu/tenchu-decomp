#ifndef TENCHU_VMEMORY_H
#define TENCHU_VMEMORY_H

#include "ram_layout.h"

/* VALLOC.C's free-list header and typedef, recovered from PSX.SYM. */
struct VMhead
{
    u32 size; /* word count, high bit reserved as an in-use flag */
    struct VMhead *next;
};
typedef struct VMhead VMheadType;

#define VMEM_BLOCK_IN_USE 0x80000000u
#define VMEM_BLOCK_SIZE_MASK (~VMEM_BLOCK_IN_USE)
#define VMEM_HEADER_WORDS (sizeof(struct VMhead) / sizeof(u32))
#define VMEM_HEADER_BYTES sizeof(struct VMhead)

extern u_long *virtual_memory_pool;
void *valloc(u32 size);
void *vrealloc(void *allocation, u32 size);
void *vmemoryGC(void *allocation);
void vinit(void *address, u32 size);
void *vcalloc(u32 size, u8 value);
void vfree(void *allocation);
unsigned long vgetmaxsize(void);
unsigned long vgetfreesize(void);
unsigned long vsize(void *allocation);
void SystemOut(unsigned char *message);

/* Compiler output proves the original used integer constants: these spellings
 * produce retail's LUI/ORI pairs in both allocator functions.  The normal-link
 * build does not change the C source; tools/reloc_c_literals.py replaces only
 * this exact LUI/ORI materialisation with the ABI's relocation-bearing
 * LUI/ADDIU spelling, preserving cc1's register allocation and schedule while
 * following linker-owned pool policy. */
#define VMEM_DEFAULT_POOL ((u_long *)TENCHU_MEMORY_POOL_FLOOR)
#define VMEM_DEFAULT_CAPACITY TENCHU_RETAIL_MEMORY_POOL_CAPACITY

/* valloc: leftover slack smaller than this many words is not worth
 * splitting off as a free block. */
#define VMEM_MIN_SPLIT_SLACK 0x13
#define VMEM_MIN_GROW_SPLIT_SLACK \
    (VMEM_MIN_SPLIT_SLACK - VMEM_HEADER_WORDS)

#endif /* TENCHU_VMEMORY_H */
