#include "common.h"
#include "main.exe.h"

/*
 * ProcItemTeleport (0x8003f18c) — the teleport bell item processor. If the
 * owner's pad state has the sight-mode bit set (pad.data & 0x10), just
 * re-enter sight camera mode and re-sort the target sprite (no dispose: the
 * item stays armed). Otherwise snap the camera's target vector; if it landed
 * within range (distance² < 20000) of the camera owner's model, warp the
 * owner's model there and spray SetBleeds particles at the target. Either
 * way, drop back to normal camera mode and dispose of the item (the usual
 * proc/DeleteConflict/owner-clear tail shared by every ProcItem*).
 *
 * Matching notes (see also ProcItemKusuri.c for the item-TU conventions):
 *  - `ff = ITEM_MODE_DISPOSE` (0xff; a callee-saved reg, $s3) is tested at
 *    entry AND reused
 *    verbatim for the dispose store `item->mode = ff` — this function never
 *    repurposes that register for a scratch buffer (unlike Kusuri's mode 2),
 *    so the SAME register feeds both the compare and the later store.
 *  - The pad-bit branch is Ghidra's polarity flipped: the real source is an
 *    early-return guard (`if (pad.data & 0x10) { sight-mode; return; }`),
 *    not Ghidra's `if (==0) {snap} else {sight}` — the asm falls through to
 *    the snap-camera code and only branches away for the sight-mode case.
 *  - `CamState.Owner->model->locate.coord.t[N] = ...` is written as three
 *    separate full-chain statements, not through a cached pointer: the asm
 *    reloads CamState.Owner and ->model fresh for each of the three stores
 *    (no `own`/`md` temp lives across them), matching the repeated-access
 *    idiom (contrast the cached-pointer rule, which applies when the asm
 *    instead shows ONE load surviving across several uses).
 *  - The dispose tail reuses the proven idiom: `ppu = item->proc; if (ppu ==
 *    0) return; item->mode = ff; item->proc(item);` — checking through `ppu`
 *    but calling through the field lets cse fold the reload, landing the
 *    pointer in $v0 (Kusuri/Manebue/Drop's rule).
 */
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemTeleport(struct tag_TItem *item);
 *     ITEM.C:1097, 39 src lines, frame 48 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE TargetSprite[1];
 *     extern struct GsOT *OTablePt;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

extern void SnapCameraTargetVector(void);
extern GsOT *OTablePt;

void ProcItemTeleport(TItem *item)
{
    void (*ppu)(TItem *);
    u8 ff;

    ff = ITEM_MODE_DISPOSE;
    if (item->mode == ff)
    {
        item->mode = 0;
        return;
    }
    if ((item->owner->pad.data & 0x10) != 0)
    {
        SetCameraMode(CMODE_SIGHT);
        GsSortSprite(TargetSprite, OTablePt, 0);
        return;
    }
    SnapCameraTargetVector();
    if (GetVectorDistance((VECTOR *)CamState.Owner->model->locate.coord.t, &CamState.TargetVector) < 20000)
    {
        CamState.Owner->model->locate.coord.t[0] = CamState.TargetVector.vx;
        CamState.Owner->model->locate.coord.t[1] = CamState.TargetVector.vy;
        CamState.Owner->model->locate.coord.t[2] = CamState.TargetVector.vz;
        SetBleeds(&CamState.TargetVector, 1000, 20, 50, 60, 0xffffff);
    }
    SetCameraMode(CMODE_NORMAL);
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
}

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcItemTeleport(tag_TItem *item)
//
// {
//   int iVar1;
//
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//   }
//   else if (((item->owner->pad).data & 0x10) == 0) {
//     SnapCameraTargetVector();
//     iVar1 = GetVectorDistance((VECTOR *)((CamState.Owner)->model->locate).coord.t,
//                               &CamState.TargetVector);
//     if (iVar1 < 20000) {
//       ((CamState.Owner)->model->locate).coord.t[0] = CamState.TargetVector.vx;
//       ((CamState.Owner)->model->locate).coord.t[1] = CamState.TargetVector.vy;
//       ((CamState.Owner)->model->locate).coord.t[2] = CamState.TargetVector.vz;
//       SetBleeds(-0x6110,1000,0x14);
//     }
//     SetCameraMode(CMODE_NORMAL);
//     if (item->proc != (undefined **)0x0) {
//       item->mode = 0xff;
//       (*(code *)item->proc)(item);
//       DeleteConflict(item->locate);
//       if (item->mode != 0) {
//         AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//       }
//       item->owner = (Humanoid *)0x0;
//       item->proc = (undefined **)0x0;
//     }
//   }
//   else {
//     SetCameraMode(CMODE_SIGHT);
//     GsSortSprite(TargetSprite,OTablePt,0);
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? AdtMessageBox(? *, s32, u8);                      /* extern */
// ? DeleteConflict(s32);                              /* extern */
// s32 GetVectorDistance(s32, ? *);                    /* extern */
// ? GsSortSprite(? *, s32, ?);                        /* extern */
// ? SetBleeds(? *, ?, ?, ?, s32, s32);                /* extern */
// ? SetCameraMode(?);                                 /* extern */
// ? SnapCameraTargetVector();                         /* extern */
// extern ? CamState;
// extern ? D_800121CC;
// extern ? D_800C0600;
// extern s32 OTablePt;
//
// void ProcItemTeleport(void *arg0) {
//     ? (*temp_v0)(void *);
//     u8 temp_v0_2;
//
//     if (arg0->unk54 == 0xFF) {
//         arg0->unk54 = 0U;
//         return;
//     }
//     if (arg0->unk0->unk10 & 0x10) {
//         SetCameraMode(3);
//         GsSortSprite(&D_800C0600, OTablePt, 0);
//         return;
//     }
//     SnapCameraTargetVector();
//     if (GetVectorDistance(CamState.unk10->unk58 + 0x18, &CamState) < 0x4E20) {
//         CamState.unk10->unk58->unk18 = (s32) CamState.unk0;
//         CamState.unk10->unk58->unk1C = (s32) CamState.unk4;
//         CamState.unk10->unk58->unk20 = (s32) CamState.unk8;
//         SetBleeds(&CamState, 0x3E8, 0x14, 0x32, 0x3C, 0xFFFFFF);
//     }
//     SetCameraMode(0);
//     temp_v0 = arg0->unkC;
//     if (temp_v0 != NULL) {
//         arg0->unk54 = 0xFFU;
//         temp_v0(arg0);
//         DeleteConflict(arg0->unk10);
//         temp_v0_2 = arg0->unk54;
//         if (temp_v0_2 != 0) {
//             AdtMessageBox(&D_800121CC, arg0->unk8, temp_v0_2);
//         }
//         arg0->unk0 = NULL;
//         arg0->unkC = NULL;
//     }
// }
