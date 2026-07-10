#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void * valloc(unsigned long size);
 *     VALLOC.C:76, 50 src lines, frame 1080 bytes, saved-reg mask 0x80030000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $t0       unsigned long size
 *     stack sp+24     struct VMhead vh
 *     reg   $a2       struct VMhead * vhp
 *     reg   $s1       unsigned long * vmpt
 *     stack sp+32     struct VMhead vh
 *     stack sp+40     unsigned char [1024] str
 *     reg   $a3       unsigned long maxsize
 *     reg   $a0       struct VMhead * vhp
 *     reg   $a1       unsigned long freesize
 *     reg   $v1       struct VMhead * vhp
 * END PSX.SYM */

/*
 * valloc (0x8001656c, 0x1CC bytes) — the free-list allocator's core routine.
 * Same TU as vinit.c/vfree.c/vcalloc.c/vgetmaxsize.c/vgetfreesize.c
 * (VALLOC.C). Lazily self-initializes the pool the way vinit(0,0) would,
 * rounds the request up to words, then walks the free list for the first
 * fit: exact-or-near (slack < 0x13 words) is marked in-use whole, bigger is
 * split (the tail becomes a fresh free block). On failure it walks the list
 * twice more (max free block, total free) to format the fatal "OUT OF
 * MEMORY" diagnostic for SystemOut, then returns the (NULL) cursor.
 *
 * Matching notes:
 *  - THE $s1 MECHANISM (this was the previous session's 2-instruction
 *    residual): the search cursor `vmpt` is ALSO the return value, and the
 *    function's very last statement is `return vmpt;` — AFTER the sprintf/
 *    SystemOut calls (the target's `addu $v0,$s1,$zero` at 0x80016720).
 *    That final use keeps vmpt live across both calls, which is the entire
 *    reason cc1 gives it callee-saved $s1 (and the extra sw/lw pair). The
 *    early exit is `if (vmpt) goto done;` to the shared `done: return
 *    vmpt;` — that compiles to the target's 2-insn `bnez $s1,epilogue`
 *    with `move $v0,$s1` in the delay slot; a literal early `return vmpt;`
 *    instead compiles branch-AROUND (beqz + j + nop, 2 insns long). A draft
 *    that ends at SystemOut() has no call-crossing use, so the cursor lands
 *    caller-saved and the whole loop's registers cascade off-target.
 *  - THE SPLIT-ARM FRESH RELOADS (cse.c follow-jumps, root-caused in the
 *    gcc-2.8.1 sources): the split arm must reload `vhp->size`/`vhp->next`
 *    fresh (lw/subu/addiu -2 through the $a0 copy) even though the tests
 *    just loaded *vmpt. cse's path-following (cse_end_of_basic_block)
 *    follows a conditional branch's taken edge whenever the target label
 *    has NUSES==1 and is preceded by a BARRIER — which the natural
 *    if/else+break shape always satisfies, so cse merges the split arm's
 *    loads with the tests' values (and a post-cse jump_optimize then also
 *    inverts the branch and moves the near-fit arm to the function end).
 *    The scan is blocked by a NOTE_INSN_LOOP_END sitting between that
 *    barrier and the label: hence the dummy do{}while(0) around the
 *    near-fit arm, whose loop notes land exactly there. With the follow
 *    blocked, the split arm compiles as its own fresh basic block —
 *    byte-exact, including the whole loop's register assignment.
 *  - Stack layout: the split-tmp `vh` is the OUTER declaration (sp+0x18),
 *    the self-init `vh` is a nested-block shadow inside the `if` (sp+0x20),
 *    and `str` lives in a later nested block (sp+0x28) — matching PSX.SYM's
 *    two `vh` scopes at sp+24/sp+32 and cc1's in-encounter-order slot
 *    assignment.
 *  - The loop reads its tests through `vmpt` (lw 0($s1)) but takes a copy
 *    `vhp = (PoolBlock *)vmpt;` at loop top ($a0, scheduled into the first
 *    branch's delay slot): the split arm's loads/stores and the advance
 *    (`vmpt = vhp->next`) go through the copy.
 *  - `maxsize <<= 2;` is a real source statement between the two tail loops
 *    (after `freesize = 0;`): its `sll` is what reorg steals into the second
 *    loop guard's delay slot; sprintf then receives `maxsize` raw.
 *  - The self-init branch duplicates vinit.c's `size == 0` arm literally
 *    (pool = 0x800DC000; vh.size = 0x47ffe; vh.next = 0) — no call, cc1
 *    2.8.1 does not inline.
 *  - The request rounding is `if (size & 3) size += 4;` (NOT `(size+3)&~3`)
 *    — read off the raw immediate; keep the odd shape.
 *  - Both whole-struct stores (`*virtual_memory_pool = vh;`, `*(PoolBlock *)
 *    vmpt = vh;`) are aggregate assignments: two field stores to the stack
 *    slot, then the $t1/$t2 reload+store block copy (vinit.c's idiom).
 */

