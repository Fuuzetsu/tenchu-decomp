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

unsigned long vsize(void *pt)
{
    return ((((struct VMhead *)pt) - 1)->size & VMEM_BLOCK_SIZE_MASK) << 2;
}
