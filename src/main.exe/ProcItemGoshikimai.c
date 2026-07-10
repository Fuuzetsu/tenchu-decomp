#include "common.h"
#include "main.exe.h"

/*
 * ProcItemGoshikimai (0x8004333c) — the goshikimai (five-colored rice charm)
 * item processor. mode 0: freeze the owner (dispose weapon, throw animation
 * 0xF03, status 8), nudge into place, advance to mode 1; mode 1: once the
 * throw animation reaches frame 0xF, build a PARAM_ITEM_USE from the owner's
 * held-object bone (object[0xd]) and the item's stashed end-velocity, restore
 * the owner's normal stance (NowReturnNormal), dispose the item, then spawn
 * the follow-up dropped/thrown item with ReqItemDrop.
 *
 * Matching notes (see also ProcItemKusuri.c/ReqItemGoshikimai.c for the
 * item-TU conventions):
 *  - `pp = (param_korogari *)item->param;` is the VERY FIRST statement
 *    (before even the entry mode==0xff test): the addiu fills the entry
 *    branch's delay slot, same lever as ReqItemGoshikimai's `pp` — and like
 *    that twin, none of param_korogari's NAMED fields are read; pp is purely
 *    a cast vehicle for the raw offset-2/offset-4 accesses below.
 *  - `if (mode==ff)` (a plain if, separate statement) and the `switch
 *    (item->mode)` right after each get their OWN fresh lbu of item->mode —
 *    two total reloads, matching the switch rule (expand_case always
 *    re-reads the discriminant). The switch has NO default: mode values
 *    other than 0/1 fall out of the switch with nothing after it (straight
 *    to the epilogue, mode untouched) — a real function-level fallthrough,
 *    not a case.
 *  - `item->mode = 0; return;` is written OUT TWICE — once as the entry `ff`
 *    guard, once at the end of case 1 (when `mid != 0xf03`) — not shared via
 *    a goto/post-switch tail. GCC's cross-jump pass merges the two
 *    identical copies from the `sb`/`j` backwards (the cookbook's "shared
 *    tails" rule); a shared label would merge MORE than the original since
 *    the two call sites differ.
 *  - Case 0 mirrors ProcItemKusuri's case 0 exactly (dispose/animate/status/
 *    MoveHumanoid), just different animation id (0xf03) and status (8).
 *  - `item->param`'s three stashed s16 fields are read with the SAME
 *    fresh-vs-cached asymmetry ReqItemGoshikimai writes them with: offset 0
 *    is read through a fresh `item->param` cast (not `pp`), offsets 2 and 4
 *    through `pp` — matching bases exactly mirror the twin's store bases.
 *  - `item->owner->model->object[0xd]` is recomputed in full for EACH of the
 *    three GetAbsolutePosition calls (three separate jal's in the asm, no
 *    cached model/object pointer) — Ghidra's literal repetition is the
 *    source's real shape, not a decompiler artifact.
 *  - The dispose tail reuses `ff` (the same 0xff local tested at entry) for
 *    `item->mode = ff`, like every other ProcItem*.
 */
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemGoshikimai(struct tag_TItem *item);
 *     ITEM.C:2223, 35 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     param $s1       struct tag_TItem * item
 *     reg   $s0       struct param_goshikimai * param
 *     reg   $s0       struct Humanoid * human
 *     stack sp+16     struct PARAM_ITEM_LAUNCH p
 *     reg   $s1       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern short ActionHalt;
 * END PSX.SYM */

extern void NowReturnNormal(Humanoid *h);

