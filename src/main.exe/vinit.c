#include "common.h"
#include "main.exe.h"
#include "vmemory.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void vinit(void *adr, unsigned long size);
 *     VALLOC.C:33, 8 src lines, frame 8 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
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
 *     param $a0       void * adr
 *     param $a1       unsigned long size
 *     stack sp+0      struct VMhead vh
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *virtual_memory_pool;
 * END PSX.SYM */

/*
 * vinit (0x80016c48) — initializes the "virtual memory" free-list allocator
 * (the valloc/vfree/vgetmaxsize/vgetfreesize family: valloc is at 0x8001656c,
 * right after vgetfreesize at 0x80016ce8). Installs `adr` (or the default
 * MemoryPool buffer when NULL) as the pool base and writes a single free
 * block header covering the whole pool: size in words (derived from `size`,
 * or a hardcoded default word count when size==0) and a null `next` link.
 *
 * Matching notes:
 *  - virtual_memory_pool is %gp_rel (D_800976A0) — small, TU-local (this is
 *    that original TU).
 *  - The default-pool address materializes as `lui/ori` (0x800DC000), NOT
 *    `lui/addiu` — that's the tell for a raw hardcoded pointer CONSTANT in
 *    source, not a relocatable symbol reference (a real extern-symbol `la`
 *    would need addiu so the assembler's %hi/%lo pair absorbs the
 *    sign-extension; ori only appears for a plain integer/address literal).
 *    Ghidra's "MemoryPool" name (used elsewhere, e.g. valloc) is a label for
 *    the same address, not proof this call site referenced that symbol.
 *    Confirmed by m2c: `D_800976A0 = (void *)0x800DC000;`.
 *  - The polarity of the size test is flipped from Ghidra's rendering:
 *    Ghidra shows `if (size==0) big=default; else big=computed;`, but the
 *    real asm's `beqz` branches to the CONSTANT arm — i.e. the constant is
 *    the jumpifnot/ELSE arm, so the source condition is `size != 0`
 *    (computed first/fallthrough, default second/jumped-to). m2c agrees:
 *    `if (arg1 != 0) { ... } else { ... }`.
 *  - The two header fields (size, next) are one aggregate local: cc1 stores
 *    each field to its OWN stack slot (an 8-byte frame — matches the
 *    `addiu $sp,$sp,-8` prologue), then block-copies both words into
 *    `*(struct VMhead *)virtual_memory_pool` via reload+store — the classic
 *    whole-struct
 *    assignment shape, not two direct field stores through the pointer
 *    (which would skip the stack roundtrip entirely). The destination
 *    pointer is read directly (no separate `p` local caching it): it changes
 *    across the addr==0 branch, so cc1 cannot keep it live in a register
 *    across that join and reloads gp_rel fresh at the point of use.
 *  - VMEM_DEFAULT_POOL/VMEM_DEFAULT_CAPACITY name the central retail policy
 *    constants which compiler output proves were used here.  The normal-link
 *    build keeps this one source and changes only the resulting LUI/ORI pairs
 *    to standard relocation-bearing LUI/ADDIU pairs; the function's register
 *    allocation, schedule, and 0x54-byte size stay unchanged.
 */

void vinit(void *adr, u32 size)
{
    struct VMhead h;

    virtual_memory_pool = adr;
    if (adr == 0)
        virtual_memory_pool = VMEM_DEFAULT_POOL;

    if (size != 0)
        h.size = (size >> 2) - 2;
    else
        h.size = VMEM_DEFAULT_CAPACITY;
    h.next = 0;
    *(struct VMhead *)virtual_memory_pool = h;
}
