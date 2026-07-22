#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemKaengeki(struct tag_TItem *item);
 *     ITEM.C:2287, 54 src lines, frame 80 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     param $s3       struct tag_TItem * item
 *     reg   $s1       struct param_kaengeki * param
 *     reg   $s0       struct Humanoid * human
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+16     struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     stack sp+16     struct PARAM_ITEM_LAUNCH rp
 *
 * Globals it touches, as the original declared them:
 *     extern short ActionHalt;
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

#include "item.h"

/*
 * MATCH.
 *
 * ProcItemKaengeki (0x80043710) runs the fire-breath item.  It starts the
 * owner's use animation, drops the item if that animation is interrupted,
 * then spends 40 frames steering the owner and launching camera-relative
 * napalm requests.
 *
 * Matching notes:
 *  - `rp`, `rx`, and `ry` are one contiguous sp+0x10..0x3f working window.
 *    The mode-1 path views its first 0x28 bytes as the dropped-item request;
 *    mode 2 reuses the same request and the trailing two words as camera
 *    rotation outputs.
 *  - `mode_index = 0` is a zero-byte CSE eviction.  Naming the entry mode
 *    load and dead-overwriting that local before the switch makes
 *    expand_case emit the target's fresh second `lbu`; a direct switch after
 *    the entry guard incorrectly reuses the first load.
 *  - `ff` is an s32 caller-saved local in a1.  It remains live from the entry
 *    compare to mode 2's no-call dispose path, while mode 1 rematerializes
 *    ITEM_MODE_DISPOSE as a fresh 0xff value after its calls. That difference
 *    keeps the two dispose prefixes separate while cross-jump merges them at
 *    the indirect call.
 *  - The completed request assigns user before type.  This prevents the
 *    type's `li 22` from filling the steering guard's delay slot and yields
 *    the target load/li/store ordering at the request head.
 *  - The steering writes use direct compound expressions through
 *    `human->model`.  That lets CSE overwrite the dead human pointer with the
 *    model in v1; a named model assignment instead colors the model into a0.
 *  - The staged vector statements are intentional: copy rotated end to
 *    start, scale start by 12, add the saved origin, double end, then add
 *    start.  They reproduce both rounds of stack stores in the target.
 */
extern TCameraStatus CamState;
extern GsRVIEW2 ViewInfo;

extern s16 NowReturnNormal(Humanoid *human);
extern void GetVectorRotation(VECTOR *from, VECTOR *to, s32 *rx, s32 *ry);
extern void RotateVector(VECTOR *vec, s32 rx, s32 ry, s32 rz);
extern int ReqItemUse(PARAM_ITEM_LAUNCH *p);