typedef struct PoolBlock
{
    s32 size; /* word count, sign bit reserved as an in-use flag by valloc */
    struct PoolBlock *next;
} PoolBlock;

extern PoolBlock *virtual_memory_pool;

extern int sprintf(char *buf, char *fmt, ...);
extern void SystemOut(u8 *string);

extern char D_80011024[]; /* "OUT OF MEMORY\nREQUEST=%d\nFREE=%d(%d)\n" — pooled
                              right before vfree.c's D_8001104C ("DOUBLE MEMORY
                              RELEASE") in this TU's rodata */

void *valloc(u32 size)
{
    PoolBlock vh;   /* split tmp, sp+0x18 */
    PoolBlock *vhp; /* search-loop copy of the cursor */
    u32 *vmpt;      /* cursor AND result — returned after SystemOut, hence $s1 */
    u32 off;
    u32 mask;
    u32 tag;

    if (virtual_memory_pool == 0)
    {
        PoolBlock vh; /* nested shadow, sp+0x20 */

        virtual_memory_pool = (PoolBlock *)0x800DC000;
        vh.size = 0x47ffe;
        vh.next = 0;
        *virtual_memory_pool = vh;
    }

    if ((size & 3) != 0)
        size = size + 4;
    size = size >> 2;

    vmpt = (u32 *)virtual_memory_pool;
    if (vmpt != 0)
    {
        off = (size << 2) + 8;
        mask = 0x80000000;
        tag = size | mask;
        do
        {
            vhp = (PoolBlock *)vmpt;
            if (!(vmpt[0] & mask) && size <= vmpt[0])
            {
                if (vmpt[0] - size < 0x13)
                {
                    /* The do{}while(0) is LOAD-BEARING: its LOOP_END note
                     * lands between this arm's `goto` and the split arm's
                     * label, which blocks cse.c's follow-jumps path scan
                     * (cse_end_of_basic_block breaks at NOTE_INSN_LOOP_END)
                     * — that is what keeps the split arm's vhp-> reads as
                     * FRESH loads (lw/subu/addiu) like the target instead
                     * of CSE reusing the test's loaded value/slack. */
                    do
                    {
                        vmpt[0] = vmpt[0] | mask;
                        vmpt = vmpt + 2;
                        goto found;
                    } while (0);
                }
                else
                {
                    vh.size = vhp->size - size - 2;
                    vh.next = vhp->next;
                    vmpt = (u32 *)((u8 *)vmpt + off);
                    vhp->size = tag;
                    vhp->next = (PoolBlock *)vmpt;
                    *(PoolBlock *)vmpt = vh;
                    vmpt = (u32 *)(vhp + 1);
                }
                break;
            }
            vmpt = (u32 *)vhp->next;
        } while (vmpt != 0);
    found:
        if (vmpt != 0)
            goto done; /* bnez straight to the shared epilogue-return */
    }

    {
        u8 str[1024]; /* sp+0x28 */
        u32 maxsize;
        u32 freesize;
        PoolBlock *p;
        PoolBlock *q;

        maxsize = 0;
        for (p = virtual_memory_pool; p != 0; p = p->next)
        {
            if (!(p->size & 0x80000000) && maxsize < (u32)p->size)
                maxsize = p->size;
        }

        freesize = 0;
        maxsize <<= 2;
        for (q = virtual_memory_pool; q != 0; q = q->next)
        {
            if (!(q->size & 0x80000000))
                freesize += q->size;
        }

        sprintf((char *)str, D_80011024, size << 2, maxsize, freesize << 2);
        SystemOut(str);
    }
done:
    return vmpt;
}
