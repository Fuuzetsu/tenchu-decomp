#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void vfree(void *pt);
 *     VALLOC.C:216, 27 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       void * pt
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *virtual_memory_pool;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 9 of 260 bytes differ (whole-image: 9, function is
 * the RIGHT LENGTH — a pure register-color residual, no structural issue).
 *
 * vfree (0x80016d7c) — same TU as vinit.c/vgetfreesize.c/vsize.c/vcalloc.c
 * (virtual_memory_pool/valloc/vfree/vgetmaxsize/vgetfreesize/vcalloc all
 * cluster together): releases a block returned by valloc/vcalloc back to
 * the pool's singly-linked free list (see vinit.c for PoolBlock), then
 * coalesces it with its physically-adjacent neighbours in the list: the
 * block already pointed to by its own `next` (forward), and whichever
 * block's `next` points AT this one (backward, found by a linear walk
 * from the pool head — the list is otherwise unordered from vfree's point
 * of view).
 *
 * Matching notes:
 *  - The header address `(PoolBlock *)pt - 1` is computed unconditionally
 *    in the null-guard branch's delay slot but only used once the guard
 *    passes — ordinary "independent computation floats into the guard's
 *    delay slot", not a source-level rule.
 *  - The backward-merge search is a genuine `do { ... } while (prev != 0);`
 *    guarded by `if (virtual_memory_pool != 0)`: falling straight from the
 *    guard into the loop body with NO separate entry test is exactly the
 *    do-while shape (a `while` here would re-test the already-proven
 *    condition). Written as a hand-rolled `label:`/`goto` (the do-while
 *    KEYWORD form instead compiled to a "jump straight to the bottom
 *    test" shape — an EXTRA unconditional `j`, wrong instruction count;
 *    the label/goto form reproduces the target's straight fall-through
 *    into the loop body with no entry test, matching GetAreaMapLevel's
 *    "every loop here is a hand-rolled goto loop" precedent).
 *  - `header->size` is read/written as a plain `s32`. The forward-merge
 *    "is next free" test needs the LITERAL `(~next->size & mask)` spelling
 *    (Ghidra's own rendering) with `mask` a NAMED VARIABLE, not the inline
 *    literal `0x80000000` — an inline literal constant-folds the whole
 *    test back into a signed `bltz` (fold-const's sign-bit-test
 *    simplification), losing the real `nor+and` instructions.
 *  - **NEW RULE, confirmed here**: a call-crossing constant used only
 *    ONCE is cheap enough for cc1 to leave UNALLOCATED and re-materialize
 *    at its point of use (no persistent register, no prologue save) —
 *    this is what our EARLIER drafts did, and it cost 2 instructions
 *    (missing `sw s2`/`lw s2`) plus cascaded every downstream register
 *    color by one slot. Using the SAME named constant a SECOND time
 *    (the double-release guard, spelled `(header->size & mask) == 0`
 *    instead of the equally-valid `header->size >= 0`) was what tipped
 *    cc1 into actually allocating it a real callee-saved register
 *    (mask 0x80070000: ra, s0, s1, s2) — matching the target exactly and
 *    fixing the whole-image length. Confirmed by regalloc.py showing `$s2:
 *    (copy target)` only after this change.
 *  - Residual (9 bytes): the forward-merge block's `next`/`next->size`
 *    registers ($a0/$v1) and the backward-merge's final sum's result
 *    register ($v1) are swapped against the target — a pure allocator
 *    tie (autorules: no improving edit; regalloc.py: no copy-chain to a
 *    hard reg to break; one bounded `tools/permute.py` run plateaued at
 *    score 30, never reaching 0, in a CPU-contended shared run). Declaration
 *    order/extra temps (`n2`, a separate `nsz`) tried and made no
 *    difference — parked per the cookbook's sub-C-level early-stop.
 */

typedef struct PoolBlock
{
    s32 size; /* word count, sign bit reserved as an in-use flag by valloc */
    struct PoolBlock *next;
} PoolBlock;

extern PoolBlock *virtual_memory_pool;

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/vfree", vfree);
#else

extern char D_8001104C[]; /* "DOUBLE MEMORY RELEASE" — still referenced by
                              the unmatched vmemoryGC asm; reuse it rather
                              than opening a fresh .rodata (see LoadAreaMap.c) */
extern void SystemOut(char *);

void vfree(void *pt)
{
    PoolBlock *header;
    PoolBlock *next;
    PoolBlock *prev;
    PoolBlock *n2;
    u32 mask;
    u32 sz;
    s32 psz;

    if (pt == 0)
        return;

    header = (PoolBlock *)pt - 1;
    mask = 0x80000000;
    if ((header->size & mask) == 0)
        SystemOut(D_8001104C);

    sz = header->size & 0x7fffffff;
    header->size = sz;

    next = header->next;
    if (next != 0 && (~next->size & mask) != 0)
    {
        header->size = next->size + 2 + sz;
        header->next = next->next;
    }

    prev = virtual_memory_pool;
    if (prev != 0)
    {
    search:
        n2 = prev->next;
        if (n2 == header)
            goto found;
        prev = n2;
        if (prev != 0)
            goto search;

    found:
        if (prev != 0)
        {
            psz = prev->size;
            if (psz >= 0)
            {
                prev->size = header->size + 2 + psz;
                prev->next = header->next;
            }
        }
    }
}
#endif
