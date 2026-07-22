#include "common.h"
#include "main.exe.h"
#include "vmemory.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void vfree(void *pt);
 *     VALLOC.C:216, 27 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *virtual_memory_pool;
 * END PSX.SYM */

/*
 * vfree (0x80016d7c) — same TU as vinit.c/vgetfreesize.c/vsize.c/vcalloc.c
 * (virtual_memory_pool/valloc/vfree/vgetmaxsize/vgetfreesize/vcalloc all
 * cluster together): releases a block returned by valloc/vcalloc back to
 * the pool's singly-linked free list (see vinit.c for `struct VMhead`), then
 * coalesces it with its physically-adjacent neighbours in the list: the
 * block already pointed to by its own `next` (forward), and whichever
 * block's `next` points AT this one (backward, found by a linear walk
 * from the pool head — the list is otherwise unordered from vfree's point
 * of view).
 *
 * Matching notes (the last 9 bytes were a register-allocation tie, solved
 * with cc1 -dg/-dl RTL dumps run standalone — not by respelling the C):
 *  - **`s` is ONE variable deliberately reused for BOTH neighbours' sizes**
 *    (the forward merge's `next->size` and the backward merge's
 *    `prev->size`) — this is what cracked the 9-byte residual. As two
 *    separate locals the pseudos score global-alloc priorities 5000
 *    (nsize, 3 refs/6 insns) and 7500 (psz, 3/4), both BELOW `next`'s 8000
 *    (4/10), so `next` is allocated first and takes $v1; the target needs
 *    it in $a0. Merged, the shared pseudo's disjoint ranges sum to 6
 *    refs/10 insns = priority 12000 (floor_log2(6)=2), outranking `next`:
 *    it takes $v1 in both regions (both target loads are `lw v1,0(a0)`),
 *    `next` is pushed to $a0, `sz` to $a1 — the exact target coloring, and
 *    robust (the demo build allocates identically). Diagnosed by reading
 *    priorities from `-dl` ("Register N used X times across Y insns") and
 *    the allocation order from `-dg`'s "regs to allocate" line.
 *  - Both coalescing sums must be spelled `A + (B + 2)` (or equivalently
 *    `+=`): fold-const canonicalizes `A + (B + 2)` to `(A + 2) + B`, which
 *    puts the `addiu A,2` first (it lands in the guard's delay slot) and
 *    ties the addu's DEST to A's dying register (dest = op0). The previous
 *    draft's `B + 2 + A` spelling mirrored the operand order and moved the
 *    tail sum from $v1 to $v0.
 *  - The header address `(struct VMhead *)pt - 1` is computed unconditionally
 *    in the null-guard branch's delay slot but only used once the guard
 *    passes — ordinary "independent computation floats into the guard's
 *    delay slot", not a source-level rule.
 *  - The mixed addressing is the named-`header`-local shape: `header->size`
 *    compiles to `-8($s1)` (cse rewrites it through `pt`) while
 *    `header->next` stays `4($s0)` — a raw-expression spelling with NO
 *    header local instead folds EVERYTHING to $s1-relative, loses the
 *    `addiu s0,s1,-8` temp, and drops to 62 instructions (wrong shape).
 *  - The backward-merge search is a hand-rolled `search:`/`goto` loop (the
 *    do-while KEYWORD form emits an extra unconditional `j`; the label/goto
 *    form reproduces the straight fall-through into the loop body).
 *  - The forward-merge "is next free" test needs the LITERAL `(~s & mask)`
 *    spelling with `mask` a NAMED VARIABLE, not the inline literal
 *    0x80000000 — an inline literal constant-folds the whole test back
 *    into a signed branch, losing the real `nor+and` instructions.
 *  - `mask`'s SECOND use (the double-release guard `(header->size & mask)
 *    == 0`, which combine still folds to `bltz`) is what tips cc1 into
 *    allocating the call-crossing constant a real callee-saved register
 *    ($s2) instead of rematerializing it at its single use.
 *  - PSX.SYM anomaly: the SYM records ZERO locals for vfree (param only),
 *    yet the demo binary is instruction-identical to retail and this
 *    matched source needs seven — and a genuinely local-free spelling
 *    compiles to a different shape (see above). The SYM's local list can
 *    under-record; treat a zero-locals record as unverified.
 */

extern char D_8001104C[]; /* "DOUBLE MEMORY RELEASE" — still referenced by
                              the unmatched vmemoryGC asm; reuse it rather
                              than opening a fresh .rodata (see LoadAreaMap.c) */

void vfree(void *pt)
{
    struct VMhead *header;
    struct VMhead *next;
    struct VMhead *prev;
    struct VMhead *n2;
    u32 mask;
    u32 sz;
    s32 s;

    if (pt == 0)
        return;

    header = (struct VMhead *)pt - 1;
    mask = 0x80000000;
    if ((header->size & mask) == 0)
        SystemOut(D_8001104C);

    sz = header->size & 0x7fffffff;
    header->size = sz;

    next = header->next;
    if (next != 0)
    {
        s = next->size;
        if ((~s & mask) != 0)
        {
            header->size = sz + (s + 2);
            header->next = next->next;
        }
    }

    prev = (struct VMhead *)virtual_memory_pool;
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
            s = prev->size;
            if (s >= 0)
            {
                prev->size = s + (header->size + 2);
                prev->next = header->next;
            }
        }
    }
}
