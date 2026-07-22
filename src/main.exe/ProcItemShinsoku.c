#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemShinsoku(struct tag_TItem *item);
 *     ITEM.C:1324, 55 src lines, frame 104 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     reg   $s1       struct param_shinsoku * param
 *     stack sp+24     struct VECTOR pos
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+40     struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s0       struct VECTOR * apos
 *     stack sp+56     struct MapVector map
 *     stack sp+40     struct VECTOR pos
 *     reg   $s3       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

/*
 * MATCH.
 *
 * ProcItemShinsoku (0x8003f8a0, ITEM.C:1324) drives the rapid-movement item:
 * it starts/monitors motion 0xf05, drops itself if that motion is interrupted,
 * moves the owner's model with a floor query while steering from the pad, emits
 * a periodic effect, and restores the normal motion/camera before disposal.
 *
 * Matching notes:
 *  - The stack is two adjacent source objects: `pos_storage` at sp+0x28 and
 *    `launch_buf` at sp+0x38.  The latter is deliberately re-viewed as the
 *    mode-1 PARAM_ITEM_LAUNCH, mode-2 query/map, and periodic-effect VECTOR.  This
 *    gives stackplan's exact 0x38-byte working window and 0x78 frame.
 *  - `launchp = 0` after memset is a zero-byte CSE eviction.  Without that dead
 *    reassignment, cse2 keeps `&launch_buf` in an extra callee-saved register
 *    through all three rand calls, adding an s5 save/restore and growing the
 *    frame.  Eviction makes both call sites re-materialize sp+0x38 like target.
 *  - The movement position is built through direct `pos_storage` writes, then
 *    `apos = &pos_storage` is assigned only for the query/level span.  The final
 *    model-coordinate copies must return to direct stack reads; using `apos`
 *    there emits s0-relative loads instead.
 *  - The validity flag is assigned at the END of both comparison arms.  reorg
 *    then places `valid=0`/`valid=1` in the two guard delay slots and global
 *    allocation reuses the comparison's v0, rather than the dead a1 map arg.
 *  - The camera high-base is assigned independently on the effect and skipped
 *    paths.  `(u32 *)0x80090000` plus the -0x6100 load offset produces the two
 *    target `lui v0,0x8009` definitions: one in the count guard's delay slot,
 *    and one after the effect call that clobbers v0.
 *  - The first mode-2 motion check dereferences `item->owner` directly; sharing
 *    the later pad-control `human` local changes a0 to a2 at all three loads.
 *  - The human-shaped `CamState.Owner` access emits two CamState HI16
 *    relocations around the effect call and one shared LO16 field load.  With
 *    CamState pinned to its retail address, those relocations resolve to the
 *    exact words formerly produced by the duplicated numeric-base scaffold.
 */


extern TCameraStatus CamState;

extern short SetNowMotion(Humanoid *human, short mid, short move);
extern short Sound(Humanoid *human, s32 seid);
extern void FUN_80033bc0(VECTOR *pos, s32 spread, s32 divisor, s32 count);
extern s32 GetAreaMapVector(u_long *area, MapVector *map,
                            VECTOR *position, s32 width, s32 mode);
extern void FUN_8003944c(VECTOR *pos, ModelArchiveType *model, s32 a, s32 b,
                         s32 color, s32 f, s32 rot, s32 h, s32 i, s32 j);