void ProcItemKaengeki(TItem *item)
{
    param_kaengeki *param;
    PARAM_ITEM_LAUNCH rp;
    void (*ppu)(TItem *);
    s32 rx;
    s32 ry;
    s32 ff;
    u8 mode_index;

    param = &item->param.kaengeki;
    ff = ITEM_MODE_DISPOSE;
    mode_index = item->mode;
    if (mode_index == ff)
    {
        if (item->owner->motion->mid == 0xf04)
        {
            NowReturnNormal(item->owner);
        }
        item->mode = 0;
        return;
    }

    mode_index = 0;
    switch (item->mode)
    {
    case 0:
    {
        Humanoid *human;

        human = item->owner;
        if (ActionHalt == 0 && human->life > 0)
        {
            MotionDataType *motion;

            dispose_weapon_data_of_char_(human, 3);
            UpdateMotion(human->motion, 0xf04);
            human->status = 0xf;
            motion = human->motion->motion;
            MoveHumanoid(human, motion->orderspd, motion->sidespd);
        }
        Sound(item->owner, 0x4c);
        item->mode = item->mode + 1;
        return;
    }

    case 1:
    {
        MotionManager *motion;

        motion = item->owner->motion;
        if (motion->count == 0 && motion->loop != 0)
        {
            SoundEx((VECTOR *)item->owner->model->locate.coord.t, 0x28);
            item->mode = item->mode + 1;
            param->count = 0x28;
        }
        if (item->owner->motion->mid == 0xf04)
        {
            return;
        }
        {
            VECTOR *pos;
            Humanoid *human;
            s32 itemID;

            pos = GetAbsolutePosition(item->locate, 0, 0, 0);
            human = item->owner;
            itemID = item->type;
            memset(&rp, 0, sizeof(PARAM_ITEM_LAUNCH));
            rp.type = itemID;
            rp.user = human;
            rp.start.vx = pos->vx;
            rp.start.vy = pos->vy;
            rp.start.vz = pos->vz;
            rp.end.vx = rand() % 200 - 100;
            rp.end.vy = rand() % 100 - 200;
            rp.end.vz = rand() % 200 - 100;
            ReqItemDrop(&rp);
            ppu = item->proc;
            if (ppu == 0)
            {
                return;
            }
            item->mode = ITEM_MODE_DISPOSE;
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
    }

    case 2:
    {
        Humanoid *human;
        u16 buttons;
        u8 count;
        ModelArchiveType *model;
        s32 rz;

        if (item->owner->motion->mid != 0xf04)
        {
            goto dispose;
        }
        count = param->count - 1;
        param->count = count;
        if (count == 0)
        {
            goto dispose;
        }

        human = item->owner;
        buttons = human->pad.data;
        if ((buttons & 0x2000) != 0)
        {
            human->model->rotate.vy = human->model->rotate.vy + 0x20;
        }
        else if ((buttons & 0x8000) != 0)
        {
            human->model->rotate.vy = human->model->rotate.vy - 0x20;
        }

        rp.user = item->owner;
        rp.type = ITEM_NAPALM;
        rp.end.vx = param->end.vx;
        rp.end.vy = param->end.vy;
        rp.end.vz = param->end.vz;
        model = item->owner->model;
        if (CamState.Owner->model == model && CamState.Mode == 1)
        {
            GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx,
                              &rx, &ry);
            rz = 0;
        }
        else
        {
            rx = model->rotate.vx;
            rz = model->rotate.vz;
            ry = model->rotate.vy;
        }
        RotateVector(&rp.end, rx, ry, rz);

        rp.start.vx = rp.end.vx;
        rp.start.vy = rp.end.vy;
        rp.start.vz = rp.end.vz;
        rp.start.vx *= 12;
        rp.start.vy *= 12;
        rp.start.vz *= 12;
        rp.start.vx += param->start.vx;
        rp.start.vy += param->start.vy;
        rp.start.vz += param->start.vz;
        rp.end.vx *= 2;
        rp.end.vy *= 2;
        rp.end.vz *= 2;
        rp.end.vx += rp.start.vx;
        rp.end.vy += rp.start.vy;
        rp.end.vz += rp.start.vz;
        ReqItemUse(&rp);
        return;

dispose:
        if (item->proc == 0)
        {
            return;
        }
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
    }
}

// Historical Ghidra decompilation reference:
//
//
// void ProcItemKaengeki(tag_TItem *item)
//
// {
//   byte bVar1;
//   ushort uVar2;
//   uchar uVar3;
//   short sVar4;
//   MotionDataType *pMVar5;
//   VECTOR *pVVar6;
//   undefined **ppuVar7;
//   MotionManager *pMVar8;
//   ModelArchiveType *pMVar9;
//   int iVar10;
//   Humanoid *pHVar11;
//   TItemType TVar12;
//   PARAM_ITEM_LAUNCH local_48;
//   int local_20;
//   int local_1c;
//
//   if (item->mode == 0xff) {
//     if (item->owner->motion->mid == 0xf04) {
//       NowReturnNormal(item->owner);
//     }
//     item->mode = '\0';
//     return;
//   }
//   bVar1 = item->mode;
//   if (bVar1 == 1) {
//     pMVar8 = item->owner->motion;
//     if ((pMVar8->count == 0) && (pMVar8->loop != 0)) {
//       SoundEx((VECTOR *)(item->owner->model->locate).coord.t,0x28);
//       item->mode = item->mode + '\x01';
//       (item->param).kaengeki.count = '(';
//     }
//     if (item->owner->motion->mid == 0xf04) {
//       return;
//     }
//     pVVar6 = GetAbsolutePosition(item->locate,0,0,0);
//     pHVar11 = item->owner;
//     TVar12 = item->type;
//     memset((uchar *)&local_48,'\0',0x28);
//     local_48.start.vx = pVVar6->vx;
//     local_48.start.vy = pVVar6->vy;
//     local_48.start.vz = pVVar6->vz;
//     local_48.type = TVar12;
//     local_48.user = pHVar11;
//     iVar10 = rand();
//     local_48.end.vx = iVar10 % 200 + -100;
//     iVar10 = rand();
//     local_48.end.vy = iVar10 % 100 + -200;
//     iVar10 = rand();
//     local_48.end.vz = iVar10 % 200 + -100;
//     ReqItemDrop(&local_48);
//     ppuVar7 = item->proc;
//     if (ppuVar7 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
// LAB_80043bb4:
//     (*(code *)ppuVar7)(item);
//     DeleteConflict(item->locate);
//     if (item->mode != 0) {
//       AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//     }
//     item->owner = (Humanoid *)0x0;
//     item->proc = (undefined **)0x0;
//     return;
//   }
//   if (bVar1 < 2) {
//     if (bVar1 != 0) {
//       return;
//     }
//     pHVar11 = item->owner;
//     if ((ActionHalt == 0) && (0 < pHVar11->life)) {
//       FUN_800270c8(pHVar11,3);
//       UpdateMotion(pHVar11->motion,0xf04);
//       pHVar11->status = 0xf;
//       pMVar5 = pHVar11->motion->motion;
//       MoveHumanoid(pHVar11,(ushort)pMVar5->orderspd,(ushort)pMVar5->sidespd);
//     }
//     Sound(item->owner,0x4c);
//     item->mode = item->mode + '\x01';
//     return;
//   }
//   if (bVar1 != 2) {
//     return;
//   }
//   if ((item->owner->motion->mid != 0xf04) ||
//      (uVar3 = (item->param).kaengeki.count + 0xff, (item->param).kaengeki.count = uVar3,
//      uVar3 == '\0')) {
//     ppuVar7 = item->proc;
//     if (ppuVar7 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//     goto LAB_80043bb4;
//   }
//   pHVar11 = item->owner;
//   uVar2 = (pHVar11->pad).data;
//   if ((uVar2 & 0x2000) == 0) {
//     if ((uVar2 & 0x8000) == 0) goto LAB_80043a4c;
//     pMVar9 = pHVar11->model;
//     sVar4 = (pMVar9->rotate).vy + -0x20;
//   }
//   else {
//     pMVar9 = pHVar11->model;
//     sVar4 = (pMVar9->rotate).vy + 0x20;
//   }
//   (pMVar9->rotate).vy = sVar4;
// LAB_80043a4c:
//   local_48.user = item->owner;
//   local_48.type = ITEM_NAPALM;
//   local_48.end.vx = (item->param).kaengeki.end.vx;
//   local_48.end.vy = (item->param).kaengeki.end.vy;
//   local_48.end.vz = (item->param).kaengeki.end.vz;
//   pMVar9 = item->owner->model;
//   if (((CamState.Owner)->model == pMVar9) && (CamState.Mode == CMODE_DIRECTION)) {
//     GetVectorRotation((VECTOR *)&ViewInfo,(VECTOR *)&ViewInfo.vrx,&local_20,&local_1c);
//     iVar10 = 0;
//   }
//   else {
//     local_20 = (int)(pMVar9->rotate).vx;
//     iVar10 = (int)(pMVar9->rotate).vz;
//     local_1c = (int)(pMVar9->rotate).vy;
//   }
//   RotateVector(&local_48.end,local_20,local_1c,iVar10);
//   local_48.start.vx = (long)(&((item->param).smoke.koro.hint)->y + local_48.end.vx * 6);
//   local_48.start.vy = local_48.end.vy * 0xc + *(int *)&(item->param).smoke.koro.vx;
//   local_48.end.vx = local_48.start.vx + local_48.end.vx * 2;
//   local_48.end.vy = local_48.end.vy * 2 + local_48.start.vy;
//   local_48.start.vz = local_48.end.vz * 0xc + *(int *)&(item->param).smoke.koro.vz;
//   local_48.end.vz = local_48.end.vz * 2 + local_48.start.vz;
//   ReqItemUse(&local_48);
//   return;
// }
