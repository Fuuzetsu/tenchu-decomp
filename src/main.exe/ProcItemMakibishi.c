#include "common.h"
#include "main.exe.h"

/*
 * ProcItemMakibishi (0x8003f304) — the makibishi (caltrop) item processor.
 * mode 0: roll it (MoveKorogari) — status 1 disposes (fell out of the
 * world), status 4 registers a 100-unit conflict box and advances to mode 1;
 * mode 1: once a live character is found in the conflict box, spray a
 * SetBleeds burst + step-on sound and dispose. Every mode (and the
 * mode==neither-0-nor-1 default) falls through to a shared tail that redraws
 * the sprite from the current locate every frame.
 *
 * Matching notes (see also ProcItemDrop.c for the item-TU/ConflictObject
 * conventions this shares):
 *  - `sprt = item->model; pp = (param_korogari *)item->param;` both sit
 *    before the entry mode==0xff test (ProcItemDrop's double lever): sprt's
 *    load is sequential, pp's addiu fills the entry branch's delay slot.
 *  - The mode dispatch is a real switch (fresh reload distinct from the
 *    entry ff-check's load, matching the switch rule); with no case for
 *    "neither 0 nor 1", falling out of the switch reaches the shared draw
 *    tail directly — the SAME tail case 0/case 1 reach via `break`.
 *  - `one = 1;` is a real shared variable, not a literal: the status==1
 *    compare needs `1` in a register anyway (MIPS beq has no immediate
 *    form), and the SAME register then feeds `item->mode + one`, `.common =
 *    (void *)one`, `.size.pad = one`, and `item->coll_mode = one` — five
 *    uses, unmistakably `addu` (register) not `addiu` (immediate) at the
 *    mode-increment, so it must survive as a live variable across the
 *    DeleteConflict/InsertConflict calls (callee-saved, like `ff`/`m`).
 *  - The dispose after status==1 and the dispose after mode 1's conflict-hit
 *    are the SAME code written out TWICE (cross-jump merges from the jalr
 *    on): status==1's reuses `ff` (still live, untouched since entry); mode
 *    1's materializes a fresh literal 0xff (a new register) since nothing
 *    carries `ff` that far — same asymmetry as ProcItemKusuri's mode-2 vs
 *    mode-1 dispose.
 *  - Collision box field-store order (offset.vx/vz/vy, then size.vz/vy/vx,
 *    then common, then size.pad) exactly mirrors ProcItemDrop's status
 *    2/4 case, just different numbers (100 not 0xb4, one(1) not m(8)).
 */
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemMakibishi(struct tag_TItem *item);
 *     ITEM.C:1171, 60 src lines, frame 48 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s0       struct tag_TItem * item
 *     reg   $s4       struct Sprite3D * model
 *     reg   $s2       struct param_drop * param
 *     reg   $a0       int cid
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s0       struct tag_TItem * item
 *     reg   $a0       struct ModelType * model
 *     reg   $s1       struct Humanoid * human
 *     reg   $s1       struct Humanoid * human
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 * END PSX.SYM */

extern void MoveKorogari(tag_TItem *item, param_korogari *pp);
extern s16 GetConflictResult(ModelType *m, s32 n);
extern s16 InsertConflict(ModelType *m);
extern s32 is_character_state_present_on_stage_(Humanoid *h);
extern ConflictObjectType ConflictObject[];