void ProcItemShinsoku(TItem *item)
{
    param_shinsoku *param;
    u8 ff;
    VECTOR pos_storage;
    u8 launch_buf[sizeof(PARAM_ITEM_LAUNCH)];

    param = &item->param.shinsoku;
    ff = ITEM_MODE_DISPOSE;
    if (item->mode == ff)
    {
        item->mode = 0;
        return;
    }

    switch (item->mode)
    {
    case 0:
        SetNowMotion(item->owner, 0xf05, 1);
        Sound(item->owner, 0x4c);
        item->mode = item->mode + 1;
        return;

    case 1:
    {
        MotionManager *motion;

        motion = item->owner->motion;
        if (motion->mid != 0xf05)
        {
            VECTOR *pos;
            Humanoid *human;
            s32 itemID;
            s32 rand_x;
            s32 rand_y;
            s32 rand_z;
            PARAM_ITEM_LAUNCH *launchp;

            pos = GetAbsolutePosition(item->locate, 0, 0, 0);
            human = item->owner;
            itemID = item->type;
            launchp = (PARAM_ITEM_LAUNCH *)launch_buf;
            memset(launchp, 0, sizeof(PARAM_ITEM_LAUNCH));
            launchp = 0;
            ((PARAM_ITEM_LAUNCH *)launch_buf)->type = itemID;
            ((PARAM_ITEM_LAUNCH *)launch_buf)->user = human;
            ((PARAM_ITEM_LAUNCH *)launch_buf)->start.vx = pos->vx;
            ((PARAM_ITEM_LAUNCH *)launch_buf)->start.vy = pos->vy;
            ((PARAM_ITEM_LAUNCH *)launch_buf)->start.vz = pos->vz;
            rand_x = rand();
            ((PARAM_ITEM_LAUNCH *)launch_buf)->end.vx = rand_x % 200 - 100;
            rand_y = rand();
            ((PARAM_ITEM_LAUNCH *)launch_buf)->end.vy = rand_y % 100 - 200;
            rand_z = rand();
            ((PARAM_ITEM_LAUNCH *)launch_buf)->end.vz = rand_z % 200 - 100;
            ReqItemDrop((PARAM_ITEM_LAUNCH *)launch_buf);
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
        if (motion->count != 0)
        {
            return;
        }
        if (motion->loop == 0)
        {
            return;
        }
        FUN_80033bc0(item->owner->locate, 0x96, 0xc, 8);
        param->count = 0x4b;
        item->mode = item->mode + 1;
        return;
    }

    case 2:
    {
        Humanoid *human;
        ModelArchiveType *model;
        s32 valid;
        u16 buttons;
        s32 rotate;
        VECTOR *apos;

        if (item->owner->motion->mid != 0xf05)
        {
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

        pos_storage.vx = item->owner->model->locate.coord.t[0];
        pos_storage.vy = item->owner->model->locate.coord.t[1];
        pos_storage.vz = item->owner->model->locate.coord.t[2];
        pos_storage.vx += param->vec.vx;
        pos_storage.vy += param->vec.vy;
        pos_storage.vz += param->vec.vz;
        apos = &pos_storage;
        ((VECTOR *)launch_buf)->vx = apos->vx;
        ((VECTOR *)launch_buf)->vy = apos->vy;
        ((VECTOR *)launch_buf)->vz = apos->vz;
        ((VECTOR *)launch_buf)->vy -= 2000;
        GetAreaMapVector(GlobalAreaMap,
                         (MapVector *)(launch_buf + 0x10),
                         (VECTOR *)launch_buf, 500, 0);
        if (((MapVector *)(launch_buf + 0x10))->level >= apos->vy - 500)
        {
            if (((MapVector *)(launch_buf + 0x10))->level < apos->vy)
            {
                apos->vy = ((MapVector *)(launch_buf + 0x10))->level;
            }
            valid = 1;
        }
        else
        {
            valid = 0;
        }
        if (valid != 0)
        {
            item->owner->model->locate.coord.t[0] = pos_storage.vx;
            item->owner->model->locate.coord.t[1] = pos_storage.vy;
            item->owner->model->locate.coord.t[2] = pos_storage.vz;
        }

        if ((param->count & 3) == 0)
        {
            *(VECTOR *)launch_buf =
                *(VECTOR *)item->owner->model->locate.coord.t;
            ((VECTOR *)launch_buf)->vy -= 300;
            FUN_8003944c((VECTOR *)launch_buf, 0, 0x2000, 0x5000,
                         0x808080, 0, 0, -30, 0x10, 3);
        }
        if (CamState.Owner == item->owner)
        {
            SetCameraMode(CMODE_CROUCH);
        }

        human = item->owner;
        buttons = human->pad.data;
        if ((buttons & 0x2000) != 0)
        {
            model = human->model;
            rotate = 0x40;
            model->rotate.vy += rotate;
            RotateVectorS(&param->vec, 0, rotate, 0);
        }
        else if ((buttons & 0x8000) != 0)
        {
            model = human->model;
            rotate = -0x40;
            model->rotate.vy += rotate;
            RotateVectorS(&param->vec, 0, rotate, 0);
        }

        param->count = param->count - 1;
        if (param->count != 0 && (item->owner->pad.trig & 0xf0) == 0)
        {
            return;
        }
        NowReturnNormal(item->owner);
        SetCameraMode(CMODE_NORMAL);
        if (item->proc == 0)
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
}

// triage: HARD — 336 insns, mul/div, indirect-call, 14 callees, ~0.27 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ProcItemShinsoku(tag_TItem *item)
//
// {
//   byte bVar1;
//   ushort uVar2;
//   bool bVar3;
//   uchar uVar4;
//   VECTOR *pVVar5;
//   undefined **ppuVar6;
//   ModelArchiveType *pMVar7;
//   MotionManager *pMVar8;
//   int iVar9;
//   Humanoid *pHVar10;
//   TItemType TVar11;
//   int local_4c;
//   PARAM_ITEM_LAUNCH local_40;
//
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//     return;
//   }
//   bVar1 = item->mode;
//   if (bVar1 == 1) {
//     pMVar8 = item->owner->motion;
//     if (pMVar8->mid == 0xf05) {
//       if (pMVar8->count != 0) {
//         return;
//       }
//       if (pMVar8->loop == 0) {
//         return;
//       }
//       FUN_80033bc0(item->owner->locate,0x96,0xc,8);
//       (item->param).napalm.count = 'K';
//       goto LAB_8003fab0;
//     }
//     pVVar5 = GetAbsolutePosition(item->locate,0,0,0);
//     pHVar10 = item->owner;
//     TVar11 = item->type;
//     memset((uchar *)&local_40,'\0',0x28);
//     local_40.start.vx = pVVar5->vx;
//     local_40.start.vy = pVVar5->vy;
//     local_40.start.vz = pVVar5->vz;
//     local_40.type = TVar11;
//     local_40.user = pHVar10;
//     iVar9 = rand();
//     local_40.end.vx = iVar9 % 200 + -100;
//     iVar9 = rand();
//     local_40.end.vy = iVar9 % 100 + -200;
//     iVar9 = rand();
//     local_40.end.vz = iVar9 % 200 + -100;
//     ReqItemDrop(&local_40);
//     ppuVar6 = item->proc;
//     if (ppuVar6 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//     goto LAB_8003fd84;
//   }
//   if (bVar1 < 2) {
//     if (bVar1 != 0) {
//       return;
//     }
//     SetNowMotion(item->owner,0xf05,1);
//     Sound(item->owner,0x4c);
// LAB_8003fab0:
//     item->mode = item->mode + '\x01';
//     return;
//   }
//   if (bVar1 != 2) {
//     return;
//   }
//   if (item->owner->motion->mid != 0xf05) {
//     ppuVar6 = item->proc;
//     if (ppuVar6 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//     goto LAB_8003fd84;
//   }
//   TVar11 = (item->owner->model->locate).coord.t[0] + (int)(item->param).napalm.vec.vx;
//   local_4c = (item->owner->model->locate).coord.t[1] + (int)(item->param).napalm.vec.vy;
//   iVar9 = (item->owner->model->locate).coord.t[2] + (int)(item->param).napalm.vec.vz;
//   local_40.user = (Humanoid *)(local_4c + -2000);
//   local_40.type = TVar11;
//   local_40.start.vx = iVar9;
//   GetAreaMapVector((MapVector *)GlobalAreaMap,(VECTOR *)&local_40.start.vz,(long)&local_40);
//   bVar3 = false;
//   if ((local_4c + -500 <= local_40.start.vz) && (bVar3 = true, local_40.start.vz < local_4c)) {
//     local_4c = local_40.start.vz;
//   }
//   if (bVar3) {
//     (item->owner->model->locate).coord.t[0] = TVar11;
//     (item->owner->model->locate).coord.t[1] = local_4c;
//     (item->owner->model->locate).coord.t[2] = iVar9;
//   }
//   if (((item->param).napalm.count & 3) == 0) {
//     pMVar7 = item->owner->model;
//     local_40.type = (pMVar7->locate).coord.t[0];
//     local_40.start.vx = (pMVar7->locate).coord.t[2];
//     local_40.start.vy = *(long *)(pMVar7->locate).workm.m[0];
//     local_40.user = (Humanoid *)((pMVar7->locate).coord.t[1] + -300);
//     FUN_8003944c(&local_40,0,0x2000,0x5000,0x808080,0,0,0xffffffe2,0x10,3);
//   }
//   if (CamState.Owner == item->owner) {
//     SetCameraMode(CMODE_CROUCH);
//   }
//   pHVar10 = item->owner;
//   uVar2 = (pHVar10->pad).data;
//   if ((uVar2 & 0x2000) == 0) {
//     if ((uVar2 & 0x8000) != 0) {
//       pMVar7 = pHVar10->model;
//       iVar9 = -0x40;
//       goto LAB_8003fd0c;
//     }
//   }
//   else {
//     pMVar7 = pHVar10->model;
//     iVar9 = 0x40;
// LAB_8003fd0c:
//     (pMVar7->rotate).vy = (pMVar7->rotate).vy + (short)iVar9;
//     RotateVectorS((SVECTOR *)&(item->param).launch,0,iVar9,0);
//   }
//   uVar4 = (item->param).napalm.count + 0xff;
//   (item->param).napalm.count = uVar4;
//   if ((uVar4 != '\0') && (((item->owner->pad).trig & 0xf0) == 0)) {
//     return;
//   }
//   NowReturnNormal(item->owner);
//   SetCameraMode(CMODE_NORMAL);
//   ppuVar6 = item->proc;
//   if (ppuVar6 == (undefined **)0x0) {
//     return;
//   }
//   item->mode = 0xff;
// LAB_8003fd84:
//   (*(code *)ppuVar6)(item);
//   DeleteConflict(item->locate);
//   if (item->mode != 0) {
//     AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//   }
//   item->owner = (Humanoid *)0x0;
//   item->proc = (undefined **)0x0;
//   return;
// }
