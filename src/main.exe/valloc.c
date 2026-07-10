#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void * valloc(unsigned long size);
 *     VALLOC.C:76, 50 src lines, frame 1080 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *virtual_memory_pool;
 * END PSX.SYM */

/*
 * STATUS: NON_MATCHING — 356 of 460 bytes differ, but the function is only
 * TWO INSTRUCTIONS (8 bytes) short (112 vs the target's 114): the search
 * loop's cursor/result pointer (Ghidra's puVar3/puVar6, one register in the
 * real binary) is callee-saved ($s1, an extra sw/lw pair) in the target but
 * lands in a caller-saved register ($v1) here, and that one difference
 * cascades into every downstream register in the loop (the 63-line diff
 * below is almost entirely this one pseudo's ripple, not independent bugs).
 *
 * regalloc.py's own dump says a value only lands in $s0-$s7 by surviving a
 * call — this pseudo does NOT (it's dead well before the sprintf/SystemOut
 * calls; the early "found" return jumps straight to the shared epilogue,
 * around them). Yet cc1's global.c allocates strictly by priority
 * (floor_log2(n_refs)*n_refs/live_length*loop_depth), independent of the
 * call-crossing test, and `-dl` confirms this pseudo has by far the highest
 * priority in the function of anything NOT already forced callee-saved
 * (46 refs/53 insns when the 3 list-walk loops share one cursor variable;
 * 26 refs/31 insns confined to just the search loop after splitting the
 * cursor per loop — tried both, neither crossed whatever threshold buys it
 * $s1). Attempted and rejected: sharing one cursor across all 3 loops vs a
 * separate named cursor per loop (the latter is right — it drops the diff
 * from 76 to 63 lines and matches the target using 3 DIFFERENT registers
 * for the 3 walks, s1/a0/v1, which a single shared C variable could not
 * explain); merging the found-pointer into the cursor variable vs keeping
 * them separate (byte-identical either way — cc1 already coalesces the
 * non-overlapping live ranges); a do{}while(0) wrapper at two different
 * nesting depths to add loop_depth weight (no effect, at either depth);
 * reordering the null-check/assignment (`vhp = pool; if (vhp)` vs `if
 * (pool) { vhp = pool; ...}`, the latter matches — the former is 1
 * instruction SHORTER, moving further from target, so it's parked reverted).
 * Whatever tips the original's cc1 into spending a callee-saved register on
 * a value that doesn't need one is a real mechanism this session didn't
 * isolate — worth another pass with `-dg`'s full RTL if picked back up.
 *
 * The boundary fix below (config/splat.main.exe.yaml, config/symbols) is
 * independent of this residual and already verified: `./Build check` is
 * green with the stub at the corrected 0x1CC-byte length.
 *
 * valloc (0x8001656c, 0x1CC bytes — the splat/Ghidra carve was 4 bytes SHORT
 * at 0x1C8/456: our cc1 always reorgs the epilogue's `addiu sp,sp,+0x438`
 * into `jr ra`'s own delay slot, so the true end is 0x80016738, exactly
 * where vrealloc's own `addiu sp,sp,-0x28` prologue begins. The missing word
 * had been carved as an anonymous 4-byte `data` entry in
 * config/splat.main.exe.yaml between valloc and vrealloc; removed here so
 * valloc's carve runs directly into vrealloc's, per the cookbook's
 * "under-sized functions.tsv entry" rule (AdtVsprintf/FUN_8005fe38
 * precedent). Ghidra's own functions.tsv reports the same short 456, so this
 * wasn't just a splat/symbols.main.exe.txt slip — Ghidra's own analysis
 * stopped at the bare `jr ra` too) — same TU as vinit.c/vfree.c/vcalloc.c/
 * vgetmaxsize.c/vgetfreesize.c (virtual_memory_pool/valloc/vfree/
 * vgetmaxsize/vgetfreesize/vcalloc/vrealloc all cluster together in this
 * address range): the free-list allocator's core routine. Lazily
 * self-initializes the pool exactly the way vinit(0, 0) would (see
 * vinit.c's own default-pool branch — same idiom: assign the pool pointer,
 * then build a whole PoolBlock header on the stack and copy it in one
 * aggregate assignment), then searches the free list for the first block
 * big enough for the (rounded-up) request: an exact-or-near fit (slack
 * under 0x13 words) is marked in-use whole, a bigger block is split in two
 * (the tail becomes a fresh free block). If nothing fits, it walks the list
 * twice more (max single free block, total free) purely to format the fatal
 * "OUT OF MEMORY" diagnostic and hand it to SystemOut (which never
 * returns — see SystemOut.c).
 *
 * Matching notes:
 *  - The self-init branch is a literal duplicate of vinit.c's `size == 0`
 *    arm (pool = 0x800DC000; h.size = 0x47ffe; h.next = 0), not a call —
 *    there is no `jal vinit` in the asm, and cc1 2.8.1 does not inline.
 *  - After the self-init `if`, `virtual_memory_pool` is reloaded fresh
 *    (gp_rel `lw`) rather than kept live across the branch join — same
 *    "pointer changes across the branch, so it can't stay in a register"
 *    shape documented in vinit.c. The subsequent `if (virtual_memory_pool
 *    != 0)` is therefore provably-true dead code from our point of view,
 *    but the compiler (and Ghidra) render it as a real guard — it's real
 *    source, not something to fold away.
 *  - The request-size rounding is `if (size & 3) size += 4;` (NOT the usual
 *    `(size + 3) & ~3`) — a quirky but literal rounding read straight off
 *    the raw immediate; keep the odd shape.
 *  - The split-block tail write is a whole-`PoolBlock`-struct assignment
 *    (`*newblock = tmp;`), matching vinit.c's block-copy-via-reload idiom
 *    (two field stores to a stack-resident aggregate, then two reloads and
 *    two stores through the destination pointer) rather than two direct
 *    field stores through the pointer.
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

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/valloc", valloc);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/valloc", allocate_memory_in_mem_pool___override__prt_80016710_891e8130);
#else

void *valloc(u32 size)
{
    PoolBlock *vhp;
    PoolBlock *p;
    PoolBlock *q;
    PoolBlock *nb;
    PoolBlock vh;
    PoolBlock tmp;
    u32 mask;
    u32 tag;
    u32 off;
    u32 max;
    s32 sum;
    u8 str[1024];
    u8 *msg;

    if (virtual_memory_pool == 0)
    {
        virtual_memory_pool = (PoolBlock *)0x800DC000;
        vh.size = 0x47ffe;
        vh.next = 0;
        *virtual_memory_pool = vh;
    }

    if ((size & 3) != 0)
        size = size + 4;
    size = size >> 2;

    if (virtual_memory_pool != 0)
    {
        vhp = virtual_memory_pool;
        off = (size << 2) + 8;
        mask = 0x80000000;
        tag = size | mask;
        do
        {
            if (!(vhp->size & mask) && size <= (u32)vhp->size)
            {
                if ((u32)(vhp->size - size) < 0x13)
                {
                    vhp->size = vhp->size | mask;
                    vhp = vhp + 1;
                }
                else
                {
                    tmp.size = (vhp->size - size) - 2;
                    tmp.next = vhp->next;
                    nb = (PoolBlock *)((u8 *)vhp + off);
                    vhp->size = tag;
                    vhp->next = nb;
                    *nb = tmp;
                    vhp = vhp + 1;
                }
                break;
            }
            vhp = vhp->next;
        } while (vhp != 0);
        if (vhp != 0)
            return vhp;
    }

    max = 0;
    for (p = virtual_memory_pool; p != 0; p = p->next)
    {
        if (!(p->size & 0x80000000) && max < (u32)p->size)
            max = p->size;
    }

    sum = 0;
    for (q = virtual_memory_pool; q != 0; q = q->next)
    {
        if (!(q->size & 0x80000000))
            sum += q->size;
    }

    msg = str;
    sprintf((char *)msg, D_80011024, size << 2, max << 2, sum << 2);
    SystemOut(msg);
}
#endif