void ProcItemGoshikimai(tag_TItem *item)
{
    param_korogari *pp;
    Humanoid *own;
    MotionDataType *md;
    MotionManager *mot;
    PARAM_ITEM_USE prm;
    u8 ff;

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
        own = item->owner;
        if (ActionHalt == 0 && 0 < own->life)
        {
            dispose_weapon_data_of_char_(own, 3);
            UpdateMotion(own->motion, 0xf03);
            own->status = 8;
            md = own->motion->motion;
            MoveHumanoid(own, md->orderspd, md->sidespd);
        }
        item->mode = item->mode + 1;
        return;

    case 1:
        mot = item->owner->motion;
        if (mot->mid != 0xf03)
        {
            item->mode = 0;
            return;
        }
        if (mot->count != 0xf)
            return;
        prm.type = ITEM_KIND_2_GOSIKIMAI;
        prm.user = item->owner;
        prm.start.vx = GetAbsolutePosition(item->owner->model->object[0xd], 0, 0, 0)->vx;
        prm.start.vy = GetAbsolutePosition(item->owner->model->object[0xd], 0, 0, 0)->vy;
        prm.start.vz = GetAbsolutePosition(item->owner->model->object[0xd], 0, 0, 0)->vz;
        prm.end.vx = *(s16 *)item->param;
        prm.end.vy = *(s16 *)((u8 *)pp + 2);
        prm.end.vz = *(s16 *)((u8 *)pp + 4);
        NowReturnNormal(item->owner);
        if (item->proc != 0)
        {
            item->mode = ff;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(D_800121CC, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
        }
        ReqItemDrop(&prm);
        return;
    }
}

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcItemGoshikimai(tag_TItem *item)
//
// {
//   MotionDataType *pMVar1;
//   VECTOR *pVVar2;
//   MotionManager *pMVar3;
//   Humanoid *human;
//   PARAM_ITEM_USE local_38;
//
//   if (item->mode != 0xff) {
//     if (item->mode == '\0') {
//       human = item->owner;
//       if ((ActionHalt == 0) && (0 < human->life)) {
//         FUN_800270c8(human,3);
//         UpdateMotion(human->motion,0xf03);
//         human->status = 8;
//         pMVar1 = human->motion->motion;
//         MoveHumanoid(human,(ushort)pMVar1->orderspd,(ushort)pMVar1->sidespd);
//       }
//       item->mode = item->mode + '\x01';
//       return;
//     }
//     if (item->mode != '\x01') {
//       return;
//     }
//     pMVar3 = item->owner->motion;
//     if (pMVar3->mid == 0xf03) {
//       if (pMVar3->count != 0xf) {
//         return;
//       }
//       local_38.type = ITEM_GOSHIKIMAI;
//       local_38.user = item->owner;
//       pVVar2 = GetAbsolutePosition(item->owner->model->object[0xd],0,0,0);
//       local_38.start.vx = pVVar2->vx;
//       pVVar2 = GetAbsolutePosition(item->owner->model->object[0xd],0,0,0);
//       local_38.start.vy = pVVar2->vy;
//       pVVar2 = GetAbsolutePosition(item->owner->model->object[0xd],0,0,0);
//       local_38.start.vz = pVVar2->vz;
//       local_38.end.vx = (long)(item->param).napalm.vec.vx;
//       local_38.end.vy = (long)(item->param).napalm.vec.vy;
//       local_38.end.vz = (long)(item->param).napalm.vec.vz;
//       NowReturnNormal(item->owner);
//       if (item->proc != (undefined **)0x0) {
//         item->mode = 0xff;
//         (*(code *)item->proc)(item);
//         DeleteConflict(item->locate);
//         if (item->mode != 0) {
//           AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//         }
//         item->owner = (Humanoid *)0x0;
//         item->proc = (undefined **)0x0;
//       }
//       ReqItemDrop(&local_38);
//       return;
//     }
//   }
//   item->mode = '\0';
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? AdtMessageBox(? *, s32, u8);                      /* extern */
// ? DeleteConflict(s32);                              /* extern */
// s32 *GetAbsolutePosition(s32, ?, ?, ?);             /* extern */
// ? MoveHumanoid(void *, u8, u8);                     /* extern */
// ? NowReturnNormal(void *);                          /* extern */
// ? ReqItemDrop(s32 *);                               /* extern */
// ? UpdateMotion(void *, ?);                          /* extern */
// ? dispose_weapon_data_of_char_(void *, ?);          /* extern */
// extern s16 ActionHalt;
// extern ? D_800121CC;
//
// void ProcItemGoshikimai(void *arg0) {
//     s32 sp10;
//     void *sp14;
//     s32 sp18;
//     s32 sp1C;
//     s32 sp20;
//     s32 sp28;
//     s32 sp2C;
//     s32 sp30;
//     ? (*temp_v0_2)(void *);
//     u8 temp_v0_3;
//     u8 temp_v1;
//     void *temp_a0;
//     void *temp_s0;
//     void *temp_s0_2;
//     void *temp_v0;
//
//     temp_s0 = arg0 + 0x20;
//     if (arg0->unk54 != 0xFF) {
//         temp_v1 = arg0->unk54;
//         switch (temp_v1) {                          /* irregular */
//         case 0:
//             temp_s0_2 = arg0->unk0;
//             if ((ActionHalt == 0) && (temp_s0_2->unk8 > 0)) {
//                 dispose_weapon_data_of_char_(temp_s0_2, 3);
//                 UpdateMotion(temp_s0_2->unk5C, 0xF03);
//                 temp_s0_2->unk2 = 8;
//                 temp_v0 = temp_s0_2->unk5C->unk10;
//                 MoveHumanoid(temp_s0_2, temp_v0->unk2, temp_v0->unk3);
//             }
//             arg0->unk54 = (u8) (arg0->unk54 + 1);
//             return;
//         case 1:
//             temp_a0 = arg0->unk0->unk5C;
//             if (temp_a0->unk0 != 0xF03) {
//                 goto block_9;
//             }
//             if (temp_a0->unk2 == 0xF) {
//                 sp10 = 8;
//                 sp14 = arg0->unk0;
//                 sp18 = *GetAbsolutePosition(arg0->unk0->unk58->unk68->unk34, 0, 0, 0);
//                 sp1C = GetAbsolutePosition(arg0->unk0->unk58->unk68->unk34, 0, 0, 0)->unk4;
//                 sp20 = GetAbsolutePosition(arg0->unk0->unk58->unk68->unk34, 0, 0, 0)->unk8;
//                 sp28 = (s32) arg0->unk20;
//                 sp2C = (s32) temp_s0->unk2;
//                 sp30 = (s32) temp_s0->unk4;
//                 NowReturnNormal(arg0->unk0);
//                 temp_v0_2 = arg0->unkC;
//                 if (temp_v0_2 != NULL) {
//                     arg0->unk54 = 0xFFU;
//                     temp_v0_2(arg0);
//                     DeleteConflict(arg0->unk10);
//                     temp_v0_3 = arg0->unk54;
//                     if (temp_v0_3 != 0) {
//                         AdtMessageBox(&D_800121CC, arg0->unk8, temp_v0_3);
//                     }
//                     arg0->unk0 = NULL;
//                     arg0->unkC = NULL;
//                 }
//                 ReqItemDrop(&sp10);
//             }
//             return;
//         }
//     } else {
// block_9:
//         arg0->unk54 = 0U;
//     }
// }
