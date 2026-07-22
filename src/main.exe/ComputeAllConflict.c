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

/* The conflict pool + live count (Ghidra: ConflictObject / ConflictObjects). */

extern ModelType World;

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
 *  - `confop` and `model` (real PSX.SYM locals) belong to pass 1. Pass 2
 *    deliberately keeps `ConflictObject[i]` as direct indexed expressions;
 *    cc1 then builds the outer pointer in `$a3`, preserves both its byte
 *    offset and `i` for the result writes, and copies the pointer to `$a2`
 *    before the inner loop. Reusing the pass-1 locals here incorrectly kept
 *    them in `$s1`/`$s0` and removed that target pointer copy.
 *  - `other` (`&ConflictObject[j]`) is a block-local pointer not recorded in
 *    the demo local list. It gives the target's `$a1` pointer reuse across
 *    the inner loop's position, size, model, and counter accesses.
 *  - Each axis is one direct `__builtin_abs(other position - outer position)`
 *    expression. The opaque `abssi2` RTL lets cc1 schedule both independent
 *    size loads around the subtraction before expanding the target's
 *    `bgez; nop; negu`, eliminating the two load-delay stalls produced by a
 *    separate source-level sign fix. The commutative sum is written outer
 *    size first so the two `lh` destinations also match.
 *  - The two `result[]` writes retain their direct `ConflictObject[i/j]`
 *    indexing. The target recomputes `base + i*0x78 + j` and the symmetric
 *    address instead of shortening either store through the cached pointers.
 *  - `model->locate.super == &World.locate`: locate (GsCOORDINATE2) is
 *    ModelType's own first field, so `&model->locate` is model itself (see
 *    GetAbsolutePosition.c's identical `GsGetLw(&model->locate, &m)` idiom).
 *  - Both loops are natural `for (i = 0; i < ConflictObjects; i++)` — cc1's
 *    duplicate_loop_exit_test alone produces the guarded do-while target shape
 *    (DeleteConflict.c/GetConflictResult.c precedent), no explicit redundant
 *    guard needed here.
 *
 * STATUS: MATCHED — exact 816 bytes / 204 instructions.
 */
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
        if (ConflictObject[i].model->attribute & 0x4000)
        {
            for (j = i + 1; j < ConflictObjects; j++)
            {
                ConflictObjectType *other = &ConflictObject[j];

                if (other->model->attribute & 0x4000)
                {
                    d = __builtin_abs(other->position.vy - ConflictObject[i].position.vy);
                    if (d <= ConflictObject[i].size.vy + other->size.vy)
                    {
                        d = __builtin_abs(other->position.vz - ConflictObject[i].position.vz);
                        if (d <= ConflictObject[i].size.vz + other->size.vz)
                        {
                            d = __builtin_abs(other->position.vx - ConflictObject[i].position.vx);
                            if (d <= ConflictObject[i].size.vx + other->size.vx)
                            {
                                ConflictObject[i].result[j] = other->size.pad | 0x80;
                                ConflictObject[j].result[i] = ConflictObject[i].size.pad | 0x80;
                                ConflictObject[i].model->attribute =
                                    ConflictObject[i].model->attribute | 0x8000;
                                other->model->attribute = other->model->attribute | 0x8000;
                                ConflictObject[i].offset.pad = ConflictObject[i].offset.pad + 1;
                                other->offset.pad = other->offset.pad + 1;
                            }
                        }
                    }
                }
            }
        }
    }
}

// triage: MEDIUM — 204 insns, 3 loop, 4 callees, ~0.12 to update_something_for_each_visible_enemy_
// likely-relevant cookbook sections:
//   - Loops: 3 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference retained for provenance):
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
