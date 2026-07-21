#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemNinken(struct tag_TItem *item);
 *     ITEM.C:2368, 89 src lines, frame 64 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $s2       struct tag_TItem * item
 *     reg   $s1       struct param_ninken * param
 *     reg   $s2       struct tag_TItem * item
 *     stack sp+24     struct SVECTOR vec
 *     stack sp+32     struct SVECTOR vec
 *     reg   $s2       struct tag_TItem * item
 *     reg   $s0       unsigned long at
 *     reg   $s0       struct Humanoid * target
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern long GameClock;
 * END PSX.SYM */

typedef union
{
    struct
    {
        PARAM_ITEM_STAY saved;
        u8 pad0[4];
        PARAM_ITEM_STAY rparam;
        u8 pad1[4];
        PARAM_ITEM_USE launch;
    } drop;
    struct
    {
        VECTOR pos;
        VECTOR query;
        MapVector map;
    } spawn;
    SVECTOR effect_vec;
} ProcItemNinkenScratch;

extern u_long *GlobalAreaMap;
extern s32 GameClock;
extern Humanoid *NINKEN_CHARACTER_PTR;
extern SVECTOR D_80097AF4[];

extern s16 NowReturnNormal(Humanoid *human);
extern void MoveKorogari(tag_TItem *item, param_korogari *param);
extern Humanoid *GetHumanoid(s16 type);
extern Humanoid *BreedLife(s16 type, s32 x, s32 y, s32 z, s32 r);
extern s32 GetAreaMapVector(u_long *area, MapVector *map,
                            VECTOR *position, s32 width, s32 mode);
extern s32 is_character_state_present_on_stage_(Humanoid *human);
extern s16 EquipWeapon(Humanoid *human, s16 mode);
extern s16 SetNowMotion(Humanoid *human, s16 mid, s16 move);
extern void FUN_800270f8(Humanoid *human, s16 hide);
extern void SetupThinkFunction(Humanoid *human, s16 think);
extern Humanoid *GetNearestHumanoid(Humanoid *human, s16 distance);
extern void TurnAroundAllItems(Humanoid *human);
extern long GetAreaMapLevel(u_long *area, long x, long y, long z, int mode);

