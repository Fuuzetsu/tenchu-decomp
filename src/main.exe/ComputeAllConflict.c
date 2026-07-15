#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ComputeAllConflict(void);
 *     CONFLICT.C:329, 50 src lines, frame 72 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     reg   $s1       struct ConflictObjectType * confop
 *     reg   $s0       struct ModelType * model
 *     stack sp+16     struct MATRIX mat
 *     reg   $s2       short i
 *     reg   $t0       short j
 *
 * Globals it touches, as the original declared them:
 *     extern short ConflictObjects;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct ModelType World;
 * END PSX.SYM */

/* Conflict slot (Ghidra: ConflictObjectType, 0x78 bytes; see DeleteConflict.c /
 * GetConflictResult.c / InsertConflict.c — same TU). */
typedef struct
{
    ModelType *model;            /* 0x00 */
    VECTOR position;             /* 0x04 */
    SVECTOR offset;              /* 0x14 */
    SVECTOR size;                /* 0x1C */
    void *common;                /* 0x24 */
    u8 result[64];               /* 0x28 */
    u8 pad[0x10];                /* 0x68 */
} ConflictObjectType;            /* 0x78 */

/* The conflict pool + live count (Ghidra: ConflictObject / ConflictObjects). */
extern ConflictObjectType ConflictObject[];
extern s16 ConflictObjects;

extern ModelType World;

extern void GsGetLw(GsCOORDINATE2 *coord, MATRIX *m);
extern void GsSetLsMatrix(MATRIX *m);
extern void RotTrans(SVECTOR *v0, VECTOR *sxy, long *flag);
extern void *memset(void *s, int c, u32 n);

