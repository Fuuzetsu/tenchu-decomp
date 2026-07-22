#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void UpdateItemState(void);
 *     ITEM.C:3176, 25 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
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
 *     reg   $s4       int i
 *     reg   $s1       int mode
 *     reg   $v0       int t
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

/*
 * STATUS: MATCHING (440 bytes).
 *
 * A structured `while` makes loop.c either eliminate `i` in favour of an
 * `items[i]` GIV or bias an explicit cursor to `item + 0x10`.  The literal
 * goto back edge keeps `i` and `item` as independent, unbiased variables.
 * Since that shape also disables loop-invariant hoisting, cache `ViewInfo`
 * and `ConflictObject` explicitly before the label; this reproduces the
 * target's s5/s6 bases without reintroducing a loop GIV.
 *
 * The first camera coordinate deliberately uses `ViewInfo` directly while
 * the remaining two use `view`, retaining the target's branch-local `lui`
 * plus persistent base.  Finally, the conflict address is an integer sum
 * with the scaled index written first: that preserves `addu v1,v1,s6`
 * instead of the commuted `addu v1,s6,v1` emitted by ordinary subscripting.
 */

extern s32 abs(s32 x);

/* Per-axis item activation distance from ITEM.C's anonymous enum. */
enum
{
    LEN = 15000
};

