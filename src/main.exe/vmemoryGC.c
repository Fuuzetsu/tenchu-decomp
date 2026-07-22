#include "common.h"
#include "main.exe.h"
#include "vmemory.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void * vmemoryGC(void *pt);
 *     VALLOC.C:259, 40 src lines, frame 56 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
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
 *     param $s2       void * pt
 *     reg   $s3       struct VMhead * vhp
 *     reg   $a3       struct VMhead * svhp
 *     stack sp+16     struct VMhead vh
 *     reg   $s1       unsigned long * vmpt
 *     reg   $s6       unsigned long size
 *     reg   $s2       void * pt
 *     reg   $s2       void * pt
 *     reg   $a0       struct VMhead * svhp
 *     reg   $s0       struct VMhead * vhp
 *     reg   $s1       void * pt
 *     reg   $a0       struct VMhead * svhp
 *     reg   $s0       struct VMhead * vhp
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *virtual_memory_pool;
 * END PSX.SYM */

extern char D_8001104C[]; /* "DOUBLE MEMORY RELEASE" */
extern void *valloc(u32 size);
extern void *memcpy(void *dst, void *src, u32 n);

static inline void FreePoolBlockInline(void *pt, u32 cmask)
{
    PoolBlock *header;
    PoolBlock *next;
    PoolBlock *prev;
    PoolBlock *n2;
    u32 mask;
    u32 sz;
    s32 s;

    if (pt == 0)
        return;

    do
    {
        header = (PoolBlock *)pt - 1;
    } while (0);
    mask = 0x80000000;
    if ((header->size & mask) == 0)
        SystemOut(D_8001104C);

    sz = header->size & cmask;
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
            s = prev->size;
            if (s >= 0)
            {
                prev->size = s + (header->size + 2);
                prev->next = header->next;
            }
        }
    }
}

/*
 * vmemoryGC — same TU/family as valloc.c/vfree.c/vrealloc.c: a compacting
 * "garbage collector" over the free-list pool. valloc()s a fresh block the
 * same size as pt's current block; if the fresh block landed at a LOWER
 * address, moves pt's data there and frees the old pt block (real
 * compaction) and returns the new address. Otherwise the fresh block is no
 * improvement, so it's freed right back and instead the function tries to
 * slide pt DOWN into the free block immediately preceding it in the pool
 * list (found by the same linear walk vfree.c uses), returning the moved
 * (or, on failure, unchanged) address.
 *
 * Matching notes: no `jal vfree` appears anywhere in the target. A single
 * inline helper containing vfree.c's already-matched free+coalesce body is
 * expanded once for `pt` and once for `vmpt`. This is more than source
 * cleanup: the helper's distinct formal/local identities recover the target
 * full-width neighbour loads and reduce the exact-length residual from 241
 * to 96 bytes. The repeated `pt`/`vhp`/`svhp` debug locals are consistent
 * with those two inline expansions. Each expansion keeps vfree's leading
 * null guard; jump2 threads the first one into the following `return vmpt`
 * and the second one into the final compaction block.
 *  - PSX.SYM calls the outer allocation result `vmpt` in $s1 and records only
 *    one outer `svhp` in $a3. Reusing `vmpt` for the final split-tail pointer,
 *    then reusing the final search cursor for `vh.next` after memcpy, carries
 *    those source identities through both disjoint ranges. This fixes the
 *    entire old saved-register cycle and final caller-register tail.
 *  - One zero-trip wrapper around ONLY the inline helper's header assignment
 *    adds no code but gives both expanded header pseudos the one extra weighted
 *    reference needed for target header=$s0 / mask=$s4. With the source reuse
 *    above it replaces the old three-wrapper first-call workaround and reduces
 *    the exact-length residual from 41 to 24 bytes.
 *  - One zero-trip wrapper around the final mask definition keeps that value
 *    across memcpy and restores the exact 195-instruction extent (without it
 *    the function is 776 bytes); it leaves no branch in the optimized asm.
 *    Both surviving wrappers were re-challenged at MATCH and are load-bearing:
 *    unwrapping the helper's header assignment costs 25 bytes. A third wrapper
 *    around the final `vh.size` sum, previously believed to resolve a two-byte
 *    caller-register tie, was re-tested at MATCH and is NOT load-bearing --
 *    removed.
 *  - Parenthesizing the final offset as `base + ((hsz << 2) + 8)` selects the
 *    target addiu-before-addu tree (24 to 16 bytes); reusing `prev` after the
 *    call for `vh.next` closes the final $a0->$a3 tail (16 to 12 bytes).
 *  - Both coalescing sums are vfree.c's proven `A + (B + 2)` spelling.
 *  - **`size = (header->size & cmask) << 2` is what closed the last 12 bytes**
 *    (a saved-register swap: complement mask $s6->$s5, byte size $s5->$s6).
 *    The `and` is NOT in the target and is not meant to be: `<< 2` discards
 *    bit 31 regardless, so combine's force_to_mode deletes it. But flow.c has
 *    already counted the reference, and global.c:604 scores an allocno
 *    `floor_log2(n_refs) * n_refs / live_length`. That fourth ref carries
 *    cmask over the floor_log2 3->4 step (numerator 3 -> 8): 810 -> 2162,
 *    above `size`'s 1194, so cmask is allocated first and takes $s5. It is
 *    also the honest spelling — the header's `size` field reserves its sign
 *    bit as an in-use flag, so masking it off before the shift is what the
 *    original author would write. Same lever as vfree.c's `mask` second use.
 *    Do NOT "simplify" it back to `header->size << 2`: that is the 12-byte
 *    regression.
 *
 * The two rejected alternatives, for the record: a `do{}while(0)` around the
 * cmask definition also flips the pair, but its LOOP_BEG note bars sched2
 * from interleaving the lui/ori up among the prologue saves (the target has
 * it between `sw s5` and `sw ra`), stranding the load-delay slot -> +1 nop,
 * 196 insns. Fencing the inline `sz` use instead gives cmask 5 refs (2702),
 * which overshoots the outer `header`'s 2727 and rotates it plus mask/helper
 * header -> 38 bytes. 4 refs is the only value in the window (1194, 2307).
 *
 * STATUS: MATCH — 780 bytes / 195 instructions, 0 differing bytes.
 */
