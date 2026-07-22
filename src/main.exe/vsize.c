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
    return (((struct VMhead *)pt) - 1)->size << 2;
}