void ProcItemMakibishi(tag_TItem *item)
{
    Sprite3D *sprt;
    param_korogari *pp;
    void (*ppu)(tag_TItem *);
    u8 ff;
    u8 st;
    s32 one;
    s32 i;
    s32 n;

    sprt = item->model;
    pp = (param_korogari *)item->param;
    ff = 0xff;
    if (item->mode == ff)
    {
        item->mode = 0;
        return;
    }
    switch (item->mode)
    {
    case 0:
        MoveKorogari(item, pp);
        one = 1;
        st = pp->status;
        switch (st)
        {
        case 4:
            item->mode = item->mode + one;
            DeleteConflict(item->locate);
            n = InsertConflict(item->locate);
            ConflictObject[n].offset.vx = 0;
            ConflictObject[n].offset.vz = 0;
            ConflictObject[n].offset.vy = 0;
            ConflictObject[n].size.vz = 100;
            ConflictObject[n].size.vy = 100;
            ConflictObject[n].size.vx = 100;
            ConflictObject[n].common = (void *)one;
            ConflictObject[n].size.pad = one;
            item->coll_size = 100;
            item->coll_ofsY = 0;
            item->coll_mode = one;
            item->coll_pause = 0;
            break;

        case 1:
            ppu = item->proc;
            if (ppu == 0)
                return;
            item->mode = ff;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
            return;
        }
        break;

    case 1:
        if ((item->locate->attribute & 0x8000) == 0)
            i = -1;
        else
            i = GetConflictResult(item->locate, -1);
        if (i != -1 && is_character_state_present_on_stage_(ConflictObject[i].common) != 0)
        {
            SetBleeds((VECTOR *)item->locate->locate.coord.t, 0, 0x14, 0xa, 0xf, 0x7f0000);
            SoundEx((VECTOR *)item->locate->locate.coord.t, 0x30);
            ppu = item->proc;
            if (ppu == 0)
                return;
            item->mode = 0xff;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
            return;
        }
        break;
    }
    UpdateCoordinate(item->locate);
    sprt->locate = item->locate->locate;
    DrawSprite(sprt);
}

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcItemMakibishi(tag_TItem *item)
//
// {
//   char cVar1;
//   short sVar2;
//   undefined **ppuVar3;
//   Sprite3D *pSVar4;
//   SVECTOR *pSVar5;
//   int iVar6;
//   ModelType *pMVar7;
//   undefined4 uVar8;
//   undefined4 uVar9;
//   undefined4 uVar10;
//   Sprite3D *sprt;
//
//   sprt = (Sprite3D *)item->model;
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//     return;
//   }
//   if (item->mode == '\0') {
//     MoveKorogari(item,(param_korogari *)&(item->param).launch);
//     cVar1 = *(char *)((int)&item->param + 10);
//     if (cVar1 == '\x01') {
//       ppuVar3 = item->proc;
//       if (ppuVar3 == (undefined **)0x0) {
//         return;
//       }
//       item->mode = 0xff;
// LAB_8003f4c8:
//       (*(code *)ppuVar3)(item);
//       DeleteConflict(item->locate);
//       if (item->mode != 0) {
//         AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//       }
//       item->owner = (Humanoid *)0x0;
//       item->proc = (undefined **)0x0;
//       return;
//     }
//     if (cVar1 == '\x04') {
//       item->mode = item->mode + '\x01';
//       DeleteConflict(item->locate);
//       sVar2 = InsertConflict(item->locate);
//       ConflictObject[sVar2].offset.vx = 0;
//       ConflictObject[sVar2].offset.vz = 0;
//       ConflictObject[sVar2].offset.vy = 0;
//       ConflictObject[sVar2].size.vz = 100;
//       ConflictObject[sVar2].size.vy = 100;
//       ConflictObject[sVar2].size.vx = 100;
//       ConflictObject[sVar2].common = (undefined *)0x1;
//       ConflictObject[sVar2].size.pad = 1;
//       (item->collision).size = 100;
//       (item->collision).ofsY = 0;
//       (item->collision).mode = 1;
//       (item->collision).pause = 0;
//     }
//   }
//   else if (item->mode == '\x01') {
//     if (((int)item->locate->attribute & 0x8000U) == 0) {
//       iVar6 = -1;
//     }
//     else {
//       sVar2 = GetConflictResult(item->locate,-1);
//       iVar6 = (int)sVar2;
//     }
//     if ((iVar6 != -1) && (iVar6 = FUN_800294dc(ConflictObject[iVar6].common), iVar6 != 0)) {
//       SetBleeds((short)item->locate + 0x18,0,0x14);
//       SoundEx((VECTOR *)(item->locate->locate).coord.t,0x30);
//       ppuVar3 = item->proc;
//       if (ppuVar3 == (undefined **)0x0) {
//         return;
//       }
//       item->mode = 0xff;
//       goto LAB_8003f4c8;
//     }
//   }
//   UpdateCoordinate(item->locate);
//   pMVar7 = item->locate;
//   pSVar5 = &pMVar7->rotate;
//   pSVar4 = sprt;
//   do {
//     uVar8 = *(undefined4 *)(pMVar7->locate).coord.m[0];
//     uVar9 = *(undefined4 *)((pMVar7->locate).coord.m[0] + 2);
//     uVar10 = *(undefined4 *)((pMVar7->locate).coord.m[1] + 1);
//     (pSVar4->locate).flg = (pMVar7->locate).flg;
//     *(undefined4 *)(pSVar4->locate).coord.m[0] = uVar8;
//     *(undefined4 *)((pSVar4->locate).coord.m[0] + 2) = uVar9;
//     *(undefined4 *)((pSVar4->locate).coord.m[1] + 1) = uVar10;
//     pMVar7 = (ModelType *)((pMVar7->locate).coord.m + 2);
//     pSVar4 = (Sprite3D *)((pSVar4->locate).coord.m + 2);
//   } while (pMVar7 != (ModelType *)pSVar5);
//   DrawSprite(sprt);
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? AdtMessageBox(? *, s32, u8);                      /* extern */
// ? DeleteConflict(void *, u8);                       /* extern */
// ? DrawSprite(void *);                               /* extern */
// s16 GetConflictResult(void *, ?);                   /* extern */
// s16 InsertConflict(void *);                         /* extern */
// ? MoveKorogari(void *, void *);                     /* extern */
// ? SetBleeds(void *, ?, ?, ?, s32, s32);             /* extern */
// ? SoundEx(void *, ?);                               /* extern */
// ? UpdateCoordinate(void *);                         /* extern */
// s32 is_character_state_present_on_stage_(s32);      /* extern */
// extern ? D_800121CC;
// extern ? ConflictObject;
//
// void ProcItemMakibishi(void *arg0) {
//     s16 var_a0_2;
//     u8 temp_a1;
//     u8 temp_v0;
//     u8 temp_v1;
//     void *temp_a0;
//     void *temp_s1;
//     void *temp_s3;
//     void *temp_v1_2;
//     void *temp_v1_3;
//     void *var_a0;
//     void *var_v0;
//
//     temp_s3 = arg0->unk4;
//     temp_s1 = arg0 + 0x20;
//     if (arg0->unk54 == 0xFF) {
//         arg0->unk54 = 0U;
//         return;
//     }
//     temp_v1 = arg0->unk54;
//     switch (temp_v1) {                              /* irregular */
//     case 0:
//         MoveKorogari(arg0, temp_s1);
//         temp_a1 = temp_s1->unkA;
//         if (temp_a1 != (u8) 1) {
//             if (temp_a1 == 4) {
//                 arg0->unk54 = (u8) (arg0->unk54 + 1);
//                 DeleteConflict(arg0->unk10, temp_a1);
//                 temp_v1_2 = (InsertConflict(arg0->unk10) * 0x78) + &ConflictObject;
//                 temp_v1_2->unk14 = 0;
//                 temp_v1_2->unk18 = 0;
//                 temp_v1_2->unk16 = 0;
//                 temp_v1_2->unk20 = 0x64;
//                 temp_v1_2->unk1E = 0x64;
//                 temp_v1_2->unk1C = 0x64;
//                 temp_v1_2->unk24 = 1;
//                 temp_v1_2->unk22 = (s16) 1;
//                 arg0->unk1C = 0x64;
//                 arg0->unk1E = 0;
//                 arg0->unk14 = 1;
//                 arg0->unk18 = 0;
//             }
//         default:
// block_20:
//             UpdateCoordinate(arg0->unk10);
//             var_a0 = arg0->unk10;
//             var_v0 = temp_s3;
//             temp_v1_3 = var_a0 + 0x50;
//             do {
//                 var_v0->unk0 = (s32) var_a0->unk0;
//                 var_v0->unk4 = (s32) var_a0->unk4;
//                 var_v0->unk8 = (s32) var_a0->unk8;
//                 var_v0->unkC = (s32) var_a0->unkC;
//                 var_a0 += 0x10;
//                 var_v0 += 0x10;
//             } while (var_a0 != temp_v1_3);
//             DrawSprite(temp_s3);
//         } else {
//             if (arg0->unkC != NULL) {
//                 arg0->unk54 = 0xFFU;
// block_17:
//                 arg0->unkC(arg0);
//                 DeleteConflict(arg0->unk10);
//                 temp_v0 = arg0->unk54;
//                 if (temp_v0 != 0) {
//                     AdtMessageBox(&D_800121CC, arg0->unk8, temp_v0);
//                 }
//                 arg0->unk0 = 0;
//                 arg0->unkC = NULL;
//                 return;
//             }
//             return;
//         }
//         break;
//     case 1:
//         temp_a0 = arg0->unk10;
//         if (!(temp_a0->unk5A & 0x8000)) {
//             var_a0_2 = -1;
//         } else {
//             var_a0_2 = GetConflictResult(temp_a0, -1);
//         }
//         if ((var_a0_2 != -1) && (is_character_state_present_on_stage_(((var_a0_2 * 0x78) + &ConflictObject)->unk24) != 0)) {
//             SetBleeds(arg0->unk10 + 0x18, 0, 0x14, 0xA, 0xF, 0x7F0000);
//             SoundEx(arg0->unk10 + 0x18, 0x30);
//             if (arg0->unkC != NULL) {
//                 arg0->unk54 = 0xFFU;
//                 goto block_17;
//             }
//         } else {
//             goto block_20;
//         }
//         break;
//     }
// }
