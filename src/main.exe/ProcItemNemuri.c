#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"

extern SVECTOR D_80097B04[];

extern s16 GetConflictResult(ModelType *model, s32 n);
extern s32 is_character_state_present_on_stage_(Humanoid *human);
extern s16 Think1sleep(void);

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemNemuri(struct tag_TItem *item);
 *     ITEM.C:2738, 93 src lines, frame 112 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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
 *     reg   $s4       struct Sprite3D * model
 *     reg   $s3       struct param_napalm * param
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       struct VECTOR * pos
 *     reg   $s2       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $s1       int env
 *     reg   $v0       int bright
 *     reg   $a0       int cid
 *     reg   $a0       struct ModelType * model
 *     reg   $s3       struct Humanoid * human
 *     reg   $s3       struct Humanoid * human
 *     stack sp+16     struct VECTOR pos
 *     stack sp+32     struct SVECTOR vec
 *     stack sp+48     struct VECTOR pos
 *     reg   $s3       struct Humanoid * human
 *     reg   $s2       struct tag_TItem * item
 *     reg   $s2       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

/*
 * Advances the sleeping-powder projectile, draws its pulsing sprite, puts a
 * collided humanoid to sleep, and disposes the item after impact, expiry, or
 * leaving the area map.
 *
 * Matching notes:
 *  - `eight` shares the collision-mode constant between a halfword and a word
 *    store; two literals produce an extra `li`.
 *  - The byte-identical `bleed_n` arms disappear in jump2, but their CFG keeps
 *    the call-count and colour pseudos out of the wave's register.  `bleed_n`
 *    is initialized, and is overwritten with its real value before the call.
 *  - The nested zero-trip loops emit no instructions.  Their loop notes weight
 *    `env` to 9 refs / 49 RTL insns (priority 5510), above `wave`'s 5 / 35
 *    (2857), selecting the target $t0/$t1 allocation.  Splitting the colour
 *    across the two statements then puts its `lui` in the branch delay slot
 *    and its `ori` at the join.
 *  - `bleed_range` is assigned after the duplicated X update so its $a1 copy
 *    fills that update's load delay.  The full-width `rotate_count` similarly
 *    schedules the count load before the scale store without an `andi 0xff`.
 *  - The cleanup paths cache `proc` for the null check but call through
 *    `item->proc`; jump2 cross-jumps them into the target shared tail.
 */
