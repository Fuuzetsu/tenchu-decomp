#include "common.h"
#include "main.exe.h"
#include "vmemory.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * unsigned long vsize(void *pt);
 *     VALLOC.C:247, 8 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       void * pt
 * END PSX.SYM */

/*
 * vsize (0x80016e80, 0xc bytes) — same TU as vinit.c/vgetfreesize.c
 * (virtual_memory_pool/valloc/vfree/vgetmaxsize/vgetfreesize/vcalloc all
 * cluster together): given a pointer returned by valloc/vcalloc, reads the
 * `size` (word count) out of the VMhead header that immediately precedes
 * the payload and returns the allocation's size in BYTES.
 */

unsigned long vsize(void *pt)
{
    return ((((struct VMhead *)pt) - 1)->size & VMEM_BLOCK_SIZE_MASK) << 2;
}
