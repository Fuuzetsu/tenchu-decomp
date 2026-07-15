#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void * vcalloc(unsigned long size, unsigned char c);
 *     VALLOC.C:203, 9 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       unsigned long size
 *     param $a1       unsigned char c
 * END PSX.SYM */

/*
 * vcalloc (0x80016d2c) — calloc-shaped wrapper over the virtual allocator
 * (same TU as vinit.c: virtual_memory_pool/valloc/vfree/vgetmaxsize/
 * vgetfreesize/vcalloc all cluster together in this address range):
 * allocate `size` bytes from the pool, then fill them with byte `c`.
 */
extern void *valloc(u32 size);
extern void *memset(void *s, int c, u32 n);

void *vcalloc(u32 size, u8 c)
{
    void *p;

    p = valloc(size);
    memset(p, c, size);
    return p;
}