/*
 * ComputeAllConflict (0x8001a688) — same TU as InsertConflict.c/DeleteConflict.c/
 * GetConflictResult.c: runs the conflict pool's per-frame update, once per model
 * flagged "active" (attribute bit 0x4000). Pass 1 refreshes each active slot's
 * world-space `position` from its model (either directly, when the model's
 * coordinate hierarchy root IS World, or via GsGetLw/GsSetLsMatrix/RotTrans
 * otherwise) and clears its `result[]` row and `offset.pad` hit counter. Pass 2
 * is the O(n^2) AABB overlap test (y, then z, then x — that axis order matches
 * the target) between every distinct pair of active slots; a hit stamps both
 * slots' `result[]` (the OTHER slot's `size.pad` byte, tagged with 0x80),
 * flags both models' attribute bit 0x8000, and bumps both `offset.pad` counters.
 *
 * Matching notes:
 *  - `confop` (a real PSX.SYM local, ConflictObjectType*) caches `&ConflictObject[i]`
 *    for the WHOLE outer-loop body in both passes. `other` (`&ConflictObject[j]`,
 *    not in PSX.SYM's recorded set — the demo build's local list under-records,
 *    per the standard caveat) is needed to get the target's `a1`-cached-pointer
 *    reuse across all of the inner loop's position/size/model reads.
 *  - `model->locate.super == &World.locate`: locate (GsCOORDINATE2) is
 *    ModelType's own first field, so `&model->locate` is model itself (see
 *    GetAbsolutePosition.c's identical `GsGetLw(&model->locate, &m)` idiom).
 *  - Both loops are natural `for (i = 0; i < ConflictObjects; i++)` — cc1's
 *    duplicate_loop_exit_test alone produces the guarded do-while target shape
 *    (DeleteConflict.c/GetConflictResult.c precedent), no explicit redundant
 *    guard needed here.
 *
 * STATUS: NON_MATCHING — 8 of 816 bytes over length (206 vs target 204
 * instructions), confirmed to be a pure INSTRUCTION-SCHEDULING residual, not
 * a structural one: every field access, struct layout, loop shape, and
 * operand is proven correct (traced instruction-by-instruction against the
 * raw target .s in .shake/gen/main.exe/asm/nonmatchings/ComputeAllConflict/).
 * In each of the 3 AABB axis checks (`d = other->position.vN -
 * confop->position.vN; if (d<0) d=-d; if (d <= other->size.vN +
 * confop->size.vN)`), the target interleaves BOTH size loads with the
 * delta/abs computation (loads size.vN for `other` right after the
 * subtraction, size.vN for `confop` right after that — both BEFORE the
 * `bgez`/`negu` abs sequence), so by the time the loaded values are added
 * together enough independent instructions have executed to cover the
 * MIPS I load-delay slot with no stall. Our compile evaluates the 3
 * statements in strict sequence (matching source order) — both size loads
 * happen only when reaching the third statement, back-to-back, so the
 * second load's result is used one instruction after the load, forcing an
 * explicit `nop` load-delay filler each time. Sub-C-level: identical
 * source, target's scheduler (sched1, pre-allocation) chose to hoist two
 * independent loads earlier than a strictly-sequential reading of the same
 * 3 statements would place them. Ruled out: reordering the `size.vN +`
 * operands (no effect, addition is commutative and cc1 folds either
 * spelling to the same RTL); using `ConflictObject[i].result[j]` /
 * `ConflictObject[j].result[i]` direct-index spellings for the two
 * `result[]` stores instead of `confop`/`other` pointers (matches the
 * target's exact 3-term address recompute there, `&ConflictObject +
 * i*0x78 + j` instead of reusing the cached `confop` pointer — CONFIRMED
 * correct against the raw target asm) but this alone made the whole-image
 * residual WORSE (836 vs 824 bytes), so it was not adopted pending a fuller
 * resolution of the scheduling piece. A bounded permuter run (`timeout 300
 * tools/permute.py ComputeAllConflict -- --stop-on-zero -j4`, ~25000+
 * iterations) plateaued at best score 1905 (from a base of 2515), never
 * reaching 0.
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/ComputeAllConflict", ComputeAllConflict);
#else
void ComputeAllConflict(void)
{
    short i;
    short j;
    ModelType *model;
    ConflictObjectType *confop;
    MATRIX mat;
    int d;

    for (i = 0; i < ConflictObjects; i++)
    {
        confop = &ConflictObject[i];
        model = confop->model;
        if (model->attribute & 0x4000)
        {
            memset(confop->result, 0, 0x50);
            confop->offset.pad = 0;
            model->attribute = model->attribute & 0x7fff;
            if (model->locate.super == &World.locate)
            {
                confop->position.vx = model->locate.coord.t[0] + confop->offset.vx;
                confop->position.vy = model->locate.coord.t[1] + confop->offset.vy;
                confop->position.vz = model->locate.coord.t[2] + confop->offset.vz;
            }
            else
            {
                GsGetLw(&model->locate, &mat);
                GsSetLsMatrix(&mat);
                RotTrans(&confop->offset, &confop->position, (long *)0);
            }
        }
    }

    for (i = 0; i < ConflictObjects; i++)
    {
        confop = &ConflictObject[i];
        if (confop->model->attribute & 0x4000)
        {
            for (j = i + 1; j < ConflictObjects; j++)
            {
                ConflictObjectType *other = &ConflictObject[j];

                if (other->model->attribute & 0x4000)
                {
                    d = other->position.vy - confop->position.vy;
                    if (d < 0)
                        d = -d;
                    if (d <= other->size.vy + confop->size.vy)
                    {
                        d = other->position.vz - confop->position.vz;
                        if (d < 0)
                            d = -d;
                        if (d <= other->size.vz + confop->size.vz)
                        {
                            d = other->position.vx - confop->position.vx;
                            if (d < 0)
                                d = -d;
                            if (d <= other->size.vx + confop->size.vx)
                            {
                                confop->result[j] = other->size.pad | 0x80;
                                other->result[i] = confop->size.pad | 0x80;
                                model = confop->model;
                                model->attribute = model->attribute | 0x8000;
                                model = other->model;
                                model->attribute = model->attribute | 0x8000;
                                confop->offset.pad = confop->offset.pad + 1;
                                other->offset.pad = other->offset.pad + 1;
                            }
                        }
                    }
                }
            }
        }
    }
}
#endif /* NON_MATCHING */

