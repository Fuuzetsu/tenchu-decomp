#ifndef TENCHU_VMEMORY_H
#define TENCHU_VMEMORY_H

#include "ram_layout.h"

/* VALLOC.C's free-list header, recovered verbatim from PSX.SYM. */
typedef struct VMhead
{
    u32 size; /* word count, high bit reserved as an in-use flag */
    struct VMhead *next;
} VMhead;

extern VMhead *virtual_memory_pool;
extern unsigned long vgetmaxsize(void);
extern unsigned long vgetfreesize(void);
extern unsigned long vsize(void *pt);

/* Compiler output proves the original used integer constants: these spellings
 * produce retail's LUI/ORI pairs in both allocator functions.  The normal-link
 * build does not change the C source; tools/reloc_c_literals.py replaces only
 * this exact LUI/ORI materialisation with the ABI's relocation-bearing
 * LUI/ADDIU spelling, preserving cc1's register allocation and schedule while
 * following linker-owned pool policy. */
#define VMEM_DEFAULT_POOL ((VMhead *)TENCHU_MEMORY_POOL_FLOOR)
#define VMEM_DEFAULT_CAPACITY TENCHU_RETAIL_MEMORY_POOL_CAPACITY

#endif /* TENCHU_VMEMORY_H */
