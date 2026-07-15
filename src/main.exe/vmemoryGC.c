#include "common.h"
#include "main.exe.h"

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

typedef struct PoolBlock
{
    s32 size; /* word count, sign bit reserved as an in-use flag by valloc */
    struct PoolBlock *next;
} PoolBlock;

extern PoolBlock *virtual_memory_pool;

extern char D_8001104C[]; /* "DOUBLE MEMORY RELEASE" */
extern void SystemOut(char *);
extern void *valloc(u32 size);
extern void *memcpy(void *dst, void *src, u32 n);

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
 * Matching notes: no `jal vfree` appears anywhere in the target — the
 * free+coalesce logic (vfree.c's already-matched body) is inlined here
 * TWICE, textually, once operating on `pt` and once on `newp`. Each copy
 * keeps vfree's own leading `if (p == 0) return;` guard even though it is
 * provably dead at both call sites (we're already inside `p != 0`) — that
 * redundant check is real code in the target, the tell that this is a
 * literal source-level copy/paste of vfree's body rather than a call.
 *  - The `pt` copy's guard reads `if (pt == 0) return newp;` (this arm HAS
 *    a value to return, so vfree's bare `return;` becomes `return newp;`).
 *  - The `newp` copy has nothing to return here (it falls through to the
 *    third, prev-search block regardless), so vfree's guard becomes a
 *    wrapping `if (newp != 0) { ...rest of vfree's body... }` instead of an
 *    early return — the natural transform when adapting a void early-return
 *    into a fall-through position.
 *  - The final block's tail materializes with vrealloc.c's exact idiom:
 *    `(PoolBlock *)((u8 *)prev + (hsz << 2) + 8)`.
 *  - Both coalescing sums are vfree.c's proven `A + (B + 2)` spelling.
 *
 * STATUS: NON_MATCHING — 241 of 780 bytes differ, but the draft is the
 * CORRECT LENGTH (195/195 instructions) and every differing line is a pure
 * REGISTER RENUMBERING — same instructions, same order, same branch
 * targets, just a different specific hard-register assigned to each of the
 * 7 callee-saved-worthy values (pt, header, cmask (0x7fffffff), mask
 * (0x80000000, one variable reassigned fresh in each of the 3 inlined
 * blocks — this exact reuse, rather than 3 separately-scoped locals, is
 * what closed the LENGTH gap: see below), size, newp, and the per-block
 * header-copy/tail role that reuses one more slot 3x). Confirmed via
 * `tools/rtldump.py vmemoryGC --draft`'s `.greg` dump: target's allocation
 * order (by hard-reg number, ascending) is copy-var(s0) > newp(s1) >
 * pt(s2) > header(s3) > mask(s4) > cmask(s5) > size(s6, LAST); our cc1
 * run's own global-alloc priority queue ("N regs to allocate: ...") instead
 * orders newp > mask > pt > copy-var > header > size > cmask — the same 7
 * values, same 7 hard regs, different PRIORITY (refs/live-length) ranking.
 * Tried and ruled out: (1) declaring `mask` separately per block (3
 * distinct pseudos) — undershoots by 12-16 bytes, cc1 doesn't reuse one
 * hard reg across the 3 disjoint scopes on its own; (2) hoisting `mask` to
 * one function-scope variable reassigned fresh in each block (kept, closes
 * the LENGTH gap exactly) but over-inflates its priority past pt's; (3)
 * dropping the explicit `vhp = header;` copy in the first block (matches
 * block 2's simpler single-`header2` shape) — same score, doesn't fix the
 * ordering; (4) rewriting the SystemOut guard as `header->size >= 0`
 * instead of `(header->size & mask) == 0` — vfree.c's OWN matched source
 * uses the AND-mask spelling although it folds to an identical `bltz`
 * either way (confirmed against vfree's own target .s), and dropping it
 * measurably hurt mask's register assignment, so the AND-mask spelling is
 * kept. A bounded permuter run (`timeout 300 tools/permute.py vmemoryGC --
 * --stop-on-zero -j4`, ~20000+ iterations) plateaued at score 1550 (from a
 * base of 2200), never reaching 0 — consistent with a genuine
 * priority-formula tie below the C level, not a source-structure bug.
 * `tools/autorules.py` found only one 1-byte win (`s: s32->s16` in the
 * first block's forward-merge temp) and reports no further improving edit.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/vmemoryGC", vmemoryGC);
#else
void *vmemoryGC(void *pt)
{
    PoolBlock *header;
    u32 cmask;
    u32 mask;
    u32 size;
    void *newp;

    cmask = 0x7fffffff;
    header = (PoolBlock *)pt - 1;
    size = header->size << 2;
    newp = valloc(size);
    if (newp != 0)
    {
        if (newp < pt)
        {
            PoolBlock *vhp;
            PoolBlock *next;
            PoolBlock *prev;
            PoolBlock *n2;
            u32 sz;
            s16 s;

            memcpy(newp, pt, size);
            if (pt == 0)
                return newp;

            vhp = header;
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
            search1:
                n2 = prev->next;
                if (n2 == vhp)
                    goto found1;
                prev = n2;
                if (prev != 0)
                    goto search1;
            found1:
                if (prev != 0)
                {
                    s = prev->size;
                    if (s >= 0)
                    {
                        prev->size = s + (vhp->size + 2);
                        prev->next = vhp->next;
                    }
                }
            }
            return newp;
        }
        else
        {
            PoolBlock *header2;
            PoolBlock *next;
            PoolBlock *prev;
            PoolBlock *n2;
            u32 sz;
            s32 s;

            if (newp != 0)
            {
                header2 = (PoolBlock *)newp - 1;
                mask = 0x80000000;
                if ((header2->size & mask) == 0)
                    SystemOut(D_8001104C);

                sz = header2->size & cmask;
                header2->size = sz;

                next = header2->next;
                if (next != 0)
                {
                    s = next->size;
                    if ((~s & mask) != 0)
                    {
                        header2->size = sz + (s + 2);
                        header2->next = next->next;
                    }
                }

                prev = virtual_memory_pool;
                if (prev != 0)
                {
                search2:
                    n2 = prev->next;
                    if (n2 == header2)
                        goto found2;
                    prev = n2;
                    if (prev != 0)
                        goto search2;
                found2:
                    if (prev != 0)
                    {
                        s = prev->size;
                        if (s >= 0)
                        {
                            prev->size = s + (header2->size + 2);
                            prev->next = header2->next;
                        }
                    }
                }
            }
        }
    }

    {
        PoolBlock *prev;
        PoolBlock *n2;
        PoolBlock *tail;
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
                    mask = 0x80000000;
                    newpt = (void *)(prev + 1);
                    vh.size = sz2;
                    vh.next = header->next;
                    tail = (PoolBlock *)((u8 *)prev + (header->size << 2) + 8);
                    prev->size = header->size;
                    prev->next = tail;
                    memcpy(newpt, pt, size);
                    pt = newpt;
                    if (vh.next != 0 && (~vh.next->size & mask) != 0)
                    {
                        vh.size = vh.size + (vh.next->size + 2);
                        vh.next = vh.next->next;
                    }
                    *tail = vh;
                }
            }
        }
    }
    return pt;
}
#endif /* NON_MATCHING */

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