void *vmemoryGC(void *pt)
{
    PoolBlock *header;
    u32 cmask;
    u32 mask;
    u32 size;
    u32 *vmpt;

    cmask = 0x7fffffff;
    header = (PoolBlock *)pt - 1;
    size = (header->size & cmask) << 2;
    vmpt = valloc(size);
    if (vmpt != 0)
    {
        if ((void *)vmpt < pt)
        {
            memcpy(vmpt, pt, size);
            FreePoolBlockInline(pt, cmask);
            return vmpt;
        }
        else
        {
            FreePoolBlockInline(vmpt, cmask);
        }
    }

    {
        PoolBlock *prev;
        PoolBlock *n2;
        void *newpt;
        PoolBlock vh;
        s32 sz2;

        prev = virtual_memory_pool;
        if (prev != 0)
        {
        search3:
            n2 = prev->next;
            if (n2 == header)
                goto found3;
            prev = n2;
            if (prev != 0)
                goto search3;
        found3:
            if (prev != 0)
            {
                sz2 = prev->size;
                if (sz2 >= 0)
                {
                    do
                    {
                        mask = 0x80000000;
                    } while (0);
                    newpt = (void *)(prev + 1);
                    vh.size = sz2;
                    vh.next = header->next;
                    vmpt = (u32 *)((u8 *)prev + ((header->size << 2) + 8));
                    prev->size = header->size;
                    prev->next = (PoolBlock *)vmpt;
                    memcpy(newpt, pt, size);
                    pt = newpt;
                    prev = vh.next;
                    if (prev != 0 && (~prev->size & mask) != 0)
                    {
                        vh.size = vh.size + (prev->size + 2);
                        vh.next = prev->next;
                    }
                    *(PoolBlock *)vmpt = vh;
                }
            }
        }
    }
    return pt;
}