static void UpdateItemState(void)
{
    ConflictObjectType *object;
    ConflictObjectType *conflicts;
    TItem *item;
    GsRVIEW2 *view;
    s32 i;
    s32 hit;
    s32 sz, ofsY;
    s32 mode;
    s16 idx;

    i = 0;
    view = &ViewInfo;
    conflicts = ConflictObject;
    item = items;
loop:
    if (!(i < 0x1e))
        goto done;
    {
        if (item->proc != 0)
        {
            hit = 0;
            if (abs(ViewInfo.vpx - item->locate->locate.coord.t[0]) < LEN &&
                abs(view->vpy - item->locate->locate.coord.t[1]) < LEN)
            {
                hit = abs(view->vpz - item->locate->locate.coord.t[2]) < LEN;
            }
            if (hit)
            {
                if (item->collision.pause != 0)
                {
                    sz = item->collision.size;
                    if (sz != 0)
                    {
                        ofsY = item->collision.ofsY;
                        mode = item->collision.mode;
                        DeleteConflict(item->locate);
                        idx = InsertConflict(item->locate);
                        object = (ConflictObjectType *)
                            ((s32)idx * sizeof(*object) + (u32)conflicts);
                        object->offset.vx = 0;
                        object->offset.vz = 0;
                        object->offset.vy = ofsY;
                        object->size.vz = sz;
                        object->size.vy = sz;
                        object->size.vx = sz;
                        object->common = (void *)1;
                        object->size.pad = mode;
                        item->collision.size = sz;
                        item->collision.ofsY = ofsY;
                        item->collision.mode = mode;
                        item->collision.pause = 0;
                    }
                }
            }
            else if (item->collision.pause == 0 && item->locate->locate.super == 0)
            {
                item->collision.pause = 1;
                DeleteConflict(item->locate);
            }
        }
        item++;
        i++;
        goto loop;
    }
done:
    ;
}

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void UpdateItemState(void)
//
// {
//   short sVar1;
//   short sVar2;
//   bool bVar3;
//   short sVar4;
//   int iVar5;
//   ModelType *model;
//   tag_TItem *ptVar6;
//   int iVar7;
//
//   ptVar6 = items;
//   for (iVar7 = 0; iVar7 < 0x1e; iVar7 = iVar7 + 1) {
//     if (ptVar6->proc != (undefined **)0x0) {
//       bVar3 = false;
//       iVar5 = abs(ViewInfo.vpx - (ptVar6->locate->locate).coord.t[0]);
//       if ((iVar5 < 15000) &&
//          (iVar5 = abs(ViewInfo.vpy - (ptVar6->locate->locate).coord.t[1]), iVar5 < 15000)) {
//         iVar5 = abs(ViewInfo.vpz - (ptVar6->locate->locate).coord.t[2]);
//         bVar3 = iVar5 < 15000;
//       }
//       if (bVar3) {
//         if (((ptVar6->collision).pause != 0) && (sVar1 = (ptVar6->collision).size, sVar1 != 0)) {
//           sVar2 = (ptVar6->collision).ofsY;
//           iVar5 = (ptVar6->collision).mode;
//           DeleteConflict(ptVar6->locate);
//           sVar4 = InsertConflict(ptVar6->locate);
//           ConflictObject[sVar4].offset.vx = 0;
//           ConflictObject[sVar4].offset.vz = 0;
//           ConflictObject[sVar4].offset.vy = sVar2;
//           ConflictObject[sVar4].size.vz = sVar1;
//           ConflictObject[sVar4].size.vy = sVar1;
//           ConflictObject[sVar4].size.vx = sVar1;
//           ConflictObject[sVar4].common = (undefined *)0x1;
//           ConflictObject[sVar4].size.pad = (short)iVar5;
//           (ptVar6->collision).size = sVar1;
//           (ptVar6->collision).ofsY = sVar2;
//           (ptVar6->collision).mode = iVar5;
//           (ptVar6->collision).pause = 0;
//         }
//       }
//       else if (((ptVar6->collision).pause == 0) &&
//               ((ptVar6->locate->locate).super == (_GsCOORDINATE2 *)0x0)) {
//         model = ptVar6->locate;
//         (ptVar6->collision).pause = 1;
//         DeleteConflict(model);
//       }
//     }
//     ptVar6 = ptVar6 + 1;
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? DeleteConflict(void *);                           /* extern */
// s16 InsertConflict(void *);                         /* extern */
// s32 abs(s32);                                       /* extern */
// extern ? ConflictObject;
// extern ? ViewInfo;
// extern ? items;
//
// void UpdateItemState(void) {
//     ? *var_s2;
//     s16 temp_s0;
//     s16 temp_s3;
//     s32 temp_s1;
//     s32 var_s0;
//     s32 var_s4;
//     void *temp_v1;
//
//     var_s4 = 0;
//     var_s2 = &items;
// loop_1:
//     if (var_s4 < 0x1E) {
//         if (var_s2->unkC != 0) {
//             var_s0 = 0;
//             if ((abs(ViewInfo.unk0 - var_s2->unk10->unk18) < 0x3A98) && (abs(ViewInfo.unk4 - var_s2->unk10->unk1C) < 0x3A98)) {
//                 var_s0 = abs(ViewInfo.unk8 - var_s2->unk10->unk20) < 0x3A98;
//             }
//             if (var_s0 != 0) {
//                 if (var_s2->unk18 != 0) {
//                     temp_s3 = var_s2->unk1C;
//                     if (temp_s3 != 0) {
//                         temp_s0 = var_s2->unk1E;
//                         temp_s1 = var_s2->unk14;
//                         DeleteConflict(var_s2->unk10);
//                         temp_v1 = (InsertConflict(var_s2->unk10) * 0x78) + &ConflictObject;
//                         temp_v1->unk14 = 0;
//                         temp_v1->unk18 = 0;
//                         temp_v1->unk16 = temp_s0;
//                         temp_v1->unk20 = temp_s3;
//                         temp_v1->unk1E = temp_s3;
//                         temp_v1->unk1C = temp_s3;
//                         temp_v1->unk24 = 1;
//                         temp_v1->unk22 = (s16) temp_s1;
//                         var_s2->unk1C = temp_s3;
//                         var_s2->unk1E = temp_s0;
//                         var_s2->unk14 = temp_s1;
//                         var_s2->unk18 = 0;
//                     }
//                 }
//             } else if ((var_s2->unk18 == 0) && (var_s2->unk10->unk48 == 0)) {
//                 var_s2->unk18 = 1;
//                 DeleteConflict(var_s2->unk10);
//             }
//         }
//         var_s2 += 0x58;
//         var_s4 += 1;
//         goto loop_1;
//     }
// }
