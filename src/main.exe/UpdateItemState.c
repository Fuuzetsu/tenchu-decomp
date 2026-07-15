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
 * STATUS: NON_MATCHING — links the CORRECT LENGTH (110 instructions, 440
 * bytes) but 293 bytes differ. Default build keeps the byte-identical
 * INCLUDE_ASM stub. The C is believed semantically correct and structurally
 * complete (every call, store, field, if/else polarity, the abs distance
 * &&-chain, the fly/reset hit path).
 *
 * The sole residual is a loop.c biv-elimination tie below the C level:
 *  - The target keeps TWO induction variables — a counter `s4` (`slti s4,30`
 *    top test + `addiu s4,1`) AND a walking pointer `s2` (`addiu s2,0x58`) —
 *    with the invariant bases ViewInfo(s5)/ConflictObject(s6) hoisted.
 *  - Reproducing "unbiased walking giv + hoisted bases" REQUIRES `items[i]`
 *    array-indexing under a real loop (`while(1){ if(!(i<0x1e)) break; …;
 *    i++; }`): a hand-rolled goto loop gives the unbiased cursor but no
 *    hoisting (105, 5 short); an explicit walking pointer `it` under a real
 *    loop biases the cursor +0x10 and adds a 7th callee-saved register (112,
 *    2 long).
 *  - But with `items[i]`, loop.c's strength reduction derives the walking
 *    pointer as a GIV of the counter (`.loop` dump: "Insn 40: giv reg 95 …
 *    mult 88 add (reg:SI 87)", then "biv 80 can be eliminated"), so
 *    `maybe_eliminate_biv` replaces `i < 0x1e` with the giv bound
 *    (`addiu v0,s5,2640; slt v0,s2,v0`) and DROPS the counter. The target's
 *    counter survives only because its walking pointer is an independent BIV
 *    (loop.c cannot replace a biv's compare with another biv), which is the
 *    `it++`/explicit-pointer form — and that form biases the field offsets.
 *    No C spelling yields the target's "unbiased giv AND surviving counter"
 *    simultaneously; the counter loss renumbers the whole register file
 *    (ViewInfo→s4 vs s5, items base→s5, etc.), which is the bulk of the 293
 *    differing bytes despite the identical 110-instruction length.
 *  - A 24757-iteration bounded decomp-permuter run never approached zero
 *    (it transforms the C AST; the divergence is chosen in loop.c, an RTL
 *    pass below the AST). This is a genuine sub-C-level tie
 *    (docs/matching-cookbook.md, RTL escalation §).
 */
#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/UpdateItemState", UpdateItemState);
#else

typedef struct
{
    s32 vpx, vpy, vpz;           /* 0x00 */
    s32 vrx, vry, vrz;           /* 0x0C */
} TViewInfo;

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

extern TViewInfo ViewInfo;
extern ConflictObjectType ConflictObject[];
extern s16 InsertConflict(ModelType *m);
extern s32 abs(s32 x);

static void UpdateItemState(void)
{
    s32 i;
    s32 hit;
    s32 sz, ofsY;
    s32 md;
    s16 idx;

    i = 0;
    while (1)
    {
        if (!(i < 0x1e))
            break;
        if (items[i].proc != 0)
        {
            hit = 0;
            if (abs(ViewInfo.vpx - items[i].locate->locate.coord.t[0]) < 15000 &&
                abs(ViewInfo.vpy - items[i].locate->locate.coord.t[1]) < 15000)
            {
                hit = abs(ViewInfo.vpz - items[i].locate->locate.coord.t[2]) < 15000;
            }
            if (hit)
            {
                if (items[i].coll_pause != 0)
                {
                    sz = items[i].coll_size;
                    if (sz != 0)
                    {
                        ofsY = items[i].coll_ofsY;
                        md = items[i].coll_mode;
                        DeleteConflict(items[i].locate);
                        idx = InsertConflict(items[i].locate);
                        ConflictObject[idx].offset.vx = 0;
                        ConflictObject[idx].offset.vz = 0;
                        ConflictObject[idx].offset.vy = ofsY;
                        ConflictObject[idx].size.vz = sz;
                        ConflictObject[idx].size.vy = sz;
                        ConflictObject[idx].size.vx = sz;
                        ConflictObject[idx].common = (void *)1;
                        ConflictObject[idx].size.pad = md;
                        items[i].coll_size = sz;
                        items[i].coll_ofsY = ofsY;
                        items[i].coll_mode = md;
                        items[i].coll_pause = 0;
                    }
                }
            }
            else if (items[i].coll_pause == 0 && items[i].locate->locate.super == 0)
            {
                items[i].coll_pause = 1;
                DeleteConflict(items[i].locate);
            }
        }
        i++;
    }
}

#endif

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