// triage: MEDIUM — 195 insns, 3 loop, 3 callees, ~0.06 to ReqItemMakibishi
// likely-relevant cookbook sections:
//   - Loops: 3 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// undefined * vmemoryGC(undefined *pt)
//
// {
//   uchar *puVar1;
//   int *piVar2;
//   int *piVar3;
//   int *piVar4;
//   uint *puVar5;
//   uint uVar6;
//   int *piVar7;
//   ulong size;
//   int local_28;
//   uint *local_24;
//
//   piVar7 = (int *)(pt + -8);
//   size = *(int *)(pt + -8) << 2;
//   puVar1 = valloc(size);
//   if (puVar1 != (uchar *)0x0) {
//     if (puVar1 < pt) {
//       memcpy(puVar1,pt,size);
//       if (pt == (undefined *)0x0) {
//         return puVar1;
//       }
//       if (-1 < (int)*(uint *)(pt + -8)) {
//                     /* WARNING: Subroutine does not return */
//         SystemOut((uchar *)"DOUBLE MEMORY RELEASE");
//       }
//       uVar6 = *(uint *)(pt + -8) & 0x7fffffff;
//       *(uint *)(pt + -8) = uVar6;
//       puVar5 = *(uint **)(pt + -4);
//       if (puVar5 != (uint *)0x0) {
//         if ((~*puVar5 & 0x80000000) != 0) {
//           *(uint *)(pt + -8) = uVar6 + 2 + *puVar5;
//           *(uint *)(pt + -4) = puVar5[1];
//         }
//       }
//       piVar4 = virtual_memory_pool;
//       if (virtual_memory_pool == (int *)0x0) {
//         return puVar1;
//       }
//       do {
//         piVar2 = (int *)piVar4[1];
//         if (piVar2 == piVar7) break;
//         piVar4 = piVar2;
//       } while (piVar2 != (int *)0x0);
//       if (piVar4 == (int *)0x0) {
//         return puVar1;
//       }
//       if (*piVar4 < 0) {
//         return puVar1;
//       }
//       *piVar4 = *piVar4 + 2 + *piVar7;
//       piVar4[1] = *(int *)(pt + -4);
//       return puVar1;
//     }
//     if (puVar1 != (uchar *)0x0) {
//       if (-1 < (int)*(uint *)(puVar1 + -8)) {
//                     /* WARNING: Subroutine does not return */
//         SystemOut((uchar *)"DOUBLE MEMORY RELEASE");
//       }
//       uVar6 = *(uint *)(puVar1 + -8) & 0x7fffffff;
//       *(uint *)(puVar1 + -8) = uVar6;
//       puVar5 = *(uint **)(puVar1 + -4);
//       if (puVar5 != (uint *)0x0) {
//         if ((~*puVar5 & 0x80000000) != 0) {
//           *(uint *)(puVar1 + -8) = uVar6 + 2 + *puVar5;
//           *(uint *)(puVar1 + -4) = puVar5[1];
//         }
//       }
//       piVar4 = virtual_memory_pool;
//       if (virtual_memory_pool != (int *)0x0) {
//         do {
//           piVar2 = (int *)piVar4[1];
//           if (piVar2 == (int *)(puVar1 + -8)) break;
//           piVar4 = piVar2;
//         } while (piVar2 != (int *)0x0);
//         if ((piVar4 != (int *)0x0) && (-1 < *piVar4)) {
//           *piVar4 = *piVar4 + 2 + *(int *)(puVar1 + -8);
//           piVar4[1] = *(int *)(puVar1 + -4);
//         }
//       }
//     }
//   }
//   piVar4 = (int *)pt;
//   piVar2 = virtual_memory_pool;
//   if (virtual_memory_pool != (int *)0x0) {
//     do {
//       piVar3 = (int *)piVar2[1];
//       if (piVar3 == piVar7) break;
//       piVar2 = piVar3;
//     } while (piVar3 != (int *)0x0);
//     if ((piVar2 != (int *)0x0) && (local_28 = *piVar2, -1 < local_28)) {
//       piVar4 = piVar2 + 2;
//       local_24 = *(uint **)(pt + -4);
//       piVar3 = piVar2 + *piVar7 + 2;
//       *piVar2 = *piVar7;
//       piVar2[1] = (int)piVar3;
//       memcpy((uchar *)piVar4,pt,size);
//       if ((local_24 != (uint *)0x0) && ((~*local_24 & 0x80000000) != 0)) {
//         local_28 = local_28 + 2 + *local_24;
//         local_24 = (uint *)local_24[1];
//       }
//       *piVar3 = local_28;
//       piVar3[1] = (int)local_24;
//     }
//   }
//   return (undefined *)piVar4;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? SystemOut(? *);                                   /* extern */
// ? memcpy(u32, void *, s32, void *);                 /* extern */
// u32 valloc(s32);                                    /* extern */
// extern ? D_8001104C;
// extern void *virtual_memory_pool;
//
// u32 vmemoryGC(void *arg0) {
//     s32 sp10;
//     void *sp14;
//     s32 temp_a1;
//     s32 temp_a1_2;
//     s32 temp_s6;
//     s32 temp_v0_5;
//     s32 temp_v1;
//     s32 temp_v1_2;
//     s32 temp_v1_3;
//     s32 temp_v1_4;
//     s32 temp_v1_5;
//     s32 temp_v1_6;
//     u32 temp_v0;
//     u32 var_v0;
//     void *temp_a0;
//     void *temp_a0_2;
//     void *temp_s0;
//     void *temp_s0_2;
//     void *temp_s1;
//     void *temp_s3;
//     void *temp_v0_2;
//     void *temp_v0_3;
//     void *temp_v0_4;
//     void *var_a0;
//     void *var_a0_2;
//     void *var_a3;
//     void *var_s2;
//
//     var_s2 = arg0;
//     temp_s3 = var_s2 - 8;
//     temp_s6 = var_s2->unk-8 * 4;
//     temp_v0 = valloc(temp_s6);
//     if (temp_v0 != 0) {
//         if (temp_v0 < (u32) var_s2) {
//             memcpy(temp_v0, var_s2, temp_s6);
//             if (var_s2 != NULL) {
//                 if (var_s2->unk-8 >= 0) {
//                     SystemOut(&D_8001104C);
//                 }
//                 temp_a1 = var_s2->unk-8 & 0x7FFFFFFF;
//                 var_s2->unk-8 = temp_a1;
//                 temp_a0 = temp_s3->unk4;
//                 if (temp_a0 != NULL) {
//                     temp_v1 = temp_a0->unk0;
//                     if (~temp_v1 & 0x80000000) {
//                         var_s2->unk-8 = (s32) (temp_a1 + 2 + temp_v1);
//                         temp_s3->unk4 = (void *) temp_a0->unk4;
//                     }
//                 }
//                 var_a0 = virtual_memory_pool;
//                 var_v0 = temp_v0;
//                 if (var_a0 != NULL) {
// loop_9:
//                     temp_v0_2 = var_a0->unk4;
//                     if (temp_v0_2 != temp_s3) {
//                         var_a0 = temp_v0_2;
//                         if (var_a0 != NULL) {
//                             goto loop_9;
//                         }
//                     }
//                     var_v0 = temp_v0;
//                     if (var_a0 != NULL) {
//                         temp_v1_2 = var_a0->unk0;
//                         if (temp_v1_2 >= 0) {
//                             var_a0->unk0 = (s32) (temp_v1_2 + 2 + var_s2->unk-8);
//                             var_a0->unk4 = (void *) temp_s3->unk4;
//                             goto block_14;
//                         }
//                     }
//                     /* Duplicate return node #37. Try simplifying control flow for better match */
//                     return var_v0;
//                 }
//                 /* Duplicate return node #37. Try simplifying control flow for better match */
//                 return var_v0;
//             }
// block_14:
//             return temp_v0;
//         }
//         temp_s0 = temp_v0 - 8;
//         if (temp_v0 != 0) {
//             if (temp_v0->unk-8 >= 0) {
//                 SystemOut(&D_8001104C);
//             }
//             temp_a1_2 = temp_v0->unk-8 & 0x7FFFFFFF;
//             temp_v0->unk-8 = temp_a1_2;
//             temp_a0_2 = temp_s0->unk4;
//             if (temp_a0_2 != NULL) {
//                 temp_v1_3 = temp_a0_2->unk0;
//                 if (~temp_v1_3 & 0x80000000) {
//                     temp_v0->unk-8 = (s32) (temp_a1_2 + 2 + temp_v1_3);
//                     temp_s0->unk4 = (void *) temp_a0_2->unk4;
//                 }
//             }
//             var_a0_2 = virtual_memory_pool;
//             if (var_a0_2 != NULL) {
// loop_22:
//                 temp_v0_3 = var_a0_2->unk4;
//                 if (temp_v0_3 != temp_s0) {
//                     var_a0_2 = temp_v0_3;
//                     if (var_a0_2 != NULL) {
//                         goto loop_22;
//                     }
//                 }
//                 if (var_a0_2 != NULL) {
//                     temp_v1_4 = var_a0_2->unk0;
//                     if (temp_v1_4 >= 0) {
//                         var_a0_2->unk0 = (s32) (temp_v1_4 + 2 + temp_v0->unk-8);
//                         var_a0_2->unk4 = (void *) temp_s0->unk4;
//                     }
//                 }
//             }
//         }
//         goto block_27;
//     }
// block_27:
//     var_a3 = virtual_memory_pool;
//     var_v0 = (u32) var_s2;
//     if (var_a3 != NULL) {
// loop_28:
//         temp_v0_4 = var_a3->unk4;
//         if (temp_v0_4 != temp_s3) {
//             var_a3 = temp_v0_4;
//             if (var_a3 != NULL) {
//                 goto loop_28;
//             }
//         }
//         var_v0 = (u32) var_s2;
//         if (var_a3 != NULL) {
//             temp_v0_5 = var_a3->unk0;
//             if (temp_v0_5 >= 0) {
//                 temp_s0_2 = var_a3 + 8;
//                 sp10 = temp_v0_5;
//                 sp14 = temp_s3->unk4;
//                 temp_v1_5 = var_s2->unk-8;
//                 temp_s1 = var_a3 + ((temp_v1_5 * 4) + 8);
//                 var_a3->unk0 = temp_v1_5;
//                 var_a3->unk4 = temp_s1;
//                 memcpy((u32) temp_s0_2, var_s2, temp_s6, var_a3);
//                 var_s2 = temp_s0_2;
//                 if (sp14 != NULL) {
//                     temp_v1_6 = sp14->unk0;
//                     if (~temp_v1_6 & 0x80000000) {
//                         sp10 = sp10 + 2 + temp_v1_6;
//                         sp14 = sp14->unk4;
//                     }
//                 }
//                 temp_s1->unk0 = sp10;
//                 temp_s1->unk4 = sp14;
//             }
//             var_v0 = (u32) var_s2;
//         }
//     }
//     return var_v0;
// }