void ProcItemNinken(tag_TItem *item)
{
    param_ninken *param;
    u8 ff;
    s32 one;
    ProcItemNinkenScratch scratch;

    param = (param_ninken *)item->param;
    ff = 0xff;
    if (item->mode == ff)
    {
        Humanoid *slave;

        slave = param->slave;
        if (slave != 0)
        {
            NowReturnNormal(slave);
            param->slave->attribute |= 0x80;
            param->slave->model->locate.coord.t[0] = 999000;
            param->slave->model->locate.coord.t[1] = 999000;
            param->slave->model->locate.coord.t[2] = 999000;
            UpdateCoordinate((ModelType *)param->slave->model);
        }
        item->mode = 0;
        return;
    }

    one = 1;
    switch (item->mode)
    {
    case 0:
    {
        u8 status;

        MoveKorogari(item, &param->koro);
        status = param->koro.status;
        if (status == one)
        {
            void (*dispose_proc)(tag_TItem *);

            dispose_proc = item->proc;
            if (dispose_proc == 0)
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
        if (status == 4)
        {
            PARAM_ITEM_STAY *saved;
            PARAM_ITEM_USE *launch;

            memset(&scratch.drop.rparam, 0, sizeof(PARAM_ITEM_STAY));
            scratch.drop.rparam.type = item->type;
            scratch.drop.rparam.locate.vx =
                item->locate->locate.coord.t[0];
            scratch.drop.rparam.locate.vy =
                item->locate->locate.coord.t[1];
            scratch.drop.rparam.locate.vz =
                item->locate->locate.coord.t[2];
            scratch.drop.saved = scratch.drop.rparam;

            if (item->proc != 0)
            {
                item->mode = ff;
                item->proc(item);
                DeleteConflict(item->locate);
                if (item->mode != 0)
                {
                    AdtMessageBox(D_800121CC, item->type,
                                  (u32)item->mode);
                }
                item->owner = 0;
                item->proc = 0;
            }

            saved = &scratch.drop.saved;
            launch = &scratch.drop.launch;
            scratch.drop.launch.type = saved->type;
            launch->user = (Humanoid *)1;
            scratch.drop.launch.start.vx = saved->locate.vx;
            scratch.drop.launch.start.vy = saved->locate.vy;
            scratch.drop.launch.start.vz = saved->locate.vz;
            scratch.drop.launch.end.vx = 0;
            scratch.drop.launch.end.vy = 0;
            scratch.drop.launch.end.vz = 0;
            scratch.drop.launch.start.vy = GetAreaMapLevel(
                GlobalAreaMap, scratch.drop.launch.start.vx,
                scratch.drop.launch.start.vy,
                scratch.drop.launch.start.vz, 0);
            ReqItemDrop(launch);
            return;
        }
        else
        {
            u16 count;

            count = param->count - 1;
            param->count = count;
            if ((count << 16) <= 0)
            {
                item->mode = item->mode + 1;
            }
            UpdateCoordinate(item->locate);
            item->model->locate = item->locate->locate;
            DrawSprite(item->model);
            return;
        }
    }

    case 1:
    {
        s32 create;
        s32 valid;
        Humanoid *slave;
        VECTOR *position;
        VECTOR *query;
        MapVector *map;

        create = 0;
        if (is_character_state_present_on_stage_(NINKEN_CHARACTER_PTR) == 0 ||
            GetHumanoid(0xa9) == 0)
        {
            create = 1;
        }
        if (create != 0)
        {
            NINKEN_CHARACTER_PTR = BreedLife(0xa9, 999000, 999000,
                                             999000, 0);
            NINKEN_CHARACTER_PTR->attribute |= 0x80;
        }

        position = &scratch.spawn.pos;
        query = &scratch.spawn.query;
        map = &scratch.spawn.map;
        scratch.spawn.pos.vx = item->locate->locate.coord.t[0];
        scratch.spawn.pos.vy = item->locate->locate.coord.t[1];
        scratch.spawn.pos.vz = item->locate->locate.coord.t[2];
        scratch.spawn.query.vx = position->vx;
        scratch.spawn.query.vy = position->vy;
        scratch.spawn.query.vz = position->vz;
        scratch.spawn.query.vy -= 2000;
        GetAreaMapVector(GlobalAreaMap, map, query, 500, 0);

        if (scratch.spawn.map.level >= position->vy - 500)
        {
            if (scratch.spawn.map.level < position->vy)
            {
                position->vy = scratch.spawn.map.level;
            }
            valid = 1;
        }
        else
        {
            valid = 0;
        }
        if (valid == 0 ||
            (NINKEN_CHARACTER_PTR->attribute & 0x80) == 0)
        {
            item->mode = item->mode - 1;
            param->count = 15;
            return;
        }

        *(SVECTOR *)&scratch.spawn.query = D_80097AF4[0];
        SetSmoke(&scratch.spawn.pos, (SVECTOR *)&scratch.spawn.query, 10, 6);
        SoundEx(&scratch.spawn.pos, 0x23);
        param->slave = NINKEN_CHARACTER_PTR;
        NINKEN_CHARACTER_PTR->status = 0;
        slave = param->slave;
        slave->life = slave->lifemax;
        param->slave->model->locate.coord.t[0] = scratch.spawn.pos.vx;
        param->slave->model->locate.coord.t[1] = scratch.spawn.pos.vy;
        param->slave->model->locate.coord.t[2] = scratch.spawn.pos.vz;
        param->slave->model->rotate.vx = item->owner->model->rotate.vx;
        param->slave->model->rotate.vy = item->owner->model->rotate.vy;
        param->slave->model->rotate.vz = item->owner->model->rotate.vz;
        EquipWeapon(param->slave, 0);
        SetNowMotion(param->slave, 0x80f, 1);
        param->slave->attribute &= 0xfffc;
        param->slave->attribute = 0;
        param->slave->target = (ModelType *)item->owner->model;
        param->slave->motion->count = 0;
        PlayMotion(param->slave->motion, 1);
        param->slave->attribute &= 0xff7f;
        param->slave->model->object[0]->attribute |= 0x4000;
        FUN_800270f8(param->slave, 0);
        param->slave->vector.vy = 0;
        item->mode = item->mode + 1;
        param->count = 0x708;
        return;
    }

    case 2:
    {
        u16 count;
        Humanoid *slave;

        if (is_character_state_present_on_stage_(param->slave) == 0)
        {
            if (item->proc != 0)
            {
                item->mode = ff;
                item->proc(item);
                DeleteConflict(item->locate);
                if (item->mode != 0)
                {
                    AdtMessageBox(D_800121CC, item->type,
                                  (u32)item->mode);
                }
                item->owner = 0;
                item->proc = 0;
            }
        }

        count = param->count - 1;
        param->count = count;
        if ((count << 16) <= 0)
        {
            goto expire;
        }
        slave = param->slave;
        if (slave->life <= 0)
        {
            goto expire;
        }
        if ((slave->attribute & 0x80) == 0)
        {
            goto active;
        }

expire:
        scratch.effect_vec = D_80097AF4[0];
        SetSmoke((VECTOR *)param->slave->model->locate.coord.t,
                 &scratch.effect_vec, 10, 6);
        SoundEx((VECTOR *)param->slave->model->locate.coord.t, 0x23);
        TurnAroundAllItems(param->slave);
        {
            void (*dispose_proc)(tag_TItem *);

            dispose_proc = item->proc;
            if (dispose_proc == 0)
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

active:
        {
            s32 owner_attribute;
            Humanoid *target;
            s16 status;

            if (GameClock % 15 != 0)
            {
                return;
            }
            status = slave->status;
            if (status == 0x10 || status == 8 || status == 7)
            {
                return;
            }

            owner_attribute = item->owner->attribute;
            item->owner->attribute = 0x80;
            target = GetNearestHumanoid(param->slave, 10000);
            item->owner->attribute = owner_attribute;
            if (target != 0)
            {
                if ((ModelType *)target->model == param->slave->target)
                {
                    return;
                }
                SetupThinkFunction(param->slave, 0x5449);
                param->slave->target = (ModelType *)target->model;
                param->slave->attribute |= 2;
                EquipWeapon(param->slave, 1);
                SetNowMotion(param->slave, 0x80e, 1);
                return;
            }
            else
            {
                if (param->slave->target == item->locate)
                {
                    return;
                }
                EquipWeapon(param->slave, 0);
                SetNowMotion(param->slave, 0x80f, 1);
                param->slave->attribute &= 0xfffc;
                SetupThinkFunction(param->slave, 0);
                param->slave->target = (ModelType *)item->owner->model;
                return;
            }
        }
    }
    }
    return;
}

// triage: HARD — 575 insns, mul/div, 1 loop, indirect-call, 22 callees, ~0.13 to ProcItemDrop
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Type propagation algorithm not settling */
//
// void ProcItemNinken(tag_TItem *item)
//
// {
//   byte bVar1;
//   char cVar2;
//   short sVar3;
//   bool bVar4;
//   ushort uVar5;
//   undefined **ppuVar6;
//   ModelType *pMVar7;
//   int iVar8;
//   ModelType *pMVar9;
//   Humanoid *pHVar10;
//   SVECTOR *pSVar11;
//   Humanoid *human;
//   undefined4 uVar12;
//   undefined4 uVar13;
//   undefined4 uVar14;
//   VECTOR local_70;
//   SVECTOR local_60;
//   TItemType local_58;
//   long local_54;
//   VECTOR local_50;
//   PARAM_ITEM_USE local_40;
//
//   if (item->mode == 0xff) {
//     pHVar10 = (item->param).ninken.slave;
//     if (pHVar10 != (Humanoid *)0x0) {
//       NowReturnNormal(pHVar10);
//       pHVar10 = (item->param).ninken.slave;
//       pHVar10->attribute = pHVar10->attribute | 0x80;
//       (((item->param).ninken.slave)->model->locate).coord.t[0] = 0xf3e58;
//       (((item->param).ninken.slave)->model->locate).coord.t[1] = 0xf3e58;
//       (((item->param).ninken.slave)->model->locate).coord.t[2] = 0xf3e58;
//       UpdateCoordinate((ModelType *)((item->param).ninken.slave)->model);
//     }
//     item->mode = '\0';
//   }
//   else {
//     bVar1 = item->mode;
//     if (bVar1 == 1) {
//       bVar4 = false;
//       iVar8 = FUN_800294dc(DAT_80097f58);
//       if ((iVar8 == 0) || (pHVar10 = GetHumanoid(0xa9), pHVar10 == (Humanoid *)0x0)) {
//         bVar4 = true;
//       }
//       if (bVar4) {
//         DAT_80097f58 = (Humanoid *)BreedLife(0xa9,0xf3e58,0xf3e58,0xf3e58,0);
//         DAT_80097f58->attribute = DAT_80097f58->attribute | 0x80;
//       }
//       local_70.vx = (item->locate->locate).coord.t[0];
//       local_70.vy = (item->locate->locate).coord.t[1];
//       local_70.vz = (item->locate->locate).coord.t[2];
//       local_60._4_4_ = local_70.vy + -2000;
//       local_60._0_4_ = local_70.vx;
//       local_58 = local_70.vz;
//       GetAreaMapVector((MapVector *)GlobalAreaMap,&local_50,(long)&local_60);
//       bVar4 = false;
//       if ((local_70.vy + -500 <= local_50.vx) && (bVar4 = true, local_50.vx < local_70.vy)) {
//         local_70.vy = local_50.vx;
//       }
//       if ((bVar4) && ((DAT_80097f58->attribute & 0x80U) != 0)) {
//         local_60.vx = (undefined2)DAT_80097af4;
//         local_60.vy = DAT_80097af4._2_2_;
//         local_60.vz = (undefined2)DAT_80097af8;
//         local_60.pad = DAT_80097af8._2_2_;
//         SetSmoke(&local_70,&local_60,10,6);
//         SoundEx(&local_70,0x23);
//         pHVar10 = DAT_80097f58;
//         (item->param).ninken.slave = DAT_80097f58;
//         pHVar10->status = 0;
//         pHVar10 = (item->param).ninken.slave;
//         pHVar10->life = pHVar10->lifemax;
//         (((item->param).ninken.slave)->model->locate).coord.t[0] = local_70.vx;
//         (((item->param).ninken.slave)->model->locate).coord.t[1] = local_70.vy;
//         (((item->param).ninken.slave)->model->locate).coord.t[2] = local_70.vz;
//         (((item->param).ninken.slave)->model->rotate).vx = (item->owner->model->rotate).vx;
//         (((item->param).ninken.slave)->model->rotate).vy = (item->owner->model->rotate).vy;
//         (((item->param).ninken.slave)->model->rotate).vz = (item->owner->model->rotate).vz;
//         EquipWeapon((item->param).ninken.slave,0);
//         SetNowMotion((item->param).ninken.slave,0x80f,1);
//         pHVar10 = (item->param).ninken.slave;
//         pHVar10->attribute = pHVar10->attribute & 0xfffc;
//         ((item->param).ninken.slave)->attribute = 0;
//         ((item->param).ninken.slave)->target = (ModelType *)item->owner->model;
//         ((item->param).ninken.slave)->motion->count = 0;
//         PlayMotion(((item->param).ninken.slave)->motion,1);
//         pHVar10 = (item->param).ninken.slave;
//         pHVar10->attribute = pHVar10->attribute & 0xff7f;
//         pMVar7 = *((item->param).ninken.slave)->model->object;
//         pMVar7->attribute = pMVar7->attribute | 0x4000;
//         FUN_800270f8((item->param).gun.vec.pad,0);
//         (((item->param).ninken.slave)->vector).vy = 0;
//         item->mode = item->mode + '\x01';
//         (item->param).ninken.count = 0x708;
//       }
//       else {
//         item->mode = item->mode + 0xff;
//         (item->param).ninken.count = 0xf;
//       }
//     }
//     else {
//       if (bVar1 < 2) {
//         if (bVar1 != 0) {
//           return;
//         }
//         MoveKorogari(item,(param_korogari *)&(item->param).launch);
//         cVar2 = *(char *)((int)&item->param + 10);
//         if (cVar2 != '\x01') {
//           if (cVar2 == '\x04') {
//             memset((uchar *)&local_58,'\0',0x14);
//             local_70.vx = item->type;
//             local_70.vy = (item->locate->locate).coord.t[0];
//             local_70.vz = (item->locate->locate).coord.t[1];
//             local_70.pad = (item->locate->locate).coord.t[2];
//             local_60.vx = (undefined2)local_50.vz;
//             local_60.vy = local_50.vz._2_2_;
//             local_58 = local_70.vx;
//             local_54 = local_70.vy;
//             local_50.vx = local_70.vz;
//             local_50.vy = local_70.pad;
//             if (item->proc != (undefined **)0x0) {
//               item->mode = 0xff;
//               (*(code *)item->proc)(item);
//               DeleteConflict(item->locate);
//               if (item->mode != 0) {
//                 AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//               }
//               item->owner = (Humanoid *)0x0;
//               item->proc = (undefined **)0x0;
//             }
//             local_40.type = local_70.vx;
//             local_40.user = (Humanoid *)&DAT_00000001;
//             local_40.start.vx = local_70.vy;
//             local_40.start.vy = local_70.vz;
//             local_40.end.vx = 0;
//             local_40.end.vy = 0;
//             local_40.end.vz = 0;
//             local_40.start.vz = local_70.pad;
//             local_40.start.vy = GetAreaMapLevel(GlobalAreaMap,local_70.vy,local_70.vz);
//             ReqItemDrop(&local_40);
//             return;
//           }
//           uVar5 = (item->param).ninken.count - 1;
//           (item->param).ninken.count = uVar5;
//           if ((int)((uint)uVar5 << 0x10) < 1) {
//             item->mode = item->mode + '\x01';
//           }
//           UpdateCoordinate(item->locate);
//           pMVar7 = item->locate;
//           pMVar9 = item->model;
//           pSVar11 = &pMVar7->rotate;
//           do {
//             uVar12 = *(undefined4 *)(pMVar7->locate).coord.m[0];
//             uVar13 = *(undefined4 *)((pMVar7->locate).coord.m[0] + 2);
//             uVar14 = *(undefined4 *)((pMVar7->locate).coord.m[1] + 1);
//             (pMVar9->locate).flg = (pMVar7->locate).flg;
//             *(undefined4 *)(pMVar9->locate).coord.m[0] = uVar12;
//             *(undefined4 *)((pMVar9->locate).coord.m[0] + 2) = uVar13;
//             *(undefined4 *)((pMVar9->locate).coord.m[1] + 1) = uVar14;
//             pMVar7 = (ModelType *)((pMVar7->locate).coord.m + 2);
//             pMVar9 = (ModelType *)((pMVar9->locate).coord.m + 2);
//           } while (pMVar7 != (ModelType *)pSVar11);
//           DrawSprite((Sprite3D *)item->model);
//           return;
//         }
//         ppuVar6 = item->proc;
//         if (ppuVar6 == (undefined **)0x0) {
//           return;
//         }
//         item->mode = 0xff;
//       }
//       else {
//         if (bVar1 != 2) {
//           return;
//         }
//         iVar8 = FUN_800294dc((item->param).gun.vec.pad);
//         if ((iVar8 == 0) && (item->proc != (undefined **)0x0)) {
//           item->mode = 0xff;
//           (*(code *)item->proc)(item);
//           DeleteConflict(item->locate);
//           if (item->mode != 0) {
//             AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//           }
//           item->owner = (Humanoid *)0x0;
//           item->proc = (undefined **)0x0;
//         }
//         uVar5 = (item->param).ninken.count - 1;
//         (item->param).ninken.count = uVar5;
//         if (((0 < (int)((uint)uVar5 << 0x10)) &&
//             (pHVar10 = (item->param).ninken.slave, 0 < pHVar10->life)) &&
//            ((pHVar10->attribute & 0x80U) == 0)) {
//           if (GameClock != (GameClock / 0xf) * 0xf) {
//             return;
//           }
//           sVar3 = pHVar10->status;
//           if (sVar3 == 0x10) {
//             return;
//           }
//           if (sVar3 == 8) {
//             return;
//           }
//           if (sVar3 == 7) {
//             return;
//           }
//           sVar3 = item->owner->attribute;
//           item->owner->attribute = 0x80;
//           pHVar10 = GetNearestHumanoid((item->param).ninken.slave,10000);
//           item->owner->attribute = sVar3;
//           if (pHVar10 == (Humanoid *)0x0) {
//             pHVar10 = (item->param).ninken.slave;
//             if (pHVar10->target == item->locate) {
//               return;
//             }
//             EquipWeapon(pHVar10,0);
//             SetNowMotion((item->param).ninken.slave,0x80f,1);
//             pHVar10 = (item->param).ninken.slave;
//             pHVar10->attribute = pHVar10->attribute & 0xfffc;
//             SetupThinkFunction((item->param).ninken.slave,0);
//             ((item->param).ninken.slave)->target = (ModelType *)item->owner->model;
//             return;
//           }
//           human = (item->param).ninken.slave;
//           if ((ModelType *)pHVar10->model == human->target) {
//             return;
//           }
//           SetupThinkFunction(human,0x5449);
//           ((item->param).ninken.slave)->target = (ModelType *)pHVar10->model;
//           pHVar10 = (item->param).ninken.slave;
//           pHVar10->attribute = pHVar10->attribute | 2;
//           EquipWeapon((item->param).ninken.slave,1);
//           SetNowMotion((item->param).ninken.slave,0x80e,1);
//           return;
//         }
//         local_70.vx = DAT_80097af4;
//         local_70.vy = DAT_80097af8;
//         SetSmoke((VECTOR *)(((item->param).ninken.slave)->model->locate).coord.t,
//                  (SVECTOR *)&local_70,10,6);
//         SoundEx((VECTOR *)(((item->param).ninken.slave)->model->locate).coord.t,0x23);
//         TurnAroundAllItems((item->param).gun.vec.pad);
//         ppuVar6 = item->proc;
//         if (ppuVar6 == (undefined **)0x0) {
//           return;
//         }
//         item->mode = 0xff;
//       }
//       (*(code *)ppuVar6)(item);
//       DeleteConflict(item->locate);
//       if (item->mode != 0) {
//         AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//       }
//       item->owner = (Humanoid *)0x0;
//       item->proc = (undefined **)0x0;
//     }
//   }
//   return;
// }
