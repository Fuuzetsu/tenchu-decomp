#include "common.h"
#include "main.exe.h"

/*
 * vinit (0x80016c48) — initializes the "virtual memory" free-list allocator
 * (the valloc/vfree/vgetmaxsize/vgetfreesize family: valloc is at 0x8001656c,
 * right after vgetfreesize at 0x80016ce8). Installs `addr` (or the default
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
 *    `*virtual_memory_pool` via reload+store — the classic whole-struct
 *    assignment shape, not two direct field stores through the pointer
 *    (which would skip the stack roundtrip entirely). The destination
 *    pointer is read directly (no separate `p` local caching it): it changes
 *    across the addr==0 branch, so cc1 cannot keep it live in a register
 *    across that join and reloads gp_rel fresh at the point of use.
 */

typedef struct PoolBlock
{
    s32 size; /* word count, sign bit reserved as an in-use flag by valloc */
    struct PoolBlock *next;
} PoolBlock;

extern PoolBlock *virtual_memory_pool;

void vinit(void *addr, u32 size)
{
    PoolBlock h;

    virtual_memory_pool = addr;
    if (addr == 0)
        virtual_memory_pool = (PoolBlock *)0x800DC000;

    if (size != 0)
        h.size = (size >> 2) - 2;
    else
        h.size = 0x47ffe;
    h.next = 0;
    *virtual_memory_pool = h;
}
