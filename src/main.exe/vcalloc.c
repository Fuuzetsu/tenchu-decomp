#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void * vcalloc(unsigned long size, unsigned char c);
 *     VALLOC.C:203, 9 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned long size
 *     param $a1       unsigned char c
 * END PSX.SYM */

extern void *valloc(u32 size);
extern void *memset(void *s, int c, u32 n);

void *vcalloc(u32 size, u8 c)
{
    void *allocation;

    allocation = valloc(size);
    memset(allocation, c, size);
    return allocation;
}