void ProcItemNemuri(TItem *item)
{
    enum
    {
        MaxCount = 100
    };
    Sprite3D *sprt;
    param_napalm *param;
    void (*proc)(TItem *);
    u8 ff;
    u8 count;
    s32 rotate_count;

    sprt = (Sprite3D *)item->model;
    param = &item->param.napalm;
    ff = ITEM_MODE_DISPOSE;
    if (item->mode == ff)
    {
        item->mode = 0;
        return;
    }

    switch (item->mode)
    {
    case 0:
        SetNowMotion(item->owner, 0xf02, 1);
        SoundEx((VECTOR *)item->owner->model->locate.coord.t, 0x26);
        item->mode = item->mode + 1;
        return;

    case 1:
        if (item->owner->motion->mid == 0xf02)
        {
            if (item->owner->motion->count != 3)
            {
                return;
            }
            {
                VECTOR *position;
                s32 n;
                s32 eight;

                position = GetAbsolutePosition(
                    item->owner->model->object[14], 0, 0, 0);
                param->count = 0;
                item->mode = item->mode + 1;
                item->locate->locate.coord.t[0] = position->vx;
                item->locate->locate.coord.t[1] = position->vy;
                item->locate->locate.coord.t[2] = position->vz;
                DeleteConflict(item->locate);
                n = InsertConflict(item->locate);
                eight = 8;
                ConflictObject[n].offset.vx = 0;
                ConflictObject[n].offset.vz = 0;
                ConflictObject[n].offset.vy = 0;
                ConflictObject[n].size.vz = 1000;
                ConflictObject[n].size.vy = 1000;
                ConflictObject[n].size.vx = 1000;
                ConflictObject[n].common = (void *)1;
                ConflictObject[n].size.pad = eight;
                item->collision.size = 1000;
                item->collision.ofsY = 0;
                item->collision.mode = eight;
                item->collision.pause = 0;
                return;
            }
        }
        proc = item->proc;
        if (proc == 0)
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

    case 2:
    {
        s32 wave;
        s32 bright;
        s32 cid;
        s32 dead;
        s32 env;
        s32 bleed_n = 0;
        s32 bleed_range;
        Humanoid *human;

        wave = rsin(param->count * 0x88);
        if (wave < 0)
        {
            wave += 0x3f;
        }
        if (bleed_n != 0)
        {
            env = 0x6e0000;
                        env |= 0x6e6e;
            item->locate->locate.coord.t[0] +=
                item->param.napalm.vec.vx;
            bleed_range = 300;
        }
        else
        {
            env = 0x6e0000;
                        env |= 0x6e6e;
            item->locate->locate.coord.t[0] +=
                item->param.napalm.vec.vx;
            bleed_range = 300;
        }
        bleed_n = 2;
        item->locate->locate.coord.t[1] += param->vec.vy;
        item->locate->locate.coord.t[2] += param->vec.vz;
        bright = (wave >> 6) + 0x80;
        sprt->sprite.r = bright;
        sprt->sprite.g = bright;
        sprt->sprite.b = bright;
        rotate_count = param->count;
        sprt->scale = bright * 2 + 0x4000;
        sprt->sprite.rotate = rotate_count * 0x2d000;
        SetBleeds((VECTOR *)item->locate->locate.coord.t,
                  bleed_range, 10, bleed_n, 10, env);

        count = param->count + 1;
        param->count = count;
        if (count > MaxCount)
        {
            item->mode = item->mode + 1;
        }

        if ((item->locate->attribute & 0x8000) == 0)
        {
            cid = -1;
        }
        else
        {
            cid = GetConflictResult(item->locate, -1);
        }
        dead = -1;
        if (cid != dead)
        {
            human = (Humanoid *)ConflictObject[cid].common;
            if (is_character_state_present_on_stage_(human) != 0 &&
                human != item->owner)
            {
                VECTOR random_pos;
                VECTOR random_buf;
                VECTOR smoke_pos;
                VECTOR human_buf;
                SVECTOR *vec;
                s16 life;

                if (human->model->n > 0)
                {
                    rand();
                }
                vec = (SVECTOR *)&random_buf;
                memset(&random_buf, 0, sizeof(VECTOR));
                random_buf.vx = rand() % 200 - 100;
                random_buf.vy = rand() % 200 - 100;
                random_buf.vz = rand() % 200 - 100;
                random_pos = random_buf;
                SoundEx((VECTOR *)item->locate->locate.coord.t, 0x23);

                *vec = D_80097B04[0];
                memset(&human_buf, 0, sizeof(VECTOR));
                human_buf.vx = human->model->locate.coord.t[0];
                human_buf.vy = human->model->locate.coord.t[1];
                human_buf.vz = human->model->locate.coord.t[2];
                smoke_pos = human_buf;
                SetSmoke(&smoke_pos, vec, 10, 0x1e);

                life = human->life;
                if (life > 0 && human->motion->mid != 0x100)
                {
                    if ((human->type & 0xf0) != 0x80 && life != dead)
                    {
                        EquipWeapon(human, 0);
                        SetNowMotion(human, 0x80f, 1);
                        human->think[0] = Think1sleep;
                        human->attribute &= 0xfffc;
                    }
                    SetNowMotion(human, 0x100, 1);
                    Sound(human, 6);
                }

                proc = item->proc;
                if (proc == 0)
                {
                    return;
                }
                item->mode = ITEM_MODE_DISPOSE;
                item->proc(item);
                DeleteConflict(item->locate);
                if (item->mode != 0)
                {
                    AdtMessageBox(D_800121CC, item->type,
                                  (u32)item->mode);
                }
                item->owner = 0;
                item->proc = 0;
                return;
            }
        }

        if (GetAreaMapLevel(GlobalAreaMap,
                            item->locate->locate.coord.t[0],
                            item->locate->locate.coord.t[1],
                            item->locate->locate.coord.t[2], 0) ==
            (long)0x80000000)
        {
            proc = item->proc;
            if (proc == 0)
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
        break;
    }

    case 3:
        proc = item->proc;
        if (proc == 0)
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

    UpdateCoordinate(item->locate);
    sprt->locate = item->locate->locate;
    DrawSprite(sprt);
}

// triage: HARD — 422 insns, mul/div, 1 loop, indirect-call, 18 callees, ~0.26 to ProcItemDrop
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation retained as reconstruction reference:
//
//
// /* WARNING: Type propagation algorithm not settling */
//
// void ProcItemNemuri(tag_TItem *item)
//
// {
//   uchar uVar1;
//   byte bVar2;
//   short sVar3;
//   VECTOR *pVVar4;
//   int iVar5;
//   long lVar6;
//   undefined **ppuVar7;
//   ModelType *pMVar8;
//   Sprite3D *pSVar9;
//   MotionManager *pMVar10;
//   SVECTOR *pSVar11;
//   undefined4 uVar12;
//   undefined4 uVar13;
//   undefined4 uVar14;
//   Humanoid *human;
//   Sprite3D *sprt;
//   SVECTOR local_50;
//   int local_48;
//   VECTOR local_40;
//   long local_30;
//   long local_2c;
//   long local_28;
//   long local_24;
//
//   sprt = (Sprite3D *)item->model;
//   if (item->mode == 0xff) {
//     item->mode = '\0';
//     return;
//   }
//   bVar2 = item->mode;
//   if (bVar2 == 1) {
//     pMVar10 = item->owner->motion;
//     if (pMVar10->mid == 0xf02) {
//       if (pMVar10->count != 3) {
//         return;
//       }
//       pVVar4 = GetAbsolutePosition(item->owner->model->object[0xe],0,0,0);
//       (item->param).napalm.count = '\0';
//       item->mode = item->mode + '\x01';
//       (item->locate->locate).coord.t[0] = pVVar4->vx;
//       (item->locate->locate).coord.t[1] = pVVar4->vy;
//       (item->locate->locate).coord.t[2] = pVVar4->vz;
//       DeleteConflict(item->locate);
//       sVar3 = InsertConflict(item->locate);
//       ConflictObject[sVar3].offset.vx = 0;
//       ConflictObject[sVar3].offset.vz = 0;
//       ConflictObject[sVar3].offset.vy = 0;
//       ConflictObject[sVar3].size.vz = 1000;
//       ConflictObject[sVar3].size.vy = 1000;
//       ConflictObject[sVar3].size.vx = 1000;
//       ConflictObject[sVar3].common = (undefined *)0x1;
//       ConflictObject[sVar3].size.pad = 8;
//       (item->collision).size = 1000;
//       (item->collision).ofsY = 0;
//       (item->collision).mode = 8;
//       (item->collision).pause = 0;
//       return;
//     }
// LAB_80045e78:
//     ppuVar7 = item->proc;
//     if (ppuVar7 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
// LAB_80045e90:
//     (*(code *)ppuVar7)(item);
//     DeleteConflict(item->locate);
//     if (item->mode != 0) {
//       AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//     }
//     item->owner = (Humanoid *)0x0;
//     item->proc = (undefined **)0x0;
//   }
//   else {
//     if (bVar2 < 2) {
//       if (bVar2 == 0) {
//         SetNowMotion(item->owner,0xf02,1);
//         SoundEx((VECTOR *)(item->owner->model->locate).coord.t,0x26);
//         item->mode = item->mode + '\x01';
//         return;
//       }
//     }
//     else if (bVar2 == 2) {
//       iVar5 = rsin((uint)(item->param).napalm.count * 0x88);
//       if (iVar5 < 0) {
//         iVar5 = iVar5 + 0x3f;
//       }
//       (item->locate->locate).coord.t[0] =
//            (item->locate->locate).coord.t[0] + (int)(item->param).napalm.vec.vx;
//       (item->locate->locate).coord.t[1] =
//            (item->locate->locate).coord.t[1] + (int)(item->param).napalm.vec.vy;
//       (item->locate->locate).coord.t[2] =
//            (item->locate->locate).coord.t[2] + (int)(item->param).napalm.vec.vz;
//       iVar5 = (iVar5 >> 6) + 0x80;
//       uVar1 = (uchar)iVar5;
//       (sprt->sprite).r = uVar1;
//       (sprt->sprite).g = uVar1;
//       (sprt->sprite).b = uVar1;
//       bVar2 = (item->param).napalm.count;
//       sprt->scale = iVar5 * 2 + 0x4000;
//       (sprt->sprite).rotate = (uint)bVar2 * 0x2d000;
//       SetBleeds((short)item->locate + 0x18,300,10);
//       bVar2 = (item->param).napalm.count + 1;
//       (item->param).napalm.count = bVar2;
//       if (100 < bVar2) {
//         item->mode = item->mode + '\x01';
//       }
//       if (((int)item->locate->attribute & 0x8000U) == 0) {
//         iVar5 = -1;
//       }
//       else {
//         sVar3 = GetConflictResult(item->locate,-1);
//         iVar5 = (int)sVar3;
//       }
//       if (iVar5 != -1) {
//         human = (Humanoid *)ConflictObject[iVar5].common;
//         iVar5 = FUN_800294dc(human);
//         if ((iVar5 != 0) && (human != item->owner)) {
//           if (0 < human->model->n) {
//             rand();
//           }
//           memset((uchar *)&local_50,'\0',0x10);
//           iVar5 = rand();
//           local_50._0_4_ = iVar5 % 200 + -100;
//           iVar5 = rand();
//           local_50._4_4_ = iVar5 % 200 + -100;
//           iVar5 = rand();
//           local_48 = iVar5 % 200 + -100;
//           SoundEx((VECTOR *)(item->locate->locate).coord.t,0x23);
//           local_50.vx = (short)DAT_80097b04;
//           local_50.vy = DAT_80097b04._2_2_;
//           local_50.vz = (short)DAT_80097b08;
//           local_50.pad = DAT_80097b08._2_2_;
//           memset((uchar *)&local_30,'\0',0x10);
//           local_40.vx = (human->model->locate).coord.t[0];
//           local_40.vy = (human->model->locate).coord.t[1];
//           local_40.vz = (human->model->locate).coord.t[2];
//           local_40.pad = local_24;
//           local_30 = local_40.vx;
//           local_2c = local_40.vy;
//           local_28 = local_40.vz;
//           SetSmoke(&local_40,&local_50,10,0x1e);
//           if ((0 < human->life) && (human->motion->mid != 0x100)) {
//             if (((human->type & 0xf0U) != 0x80) && (human->life != -1)) {
//               EquipWeapon(human,0);
//               SetNowMotion(human,0x80f,1);
//               human->think[0] = (undefined **)Think1sleep;
//               human->attribute = human->attribute & 0xfffc;
//             }
//             SetNowMotion(human,0x100,1);
//             Sound(human,6);
//           }
//           ppuVar7 = item->proc;
//           if (ppuVar7 == (undefined **)0x0) {
//             return;
//           }
//           item->mode = 0xff;
//           goto LAB_80045e90;
//         }
//       }
//       lVar6 = GetAreaMapLevel(GlobalAreaMap,(item->locate->locate).coord.t[0],
//                               (item->locate->locate).coord.t[1]);
//       if (lVar6 == -0x80000000) {
//         ppuVar7 = item->proc;
//         if (ppuVar7 == (undefined **)0x0) {
//           return;
//         }
//         item->mode = 0xff;
//         goto LAB_80045e90;
//       }
//     }
//     else if (bVar2 == 3) goto LAB_80045e78;
//     UpdateCoordinate(item->locate);
//     pMVar8 = item->locate;
//     pSVar11 = &pMVar8->rotate;
//     pSVar9 = sprt;
//     do {
//       uVar13 = *(undefined4 *)(pMVar8->locate).coord.m[0];
//       uVar14 = *(undefined4 *)((pMVar8->locate).coord.m[0] + 2);
//       uVar12 = *(undefined4 *)((pMVar8->locate).coord.m[1] + 1);
//       (pSVar9->locate).flg = (pMVar8->locate).flg;
//       *(undefined4 *)(pSVar9->locate).coord.m[0] = uVar13;
//       *(undefined4 *)((pSVar9->locate).coord.m[0] + 2) = uVar14;
//       *(undefined4 *)((pSVar9->locate).coord.m[1] + 1) = uVar12;
//       pMVar8 = (ModelType *)((pMVar8->locate).coord.m + 2);
//       pSVar9 = (Sprite3D *)((pSVar9->locate).coord.m + 2);
//     } while (pMVar8 != (ModelType *)pSVar11);
//     DrawSprite(sprt);
//   }
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? AdtMessageBox(? *, s32, u8);                      /* extern */
// ? DeleteConflict(void *);                           /* extern */
// ? DrawSprite(void *);                               /* extern */
// ? EquipWeapon(void *, ?);                           /* extern */
// void *GetAbsolutePosition(s32, ?, ?, ?);            /* extern */
// s32 GetAreaMapLevel(s32, s32, s32, s32, s32);       /* extern */
// s16 GetConflictResult(void *, ?);                   /* extern */
// s16 InsertConflict(void *);                         /* extern */
// ? SetBleeds(void *, ?, ?, ?, s32, s32);             /* extern */
// ? SetNowMotion(void *, ?, ?);                       /* extern */
// ? SetSmoke(s32 *, s32 *, ?, ?);                     /* extern */
// ? Sound(void *, ?);                                 /* extern */
// ? SoundEx(void *, ?, s32, s32, s32, s32);           /* extern */
// ? UpdateCoordinate(void *);                         /* extern */
// s32 is_character_state_present_on_stage_(void *);   /* extern */
// ? memset(s32 *, ?, ?);                              /* extern */
// s32 rand(s32);                                      /* extern */
// s32 rsin(s32, ?);                                   /* extern */
// extern ? ConflictObject;
// extern ? D_800121CC;
// extern ? D_80097B04;
// extern s32 GlobalAreaMap;
// extern ? Think1sleep;
//
// void ProcItemNemuri(void *arg0) {
//     s32 sp28;
//     s32 sp2C;
//     s32 sp30;
//     s32 sp38;
//     s32 sp3C;
//     s32 sp40;
//     s32 sp44;
//     s32 sp48;
//     s32 sp4C;
//     s32 sp50;
//     s16 temp_a0_6;
//     s16 var_a0;
//     s32 temp_ret;
//     s32 temp_ret_2;
//     s32 var_t1;
//     s8 temp_v0_3;
//     u8 temp_s1;
//     u8 temp_v0_2;
//     u8 temp_v0_4;
//     void *temp_a0;
//     void *temp_a0_2;
//     void *temp_a0_3;
//     void *temp_a0_4;
//     void *temp_a0_5;
//     void *temp_a0_7;
//     void *temp_a1;
//     void *temp_s0;
//     void *temp_s3;
//     void *temp_s4;
//     void *temp_v0;
//     void *temp_v0_5;
//     void *temp_v1;
//     void *var_v0;
//     void *var_v1;
//
//     temp_s4 = arg0->unk4;
//     temp_s0 = arg0 + 0x20;
//     if (arg0->unk54 == 0xFF) {
//         arg0->unk54 = 0U;
//         return;
//     }
//     temp_s1 = arg0->unk54;
//     switch (temp_s1) {                              /* irregular */
//     case 0:
//         SetNowMotion(arg0->unk0, 0xF02, 1);
//         SoundEx(arg0->unk0->unk58 + 0x18, 0x26);
//         arg0->unk54 = (u8) (arg0->unk54 + 1);
//         return;
//     case 1:
//         temp_a1 = arg0->unk0;
//         temp_a0 = temp_a1->unk5C;
//         if (temp_a0->unk0 == 0xF02) {
//             if (temp_a0->unk2 == 3) {
//                 temp_v0 = GetAbsolutePosition(temp_a1->unk58->unk68->unk38, 0, 0, 0);
//                 temp_s0->unk8 = 0U;
//                 arg0->unk54 = (u8) (arg0->unk54 + 1);
//                 arg0->unk10->unk18 = (s32) temp_v0->unk0;
//                 arg0->unk10->unk1C = (s32) temp_v0->unk4;
//                 arg0->unk10->unk20 = (s32) temp_v0->unk8;
//                 DeleteConflict(arg0->unk10);
//                 temp_v1 = (InsertConflict(arg0->unk10) * 0x78) + &ConflictObject;
//                 temp_v1->unk14 = 0;
//                 temp_v1->unk18 = 0;
//                 temp_v1->unk16 = 0;
//                 temp_v1->unk20 = 0x3E8;
//                 temp_v1->unk1E = 0x3E8;
//                 temp_v1->unk1C = 0x3E8;
//                 temp_v1->unk24 = (s32) temp_s1;
//                 temp_v1->unk22 = 8;
//                 arg0->unk1C = 0x3E8;
//                 arg0->unk1E = 0;
//                 arg0->unk14 = 8;
//                 arg0->unk18 = 0;
//                 return;
//             }
//         } else {
//         case 3:
//             if (arg0->unkC != NULL) {
//                 arg0->unk54 = 0xFFU;
// block_38:
//                 arg0->unkC(arg0);
//                 DeleteConflict(arg0->unk10);
//                 temp_v0_2 = arg0->unk54;
//                 if (temp_v0_2 != 0) {
//                     AdtMessageBox(&D_800121CC, arg0->unk8, temp_v0_2);
//                 }
//                 arg0->unk0 = NULL;
//                 arg0->unkC = NULL;
//                 return;
//             }
//             return;
//         }
//         break;
//     case 2:
//         var_t1 = rsin(temp_s0->unk8 * 0x88, 0xFF);
//         if (var_t1 < 0) {
//             var_t1 += 0x3F;
//         }
//         temp_a0_2 = arg0->unk10;
//         temp_a0_2->unk18 = (s32) (temp_a0_2->unk18 + arg0->unk20);
//         temp_a0_3 = arg0->unk10;
//         temp_a0_3->unk1C = (s32) (temp_a0_3->unk1C + temp_s0->unk2);
//         temp_a0_4 = arg0->unk10;
//         temp_a0_4->unk20 = (s32) (temp_a0_4->unk20 + temp_s0->unk4);
//         temp_v0_3 = (var_t1 >> 6) + 0x80;
//         temp_s4->unk7C = temp_v0_3;
//         temp_s4->unk7D = temp_v0_3;
//         temp_s4->unk7E = temp_v0_3;
//         temp_s4->unk64 = (s32) ((temp_v0_3 * 2) + 0x4000);
//         temp_s4->unk88 = (s32) (temp_s0->unk8 * 0x2D000);
//         SetBleeds(arg0->unk10 + 0x18, 0x12C, 0xA, 2, 0xA, 0x6E6E6E);
//         temp_v0_4 = temp_s0->unk8 + 1;
//         temp_s0->unk8 = temp_v0_4;
//         if ((u32) (temp_v0_4 & 0xFF) >= 0x65U) {
//             arg0->unk54 = (u8) (arg0->unk54 + 1);
//         }
//         temp_a0_5 = arg0->unk10;
//         if (!(temp_a0_5->unk5A & 0x8000)) {
//             var_a0 = -1;
//         } else {
//             var_a0 = GetConflictResult(temp_a0_5, -1);
//         }
//         if ((var_a0 != -1) && (temp_s3 = ((var_a0 * 0x78) + &ConflictObject)->unk24, (is_character_state_present_on_stage_(temp_s3) != 0)) && (temp_s3 != arg0->unk0)) {
//             if (temp_s3->unk58->unk64 > 0) {
//                 rand();
//             }
//             memset(&sp28, 0, 0x10);
//             temp_ret = rand();
//             sp28 = (temp_ret % 200) - 0x64;
//             temp_ret_2 = rand(temp_ret / 200);
//             sp2C = (temp_ret_2 % 200) - 0x64;
//             sp30 = (rand(temp_ret_2 / 200) % 200) - 0x64;
//             SoundEx(arg0->unk10 + 0x18, 0x23, sp28, sp2C, sp30, sp34);
//             sp28 = (unaligned s32) D_80097B04.unk0;
//             sp2C = (unaligned s32) D_80097B04.unk4;
//             memset(&sp48, 0, 0x10);
//             sp48 = temp_s3->unk58->unk18;
//             sp4C = temp_s3->unk58->unk1C;
//             sp50 = temp_s3->unk58->unk20;
//             sp38 = sp48;
//             sp3C = sp4C;
//             sp40 = sp50;
//             sp44 = sp54;
//             SetSmoke(&sp38, &sp28, 0xA, 0x1E);
//             temp_a0_6 = temp_s3->unk8;
//             if ((temp_a0_6 > 0) && (*temp_s3->unk5C != 0x100)) {
//                 if (((temp_s3->unk0 & 0xF0) != 0x80) && (temp_a0_6 != -1)) {
//                     EquipWeapon(temp_s3, 0);
//                     SetNowMotion(temp_s3, 0x80F, 1);
//                     temp_s3->unk60 = &Think1sleep;
//                     temp_s3->unk4 = (u16) (temp_s3->unk4 & 0xFFFC);
//                 }
//                 SetNowMotion(temp_s3, 0x100, 1);
//                 Sound(temp_s3, 6);
//             }
//             if (arg0->unkC != NULL) {
//                 arg0->unk54 = 0xFFU;
//                 goto block_38;
//             }
//         } else {
//             temp_v0_5 = arg0->unk10;
//             if (GetAreaMapLevel(GlobalAreaMap, temp_v0_5->unk18, temp_v0_5->unk1C, temp_v0_5->unk20, 0) == 0x80000000) {
//                 if (arg0->unkC != NULL) {
//                     arg0->unk54 = 0xFFU;
//                     goto block_38;
//                 }
//             } else {
//             default:
//                 UpdateCoordinate(arg0->unk10);
//                 var_v0 = arg0->unk10;
//                 var_v1 = temp_s4;
//                 temp_a0_7 = var_v0 + 0x50;
//                 do {
//                     var_v1->unk0 = (s32) var_v0->unk0;
//                     var_v1->unk4 = (s32) var_v0->unk4;
//                     var_v1->unk8 = (s32) var_v0->unk8;
//                     var_v1->unkC = (s32) var_v0->unkC;
//                     var_v0 += 0x10;
//                     var_v1 += 0x10;
//                 } while (var_v0 != temp_a0_7);
//                 DrawSprite(temp_s4);
//             }
//         }
//         break;
//     }
// }