// triage: MEDIUM — 204 insns, 3 loop, 4 callees, ~0.12 to update_something_for_each_visible_enemy_
// likely-relevant cookbook sections:
//   - Loops: 3 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Type propagation algorithm not settling */
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void ComputeAllConflict(void)
//
// {
//   short sVar1;
//   short sVar2;
//   int iVar3;
//   int iVar4;
//   ModelType *pMVar5;
//   int iVar6;
//   ModelType *pMVar7;
//   int iVar8;
//   MATRIX MStack_38;
//
//   iVar8 = 0;
//   if (0 < (short)ConflictObjects) {
//     iVar3 = 0;
//     do {
//       iVar3 = iVar3 >> 0x10;
//       pMVar7 = ConflictObject[iVar3].model;
//       if ((pMVar7->attribute & 0x4000U) != 0) {
//         memset(ConflictObject[iVar3].result,'\0',0x50);
//         ConflictObject[iVar3].offset.pad = 0;
//         pMVar5 = (ModelType *)(pMVar7->locate).super;
//         pMVar7->attribute = pMVar7->attribute & 0x7fff;
//         if (pMVar5 == &World) {
//           sVar2 = ConflictObject[iVar3].offset.vy;
//           ConflictObject[iVar3].position.vx =
//                (pMVar7->locate).coord.t[0] + (int)ConflictObject[iVar3].offset.vx;
//           sVar1 = ConflictObject[iVar3].offset.vz;
//           ConflictObject[iVar3].position.vy = (pMVar7->locate).coord.t[1] + (int)sVar2;
//           ConflictObject[iVar3].position.vz = (pMVar7->locate).coord.t[2] + (int)sVar1;
//         }
//         else {
//           GsGetLw(&pMVar7->locate,&MStack_38);
//           GsSetLsMatrix(&MStack_38);
//           RotTrans(&ConflictObject[iVar3].offset,&ConflictObject[iVar3].position,(long *)0x0);
//         }
//       }
//       iVar8 = iVar8 + 1;
//       iVar3 = iVar8 * 0x10000;
//     } while (iVar8 * 0x10000 >> 0x10 < (int)(short)ConflictObjects);
//   }
//   iVar8 = 0;
//   if (0 < (short)ConflictObjects) {
//     iVar3 = 0;
//     do {
//       iVar3 = iVar3 >> 0x10;
//       iVar4 = iVar8 + 1;
//       if ((((ConflictObject[iVar3].model)->attribute & 0x4000U) != 0) &&
//          (iVar4 * 0x10000 < (int)((uint)ConflictObjects << 0x10))) {
//         do {
//           sVar2 = (short)iVar4;
//           if (((ConflictObject[sVar2].model)->attribute & 0x4000U) != 0) {
//             iVar6 = ConflictObject[sVar2].position.vy - ConflictObject[iVar3].position.vy;
//             if (iVar6 < 0) {
//               iVar6 = -iVar6;
//             }
//             if (iVar6 <= (int)ConflictObject[iVar3].size.vy + (int)ConflictObject[sVar2].size.vy) {
//               iVar6 = ConflictObject[sVar2].position.vz - ConflictObject[iVar3].position.vz;
//               if (iVar6 < 0) {
//                 iVar6 = -iVar6;
//               }
//               if (iVar6 <= (int)ConflictObject[iVar3].size.vz + (int)ConflictObject[sVar2].size.vz)
//               {
//                 iVar6 = ConflictObject[sVar2].position.vx - ConflictObject[iVar3].position.vx;
//                 if (iVar6 < 0) {
//                   iVar6 = -iVar6;
//                 }
//                 if (iVar6 <= (int)ConflictObject[iVar3].size.vx + (int)ConflictObject[sVar2].size.vx
//                    ) {
//                   ConflictObject[iVar3].result[sVar2] = (byte)ConflictObject[sVar2].size.pad | 0x80;
//                   ConflictObject[sVar2].result[iVar3] = (byte)ConflictObject[iVar3].size.pad | 0x80;
//                   pMVar7 = ConflictObject[iVar3].model;
//                   pMVar7->attribute = pMVar7->attribute | 0x8000;
//                   pMVar7 = ConflictObject[sVar2].model;
//                   pMVar7->attribute = pMVar7->attribute | 0x8000;
//                   ConflictObject[iVar3].offset.pad = ConflictObject[iVar3].offset.pad + 1;
//                   ConflictObject[sVar2].offset.pad = ConflictObject[sVar2].offset.pad + 1;
//                 }
//               }
//             }
//           }
//           iVar4 = iVar4 + 1;
//         } while (iVar4 * 0x10000 >> 0x10 < (int)(short)ConflictObjects);
//       }
//       iVar8 = iVar8 + 1;
//       iVar3 = iVar8 * 0x10000;
//     } while (iVar8 * 0x10000 >> 0x10 < (int)(short)ConflictObjects);
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? GsGetLw(void *, ? *);                             /* extern */
// ? GsSetLsMatrix(? *);                               /* extern */
// ? RotTrans(void *, void *, ?);                      /* extern */
// ? memset(void *, ?, ?);                             /* extern */
// extern ? ConflictObject;
// extern s16 ConflictObjects;
// extern ? World;
//
// void ComputeAllConflict(void) {
//     ? sp10;
//     s16 var_s2;
//     s16 var_s2_2;
//     s16 var_t1;
//     s16 var_v0_2;
//     s16 var_v0_4;
//     s16 var_v0_6;
//     s32 temp_a0;
//     s32 temp_a2;
//     s32 temp_a3_2;
//     s32 temp_t0;
//     s32 var_v0;
//     s32 var_v0_3;
//     s32 var_v0_5;
//     s32 var_v1;
//     s32 var_v1_2;
//     s32 var_v1_3;
//     void *temp_a1;
//     void *temp_a3;
//     void *temp_s0;
//     void *temp_s1;
//     void *temp_v1;
//     void *temp_v1_2;
//
//     var_s2 = 0;
//     if (ConflictObjects > 0) {
//         var_v0 = 0 << 0x10;
//         do {
//             temp_s1 = ((var_v0 >> 0x10) * 0x78) + &ConflictObject;
//             temp_s0 = temp_s1->unk0;
//             var_v0_2 = var_s2 + 1;
//             if (temp_s0->unk5A & 0x4000) {
//                 memset(temp_s1 + 0x28, 0, 0x50);
//                 temp_s1->unk1A = 0;
//                 temp_s0->unk5A = (u16) (temp_s0->unk5A & 0x7FFF);
//                 if (temp_s0->unk48 == &World) {
//                     temp_s1->unk4 = (s32) (temp_s0->unk18 + temp_s1->unk14);
//                     temp_s1->unk8 = (s32) (temp_s0->unk1C + temp_s1->unk16);
//                     temp_s1->unkC = (s32) (temp_s0->unk20 + temp_s1->unk18);
//                 } else {
//                     GsGetLw(temp_s0, &sp10);
//                     GsSetLsMatrix(&sp10);
//                     RotTrans(temp_s1 + 0x14, temp_s1 + 4, 0);
//                 }
//                 var_v0_2 = var_s2 + 1;
//             }
//             var_s2 = var_v0_2;
//             var_v0 = var_s2 << 0x10;
//         } while (var_v0_2 < ConflictObjects);
//     }
//     var_s2_2 = 0;
//     if (ConflictObjects > 0) {
//         var_v0_3 = 0 << 0x10;
//         do {
//             temp_a0 = var_v0_3 >> 0x10;
//             temp_a2 = temp_a0 * 0x78;
//             temp_a3 = temp_a2 + &ConflictObject;
//             var_v0_4 = var_s2_2 + 1;
//             if (temp_a3->unk0->unk5A & 0x4000) {
//                 var_t1 = var_v0_4;
//                 var_v0_4 = var_s2_2 + 1;
//                 if ((var_v0_4 << 0x10) < ((u16) ConflictObjects << 0x10)) {
//                     var_v0_5 = var_t1 << 0x10;
//                     do {
//                         temp_a3_2 = var_v0_5 >> 0x10;
//                         temp_t0 = temp_a3_2 * 0x78;
//                         temp_a1 = temp_t0 + &ConflictObject;
//                         var_v0_6 = var_t1 + 1;
//                         if (temp_a1->unk0->unk5A & 0x4000) {
//                             var_v1 = temp_a1->unk8 - temp_a3->unk8;
//                             if (var_v1 < 0) {
//                                 var_v1 = -var_v1;
//                             }
//                             var_v0_6 = var_t1 + 1;
//                             if ((temp_a3->unk1E + temp_a1->unk1E) >= var_v1) {
//                                 var_v1_2 = temp_a1->unkC - temp_a3->unkC;
//                                 if (var_v1_2 < 0) {
//                                     var_v1_2 = -var_v1_2;
//                                 }
//                                 var_v0_6 = var_t1 + 1;
//                                 if ((temp_a3->unk20 + temp_a1->unk20) >= var_v1_2) {
//                                     var_v1_3 = temp_a1->unk4 - temp_a3->unk4;
//                                     if (var_v1_3 < 0) {
//                                         var_v1_3 = -var_v1_3;
//                                     }
//                                     var_v0_6 = var_t1 + 1;
//                                     if ((temp_a3->unk1C + temp_a1->unk1C) >= var_v1_3) {
//                                         (temp_a3_2 + temp_a2 + &ConflictObject)->unk28 = (s8) (temp_a1->unk22 | ~0x7F);
//                                         (temp_a0 + temp_t0 + &ConflictObject)->unk28 = (s8) (temp_a3->unk22 | ~0x7F);
//                                         temp_v1 = temp_a3->unk0;
//                                         temp_v1->unk5A = (u16) (temp_v1->unk5A | ~0x7FFF);
//                                         temp_v1_2 = temp_a1->unk0;
//                                         temp_v1_2->unk5A = (u16) (temp_v1_2->unk5A | ~0x7FFF);
//                                         temp_a3->unk1A = (u16) (temp_a3->unk1A + 1);
//                                         temp_a1->unk1A = (u16) (temp_a1->unk1A + 1);
//                                         var_v0_6 = var_t1 + 1;
//                                     }
//                                 }
//                             }
//                         }
//                         var_t1 = var_v0_6;
//                         var_v0_5 = var_t1 << 0x10;
//                     } while (var_v0_6 < ConflictObjects);
//                     var_v0_4 = var_s2_2 + 1;
//                 }
//             }
//             var_s2_2 = var_v0_4;
//             var_v0_3 = var_s2_2 << 0x10;
//         } while (var_v0_4 < ConflictObjects);
//     }
// }
