#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemDokudango(struct tag_TItem *item);
 *     ITEM.C:1632, 141 src lines, frame 128 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
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
 *     param $s4       struct tag_TItem * item
 *     reg   $s0       struct Sprite3D * model
 *     reg   $s6       struct param_dokudango * param
 *     reg   $s4       struct tag_TItem * item
 *     stack sp+16     struct TFindItemTarget find
 *     reg   $s5       struct Humanoid * target
 *     reg   $s7       int targetlen
 *     reg   $v0       struct TFindItemTarget * find
 *     reg   $v1       struct VECTOR * pos
 *     reg   $s7       int dist
 *     reg   $s3       struct TFindItemTarget * find
 *     reg   $s1       int i
 *     reg   $s0       struct Humanoid * target
 *     reg   $v0       int dist
 *     stack sp+48     struct PARAM_ITEM_LAUNCH param
 *     reg   $s4       struct tag_TItem * item
 *     reg   $s0       struct Humanoid * human
 *     reg   $v0       struct VECTOR * tv
 *     reg   $s6       struct param_korogari * param
 *     reg   $s1       int x
 *     reg   $s0       int y
 *     reg   $v0       int z
 *     reg   $s1       struct Humanoid * human
 *     reg   $s1       struct Humanoid * human
 *     reg   $s4       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short Humans;
 *     extern short ActionHalt;
 * END PSX.SYM */

extern s32 GameClock;
extern s16 Humans;
extern Humanoid *HumanGroup[];

extern void MoveKorogari(tag_TItem *item, param_korogari *param);
extern s32 GetVectorDistance(VECTOR *a, VECTOR *b);
extern s32 is_character_state_present_on_stage_(Humanoid *human);
extern s16 NowReturnNormal(Humanoid *human);
extern s16 Think1target(void);
extern void Sound(Humanoid *human, s32 id);

/*
 * Matching notes (2,468 bytes / 617 instructions):
 *  - The entry comparison and fast disposal use literal 0xff, allowing CSE to
 *    retain that value in $s1 across MoveKorogari.  The two later cleanup
 *    copies rematerialize their own literals in $v1.
 *  - Each cleanup tests and calls item->proc directly.  Combined with the
 *    literal stores, this keeps the indirect target in $v0 and lets jump2
 *    merge the fast cleanup into the final physical copy after its mode store.
 *    A named proc local or a function-wide `ff` local changes that allocation.
 *  - The full cleanup sequence and mode-advance tail remain duplicated at
 *    their semantic exits so late cross-jumping can choose the target copies.
 */

void ProcItemDokudango(tag_TItem *item)
{
    Sprite3D *model;
    param_dokudango *param;

    model = item->model;
    param = (param_dokudango *)item->param;
    if (item->mode == 0xff)
    {
        param_dokudango *restore;

        restore = param;
        if (is_character_state_present_on_stage_(restore->eater) != 0 &&
            restore->org_think != 0)
        {
            restore->eater->think[0] = restore->org_think;
            restore->eater->target = (ModelType *)item->owner->model;
        }
        restore->eater = 0;
        item->mode = 0;
        return;
    }

    if (item->mode < 2 &&
        (MoveKorogari(item, (param_korogari *)param),
         param->status == 1))
    {
        if (item->proc == 0)
        {
            return;
        }
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
    else
    {
        if (item->mode < 3)
        {
            UpdateCoordinate(item->locate);
            model->locate = item->locate->locate;
            DrawSprite(model);
        }

        switch (item->mode)
        {
        case 0:
        {
            u16 count;

            count = param->count - 1;
            param->count = count;
            if ((s32)((u32)count << 16) > 0)
            {
                return;
            }
            item->mode = item->mode + 1;
            return;
        }

        case 1:
        {
            TFindItemTarget find;
            TFindItemTarget *q;
            TFindItemTarget *search;
            VECTOR *pos;
            Humanoid **group;
            Humanoid *target;
            Humanoid *candidate;
            Humanoid *human;
            Humanoid *found;
            s32 targetlen;
            s32 ownerlen;
            s32 i;
            s32 dist;

            if ((GameClock & 1) != 0)
            {
                return;
            }
            target = 0;
            targetlen = 10000;
            q = &find;
            pos = (VECTOR *)item->locate->locate.coord.t;
            ownerlen = targetlen;
            q->i = 0;
            q->pos.vx = pos->vx;
            search = &find;
            search->pos.vy = pos->vy;
            search->pos.vz = pos->vz;
            search->find_dist = targetlen;

            while (1)
            {
                i = search->i;
                group = HumanGroup + i;
                while (1)
                {
                    if (i < Humans)
                    {
                        do
                        {
                            candidate = *group;
                        } while (0);
                        if (candidate->life > 0 &&
                            candidate->motion->mid != 0x100 &&
                            (candidate->attribute & 0x80) == 0)
                        {
                            dist = GetVectorDistance(&search->pos,
                                                     candidate->locate);
                            if (dist < search->find_dist)
                            {
                                goto hit;
                            }
                        }
                        do
                        {
                            group++;
                        } while (0);
                        i++;
                        continue;
                    }
                    found = 0;
                    break;
                }
check:
                if (found == 0)
                {
                    break;
                }
                if ((find.find->type & 0xf0) != 0x80 &&
                    find.find->life != -1 && find.dist < targetlen)
                {
                    if (find.find != item->owner)
                    {
                        goto set_target;
                    }
                    ownerlen = find.dist;
                }
                continue;
hit:
                found = candidate;
                do
                {
                    search->find = candidate;
                    search->dist = dist;
                    search->i = i + 1;
                } while (0);
                goto check;
set_target:
                target = find.find;
                targetlen = find.dist;
                continue;
            }

            if (ownerlen < 500)
            {
                PARAM_ITEM_USE drop;
                do
                {
                    drop.type = item->type;
                    drop.user = item->owner;
                    drop.start.vx = item->locate->locate.coord.t[0];
                    drop.start.vy = item->locate->locate.coord.t[1];
                    drop.start.vz = item->locate->locate.coord.t[2];
                    drop.end.vx = 0;
                    drop.end.vy = 0;
                    drop.end.vz = 0;
                } while (0);
                if (item->proc != 0)
                {
                    item->mode = 0xff;
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
                ReqItemDrop(&drop);
                return;
            }

            if (target == 0)
            {
                return;
            }
            {
                param_dokudango *restore;

                restore = (param_dokudango *)item->param;
                if (is_character_state_present_on_stage_(restore->eater) != 0 &&
                    restore->org_think != 0)
                {
                    restore->eater->think[0] = restore->org_think;
                    restore->eater->target = (ModelType *)item->owner->model;
                }
                restore->eater = 0;
            }
            param->eater = target;
            if (target->target == (ModelType *)item->owner->model &&
                (target->attribute & 3) == 0)
            {
                param->org_think = target->think[0];
                param->eater->target = item->locate;
                param->eater->think[0] = (think_func_ *)Think1target;
            }
            else
            {
                param->org_think = 0;
            }

            if (targetlen >= 1000)
            {
                return;
            }
            human = param->eater;
            if (human->status == 0x10 || human->status == 8 ||
                human->status == 7 || human->life <= 0)
            {
                return;
            }
            if (ActionHalt == 0)
            {
                MotionDataType *motion;

                dispose_weapon_data_of_char_(human, 3);
                UpdateMotion(human->motion, 0xf01);
                human->status = 0xf;
                motion = human->motion->motion;
                MoveHumanoid(human, motion->orderspd, motion->sidespd);
            }
            if (param->eater->model->n >= 0xf)
            {
                item->locate->locate.super =
                    &param->eater->model->object[14]->locate;
                item->locate->locate.coord.t[0] = 0;
                item->locate->locate.coord.t[1] = 50;
                item->locate->locate.coord.t[2] = 0;
            }
            else
            {
                item->locate->locate.super =
                    &param->eater->model->object[2]->locate;
                item->locate->locate.coord.t[0] = 0;
                item->locate->locate.coord.t[1] = 0;
                item->locate->locate.coord.t[2] = -150;
            }
            item->mode = item->mode + 1;
            return;
        }

        case 2:
        {
            MotionManager *motion;
            Humanoid *eater;

            if (is_character_state_present_on_stage_(param->eater) == 0)
            {
                goto dispose_case3;
            }
            eater = param->eater;
            motion = eater->motion;
            if (motion->mid != 0xf01)
            {
                VECTOR *tv;
                s32 x;
                s32 y;
                s32 z;

                tv = GetAbsolutePosition(item->locate, 0, 0, 0);
                item->locate->locate.super = 0;
                item->locate->locate.coord.t[0] = tv->vx;
                item->locate->locate.coord.t[1] = tv->vy;
                item->locate->locate.coord.t[2] = tv->vz;
                x = rand();
                x = x % 200;
                y = rand();
                y = y % 100;
                z = rand();
                z = z % 200;
                param->vx = x - 100;
                param->vy = y - 200;
                param->count = 30;
                param->hint = 0;
                param->status = 0;
                param->vz = z - 100;
                item->mode = 0;
                return;
            }
            if (motion->count == 0x37)
            {
                Humanoid *human;
                param_dokudango *restore;

                human = eater;
                restore = (param_dokudango *)item->param;
                if (is_character_state_present_on_stage_(restore->eater) != 0 &&
                    restore->org_think != 0)
                {
                    restore->eater->think[0] = restore->org_think;
                    restore->eater->target = (ModelType *)item->owner->model;
                }
                restore->eater = 0;
                param->eater = human;
                NowReturnNormal(human);
                param->count = 600;
                item->mode = item->mode + 1;
                return;
            }
            if ((eater->attribute & 0x40) != 0 &&
                (eater->type & 0xf0) != 0xa0)
            {
                NowReturnNormal(eater);
            }
            return;
        }
        case 3:
        {
            Humanoid *eater;
            Humanoid *human;
            s32 count;

            if (is_character_state_present_on_stage_(param->eater) == 0)
            {
                goto dispose_case3;
            }
            count = param->count - 1;
            param->count = count;
            if ((count << 16) == 0)
            {
                goto dispose_case3;
            }
            eater = param->eater;
            if (eater->life > 0)
            {
                goto poison_active;
            }
dispose_case3:
            {
                if (item->proc == 0)
                {
                    return;
                }
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

poison_active:
            if (eater->status == 0x10 || eater->status == 8 ||
                eater->status == 7 || eater->status == 0xf)
            {
                return;
            }
            if (rand() % 30 >= 3)
            {
                return;
            }
            human = param->eater;
            if ((human->type & 0xf0) == 0xa0)
            {
                if (ActionHalt == 0 && human->life > 0)
                {
                    MotionDataType *motion;

                    dispose_weapon_data_of_char_(human, 3);
                    UpdateMotion(human->motion, 0x1000);
                    human->status = 0xf;
                    motion = human->motion->motion;
                    MoveHumanoid(human, motion->orderspd, motion->sidespd);
                }
            }
            else if (ActionHalt == 0 && human->life > 0)
            {
                MotionDataType *motion;

                dispose_weapon_data_of_char_(human, 3);
                UpdateMotion(human->motion, 0x100b);
                human->status = 0xf;
                motion = human->motion->motion;
                MoveHumanoid(human, motion->orderspd, motion->sidespd);
            }
            Sound(param->eater, 6);
            return;
        }

        default:
            return;
        }
    }
}

// triage: VERY-HARD — 617 insns, mul/div, 4 loop, indirect-call, 15 callees, ~0.20 to ProcItemDrop
// likely-relevant cookbook sections:
//   - Loops: 4 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - Register allocation steering: indirect call — null-check-var/call-field

// Ghidra decompilation (retained reference):
//
//
// /* WARNING: Type propagation algorithm not settling */
//
// void ProcItemDokudango(tag_TItem *item)
//
// {
//   byte bVar1;
//   ushort uVar2;
//   int iVar3;
//   ModelType *pMVar4;
//   int iVar5;
//   VECTOR *pVVar6;
//   int iVar7;
//   int iVar8;
//   MotionDataType *pMVar9;
//   undefined **ppuVar10;
//   Sprite3D *pSVar11;
//   ModelArchiveType *pMVar12;
//   Humanoid *pHVar13;
//   SVECTOR *pSVar14;
//   MotionManager *mmp;
//   short sVar15;
//   undefined4 uVar16;
//   undefined4 uVar17;
//   undefined4 uVar18;
//   Sprite3D *sprt;
//   Humanoid **ppHVar19;
//   Humanoid *pHVar20;
//   Humanoid *local_70;
//   int local_6c;
//   int local_68;
//   VECTOR local_64;
//   int local_54;
//   PARAM_ITEM_USE local_50;
//
//   sprt = (Sprite3D *)item->model;
//   if (item->mode == 0xff) {
//     iVar3 = FUN_800294dc((item->param).gun.vec.pad);
//     if ((iVar3 != 0) &&
//        (ppuVar10 = (undefined **)(item->param).dokudango.org_think, ppuVar10 != (undefined **)0x0))
//     {
//       ((item->param).ninken.slave)->think[0] = ppuVar10;
//       ((item->param).ninken.slave)->target = (ModelType *)item->owner->model;
//     }
//     (item->param).gun.vec.pad = 0;
//     item->mode = '\0';
//     return;
//   }
//   if ((item->mode < 2) &&
//      (MoveKorogari(item,(param_korogari *)&(item->param).launch),
//      *(char *)((int)&item->param + 10) == '\x01')) {
//     ppuVar10 = item->proc;
//     if (ppuVar10 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//   }
//   else {
//     if (item->mode < 3) {
//       UpdateCoordinate(item->locate);
//       pMVar4 = item->locate;
//       pSVar14 = &pMVar4->rotate;
//       pSVar11 = sprt;
//       do {
//         uVar16 = *(undefined4 *)(pMVar4->locate).coord.m[0];
//         uVar17 = *(undefined4 *)((pMVar4->locate).coord.m[0] + 2);
//         uVar18 = *(undefined4 *)((pMVar4->locate).coord.m[1] + 1);
//         (pSVar11->locate).flg = (pMVar4->locate).flg;
//         *(undefined4 *)(pSVar11->locate).coord.m[0] = uVar16;
//         *(undefined4 *)((pSVar11->locate).coord.m[0] + 2) = uVar17;
//         *(undefined4 *)((pSVar11->locate).coord.m[1] + 1) = uVar18;
//         pMVar4 = (ModelType *)((pMVar4->locate).coord.m + 2);
//         pSVar11 = (Sprite3D *)((pSVar11->locate).coord.m + 2);
//       } while (pMVar4 != (ModelType *)pSVar14);
//       DrawSprite(sprt);
//     }
//     bVar1 = item->mode;
//     if (bVar1 == 1) {
//       if ((GameClock & 1U) != 0) {
//         return;
//       }
//       pMVar4 = item->locate;
//       local_68 = 0;
//       local_64.vx = (pMVar4->locate).coord.t[0];
//       local_64.vy = (pMVar4->locate).coord.t[1];
//       local_64.vz = (pMVar4->locate).coord.t[2];
//       local_54 = 10000;
//       pHVar13 = (Humanoid *)0x0;
//       iVar3 = 10000;
//       iVar7 = 10000;
// LAB_8004127c:
//       iVar8 = iVar3;
//       pHVar20 = pHVar13;
//       ppHVar19 = HumanGroup + local_68;
//       for (iVar3 = local_68; pHVar13 = (Humanoid *)0x0, iVar3 < Humans; iVar3 = iVar3 + 1) {
//         pHVar13 = *ppHVar19;
//         if ((((0 < pHVar13->life) && (pHVar13->motion->mid != 0x100)) &&
//             ((pHVar13->attribute & 0x80U) == 0)) &&
//            (iVar5 = GetVectorDistance(&local_64,pHVar13->locate), iVar5 < local_54)) {
//           local_68 = iVar3 + 1;
//           local_70 = pHVar13;
//           local_6c = iVar5;
//           break;
//         }
//         ppHVar19 = ppHVar19 + 1;
//       }
//       if (pHVar13 != (Humanoid *)0x0) {
//         pHVar13 = pHVar20;
//         iVar3 = iVar8;
//         if ((((local_70->type & 0xf0U) != 0x80) && (local_70->life != -1)) &&
//            ((local_6c < iVar8 && (pHVar13 = local_70, iVar3 = local_6c, local_70 == item->owner))))
//         {
//           pHVar13 = pHVar20;
//           iVar3 = iVar8;
//           iVar7 = local_6c;
//         }
//         goto LAB_8004127c;
//       }
//       if (iVar7 < 500) {
//         local_50.type = item->type;
//         local_50.user = item->owner;
//         local_50.start.vx = (item->locate->locate).coord.t[0];
//         local_50.start.vy = (item->locate->locate).coord.t[1];
//         local_50.start.vz = (item->locate->locate).coord.t[2];
//         local_50.end.vx = 0;
//         local_50.end.vy = 0;
//         local_50.end.vz = 0;
//         if (item->proc != (undefined **)0x0) {
//           item->mode = 0xff;
//           (*(code *)item->proc)(item);
//           DeleteConflict(item->locate);
//           if (item->mode != 0) {
//             AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//           }
//           item->owner = (Humanoid *)0x0;
//           item->proc = (undefined **)0x0;
//         }
//         ReqItemDrop(&local_50);
//         return;
//       }
//       if (pHVar20 == (Humanoid *)0x0) {
//         return;
//       }
//       iVar3 = FUN_800294dc((item->param).gun.vec.pad);
//       if ((iVar3 != 0) &&
//          (ppuVar10 = (undefined **)(item->param).dokudango.org_think, ppuVar10 != (undefined **)0x0)
//          ) {
//         ((item->param).ninken.slave)->think[0] = ppuVar10;
//         ((item->param).ninken.slave)->target = (ModelType *)item->owner->model;
//       }
//       (item->param).gun.vec.pad = 0;
//       (item->param).gun.vec.pad = (long)pHVar20;
//       if ((pHVar20->target == (ModelType *)item->owner->model) && ((pHVar20->attribute & 3U) == 0))
//       {
//         (item->param).dokudango.org_think = (undefined *)pHVar20->think[0];
//         pHVar20->target = item->locate;
//         ((item->param).ninken.slave)->think[0] = (undefined **)Think1target;
//       }
//       else {
//         (item->param).dokudango.org_think = (undefined *)0x0;
//       }
//       if (999 < iVar8) {
//         return;
//       }
//       pHVar13 = (item->param).ninken.slave;
//       sVar15 = pHVar13->status;
//       if (sVar15 == 0x10) {
//         return;
//       }
//       if (sVar15 == 8) {
//         return;
//       }
//       if (sVar15 == 7) {
//         return;
//       }
//       if (pHVar13->life < 1) {
//         return;
//       }
//       if (ActionHalt == 0) {
//         FUN_800270c8(pHVar13,3);
//         UpdateMotion(pHVar13->motion,0xf01);
//         pHVar13->status = 0xf;
//         pMVar9 = pHVar13->motion->motion;
//         MoveHumanoid(pHVar13,(ushort)pMVar9->orderspd,(ushort)pMVar9->sidespd);
//       }
//       pMVar12 = ((item->param).ninken.slave)->model;
//       if (pMVar12->n < 0xf) {
//         (item->locate->locate).super = &pMVar12->object[2]->locate;
//         (item->locate->locate).coord.t[0] = 0;
//         (item->locate->locate).coord.t[1] = 0;
//         (item->locate->locate).coord.t[2] = -0x96;
//       }
//       else {
//         (item->locate->locate).super = &pMVar12->object[0xe]->locate;
//         (item->locate->locate).coord.t[0] = 0;
//         (item->locate->locate).coord.t[1] = 0x32;
//         (item->locate->locate).coord.t[2] = 0;
//       }
// LAB_800417f0:
//       item->mode = item->mode + '\x01';
//       return;
//     }
//     if (bVar1 < 2) {
//       if (bVar1 != 0) {
//         return;
//       }
//       uVar2 = (item->param).lightningbolt.rot.vz - 1;
//       (item->param).lightningbolt.rot.vz = uVar2;
//       if (0 < (int)((uint)uVar2 << 0x10)) {
//         return;
//       }
//       goto LAB_800417f0;
//     }
//     if (bVar1 == 2) {
//       iVar3 = FUN_800294dc((item->param).gun.vec.pad);
//       if (iVar3 != 0) {
//         pHVar13 = (item->param).ninken.slave;
//         if (pHVar13->motion->mid != 0xf01) {
//           pVVar6 = GetAbsolutePosition(item->locate,0,0,0);
//           (item->locate->locate).super = (_GsCOORDINATE2 *)0x0;
//           (item->locate->locate).coord.t[0] = pVVar6->vx;
//           (item->locate->locate).coord.t[1] = pVVar6->vy;
//           (item->locate->locate).coord.t[2] = pVVar6->vz;
//           iVar3 = rand();
//           iVar7 = rand();
//           iVar8 = rand();
//           (item->param).lightningbolt.rot.vz = 0x1e;
//           (item->param).napalm.vec.vz = (short)iVar3 + (short)(iVar3 / 200) * -200 + -100;
//           (item->param).napalm.vec.pad = (short)iVar7 + (short)(iVar7 / 100) * -100 + -200;
//           (item->param).smoke.koro.hint = (AreaNodeType *)0x0;
//           *(undefined1 *)((int)&item->param + 10) = 0;
//           *(short *)((int)&item->param + 8) = (short)iVar8 + (short)(iVar8 / 200) * -200 + -100;
//           item->mode = '\0';
//           return;
//         }
//         if (pHVar13->motion->count != 0x37) {
//           if ((pHVar13->attribute & 0x40U) == 0) {
//             return;
//           }
//           if ((pHVar13->type & 0xf0U) == 0xa0) {
//             return;
//           }
//           NowReturnNormal(pHVar13);
//           return;
//         }
//         iVar3 = FUN_800294dc((item->param).gun.vec.pad);
//         if ((iVar3 != 0) &&
//            (ppuVar10 = (undefined **)(item->param).dokudango.org_think,
//            ppuVar10 != (undefined **)0x0)) {
//           ((item->param).ninken.slave)->think[0] = ppuVar10;
//           ((item->param).ninken.slave)->target = (ModelType *)item->owner->model;
//         }
//         (item->param).gun.vec.pad = 0;
//         (item->param).ninken.slave = pHVar13;
//         NowReturnNormal(pHVar13);
//         (item->param).lightningbolt.rot.vz = 600;
//         goto LAB_800417f0;
//       }
//     }
//     else {
//       if (bVar1 != 3) {
//         return;
//       }
//       iVar3 = FUN_800294dc((item->param).gun.vec.pad);
//       if (((iVar3 != 0) &&
//           (sVar15 = (item->param).lightningbolt.rot.vz,
//           (item->param).lightningbolt.rot.vz = sVar15 + -1, sVar15 != 1)) &&
//          (pHVar13 = (item->param).ninken.slave, 0 < pHVar13->life)) {
//         sVar15 = pHVar13->status;
//         if (sVar15 == 0x10) {
//           return;
//         }
//         if (sVar15 == 8) {
//           return;
//         }
//         if (sVar15 == 7) {
//           return;
//         }
//         if (sVar15 == 0xf) {
//           return;
//         }
//         iVar3 = rand();
//         if (2 < iVar3 % 0x1e) {
//           return;
//         }
//         pHVar13 = (item->param).ninken.slave;
//         if ((pHVar13->type & 0xf0U) == 0xa0) {
//           if ((ActionHalt != 0) || (pHVar13->life < 1)) goto LAB_800419f8;
//           FUN_800270c8(pHVar13,3);
//           mmp = pHVar13->motion;
//           sVar15 = 0x1000;
//         }
//         else {
//           if ((ActionHalt != 0) || (pHVar13->life < 1)) goto LAB_800419f8;
//           FUN_800270c8(pHVar13,3);
//           mmp = pHVar13->motion;
//           sVar15 = 0x100b;
//         }
//         UpdateMotion(mmp,sVar15);
//         pHVar13->status = 0xf;
//         pMVar9 = pHVar13->motion->motion;
//         MoveHumanoid(pHVar13,(ushort)pMVar9->orderspd,(ushort)pMVar9->sidespd);
// LAB_800419f8:
//         Sound((item->param).ninken.slave,6);
//         return;
//       }
//     }
//     ppuVar10 = item->proc;
//     if (ppuVar10 == (undefined **)0x0) {
//       return;
//     }
//     item->mode = 0xff;
//   }
//   (*(code *)ppuVar10)(item);
//   DeleteConflict(item->locate);
//   if (item->mode != 0) {
//     AdtMessageBox("item dispose fail   id %d  mode %d",item->type,(uint)item->mode);
//   }
//   item->owner = (Humanoid *)0x0;
//   item->proc = (undefined **)0x0;
//   return;
// }

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// ? AdtMessageBox(? *, s32, u8);                      /* extern */
// ? DeleteConflict(void *);                           /* extern */
// ? DrawSprite(void *);                               /* extern */
// void *GetAbsolutePosition(void *, ?, ?, ?);         /* extern */
// s32 GetVectorDistance(void **, s32);                /* extern */
// ? MoveHumanoid(void *, u8, u8);                     /* extern */
// ? MoveKorogari(void *, void *);                     /* extern */
// ? NowReturnNormal(void *, void *);                  /* extern */
// ? ReqItemDrop(s32 *);                               /* extern */
// ? Sound(void *, ?);                                 /* extern */
// ? UpdateCoordinate(void *);                         /* extern */
// ? UpdateMotion(void *, ?);                          /* extern */
// ? dispose_weapon_data_of_char_(void *, ?);          /* extern */
// s32 is_character_state_present_on_stage_(void *, void *); /* extern */
// s32 rand(void *);                                   /* extern */
// extern s16 ActionHalt;
// extern ? D_800121CC;
// extern s32 GameClock;
// extern ? HumanGroup;
// extern s16 Humans;
// extern ? Think1target;
//
// void ProcItemDokudango(void *arg0) {
//     void *sp10;
//     s32 sp30;
//     void *sp34;
//     s32 sp38;
//     s32 sp3C;
//     s32 sp40;
//     s32 sp48;
//     s32 sp4C;
//     s32 sp50;
//     ? (*temp_v0_4)(void *);
//     ? *temp_v1;
//     ? var_a1;
//     s16 temp_v1_10;
//     s16 temp_v1_6;
//     s32 temp_hi;
//     s32 temp_s0_5;
//     s32 temp_s1;
//     s32 temp_v0_3;
//     s32 temp_v0_8;
//     s32 temp_v1_5;
//     s32 temp_v1_8;
//     s32 var_fp;
//     s32 var_s1;
//     s32 var_s7;
//     u16 temp_v0_10;
//     u16 temp_v0_2;
//     u8 temp_v0;
//     u8 temp_v0_5;
//     u8 temp_v0_9;
//     u8 temp_v1_2;
//     void **var_s2;
//     void *temp_a0;
//     void *temp_a0_2;
//     void *temp_a0_3;
//     void *temp_a1;
//     void *temp_s0;
//     void *temp_s0_2;
//     void *temp_s0_3;
//     void *temp_s0_4;
//     void *temp_s0_6;
//     void *temp_s0_7;
//     void *temp_s5;
//     void *temp_v0_11;
//     void *temp_v0_6;
//     void *temp_v0_7;
//     void *temp_v1_3;
//     void *temp_v1_4;
//     void *temp_v1_7;
//     void *temp_v1_9;
//     void *var_s6;
//     void *var_v0;
//     void *var_v1;
//     void *var_v1_2;
//
//     temp_v0 = arg0->unk54;
//     temp_s0 = arg0->unk4;
//     temp_s5 = arg0 + 0x20;
//     if (temp_v0 == 0xFF) {
//         if (is_character_state_present_on_stage_(temp_s5->unkC) != 0) {
//             temp_v1 = temp_s5->unk10;
//             if (temp_v1 != NULL) {
//                 temp_s5->unkC->unk60 = temp_v1;
//                 temp_s5->unkC->unk74 = (s32) arg0->unk0->unk58;
//             }
//         }
//         temp_s5->unkC = NULL;
//         arg0->unk54 = 0U;
//         return;
//     }
//     if ((temp_v0 < 2U) && (MoveKorogari(arg0, temp_s5), (temp_s5->unkA == 1))) {
//         if (arg0->unkC != NULL) {
//             arg0->unk54 = 0xFFU;
//             goto block_80;
//         }
//     } else {
//         if ((u8) arg0->unk54 < 3U) {
//             UpdateCoordinate(arg0->unk10);
//             var_v0 = arg0->unk10;
//             var_v1 = temp_s0;
//             temp_a0 = var_v0 + 0x50;
//             do {
//                 var_v1->unk0 = (s32) var_v0->unk0;
//                 var_v1->unk4 = (s32) var_v0->unk4;
//                 var_v1->unk8 = (s32) var_v0->unk8;
//                 var_v1->unkC = (s32) var_v0->unkC;
//                 var_v0 += 0x10;
//                 var_v1 += 0x10;
//             } while (var_v0 != temp_a0);
//             DrawSprite(temp_s0);
//         }
//         temp_v1_2 = arg0->unk54;
//         switch (temp_v1_2) {                        /* irregular */
//         case 0:
//             temp_v0_2 = temp_s5->unk14 - 1;
//             temp_s5->unk14 = temp_v0_2;
//             if ((temp_v0_2 << 0x10) > 0) {
//                 return;
//             }
// block_71:
//             arg0->unk54 = (u8) (arg0->unk54 + 1);
//             return;
//         case 1:
//             var_s6 = NULL;
//             if (!(GameClock & 1)) {
//                 var_s7 = 0x2710;
//                 temp_v1_3 = arg0->unk10;
//                 var_fp = 0x2710;
//                 sp10.unk8 = 0;
//                 temp_v1_4 = temp_v1_3 + 0x18;
//                 sp10.unkC = (s32) temp_v1_3->unk18;
//                 sp10.unk10 = (s32) temp_v1_4->unk4;
//                 sp10.unk1C = 0x2710;
//                 sp10.unk14 = (s32) temp_v1_4->unk8;
// loop_24:
//                 var_s1 = sp10.unk8;
//                 var_s2 = (var_s1 * 4) + &HumanGroup;
// loop_25:
//                 var_v1_2 = NULL;
//                 if (var_s1 < Humans) {
//                     temp_s0_2 = *var_s2;
//                     if ((temp_s0_2->unk8 <= 0) || (*temp_s0_2->unk5C == 0x100) || (temp_s0_2->unk4 & 0x80) || (temp_v0_3 = GetVectorDistance(&sp10 + 0xC, temp_s0_2->unk38), var_v1_2 = temp_s0_2, ((temp_v0_3 < sp10.unk1C) == 0))) {
//                         var_s2 += 4;
//                         var_s1 += 1;
//                         goto loop_25;
//                     }
//                     sp10.unk0 = temp_s0_2;
//                     sp10.unk4 = temp_v0_3;
//                     sp10.unk8 = (s32) (var_s1 + 1);
//                 }
//                 if (var_v1_2 != NULL) {
//                     if (((sp10->unk0 & 0xF0) != 0x80) && (sp10->unk8 != -1) && (sp14 < var_s7)) {
//                         if (sp10 == arg0->unk0) {
//                             var_fp = sp14;
//                         } else {
//                             var_s6 = sp10;
//                             var_s7 = sp14;
//                         }
//                     }
//                     goto loop_24;
//                 }
//                 if (var_fp < 0x1F4) {
//                     sp30 = arg0->unk8;
//                     sp34 = arg0->unk0;
//                     sp38 = arg0->unk10->unk18;
//                     sp3C = arg0->unk10->unk1C;
//                     sp48 = 0;
//                     sp4C = 0;
//                     sp50 = 0;
//                     sp40 = arg0->unk10->unk20;
//                     temp_v0_4 = arg0->unkC;
//                     if (temp_v0_4 != NULL) {
//                         arg0->unk54 = 0xFFU;
//                         temp_v0_4(arg0);
//                         DeleteConflict(arg0->unk10);
//                         temp_v0_5 = arg0->unk54;
//                         if (temp_v0_5 != 0) {
//                             AdtMessageBox(&D_800121CC, arg0->unk8, temp_v0_5);
//                         }
//                         arg0->unk0 = NULL;
//                         arg0->unkC = NULL;
//                     }
//                     ReqItemDrop(&sp30);
//                     return;
//                 }
//                 temp_s0_3 = arg0 + 0x20;
//                 if (var_s6 != NULL) {
//                     if (is_character_state_present_on_stage_(temp_s0_3->unkC) != 0) {
//                         temp_v1_5 = temp_s0_3->unk10;
//                         if (temp_v1_5 != 0) {
//                             temp_s0_3->unkC->unk60 = temp_v1_5;
//                             temp_s0_3->unkC->unk74 = (s32) arg0->unk0->unk58;
//                         }
//                     }
//                     temp_s0_3->unkC = NULL;
//                     temp_s5->unkC = var_s6;
//                     if ((var_s6->unk74 == arg0->unk0->unk58) && !(var_s6->unk4 & 3)) {
//                         temp_s5->unk10 = (? *) var_s6->unk60;
//                         var_s6->unk74 = (void *) arg0->unk10;
//                         temp_s5->unkC->unk60 = &Think1target;
//                     } else {
//                         temp_s5->unk10 = NULL;
//                     }
//                     if (var_s7 < 0x3E8) {
//                         temp_s0_4 = temp_s5->unkC;
//                         temp_v1_6 = temp_s0_4->unk2;
//                         if ((temp_v1_6 != 0x10) && (temp_v1_6 != 8) && (temp_v1_6 != 7) && (temp_s0_4->unk8 > 0)) {
//                             if (ActionHalt == 0) {
//                                 dispose_weapon_data_of_char_(temp_s0_4, 3);
//                                 UpdateMotion(temp_s0_4->unk5C, 0xF01);
//                                 temp_s0_4->unk2 = 0xF;
//                                 temp_v0_6 = temp_s0_4->unk5C->unk10;
//                                 MoveHumanoid(temp_s0_4, temp_v0_6->unk2, temp_v0_6->unk3);
//                             }
//                             temp_v1_7 = temp_s5->unkC->unk58;
//                             if (temp_v1_7->unk64 >= 0xF) {
//                                 arg0->unk10->unk48 = (s32) temp_v1_7->unk68->unk38;
//                                 arg0->unk10->unk18 = 0;
//                                 arg0->unk10->unk1C = 0x32;
//                                 arg0->unk10->unk20 = 0;
//                             } else {
//                                 arg0->unk10->unk48 = (s32) temp_v1_7->unk68->unk8;
//                                 arg0->unk10->unk18 = 0;
//                                 arg0->unk10->unk1C = 0;
//                                 arg0->unk10->unk20 = -0x96;
//                             }
//                             goto block_71;
//                         }
//                     }
//                 }
//             } else {
//                 return;
//             }
//             break;
//         case 2:
//             if (is_character_state_present_on_stage_(temp_s5->unkC) != 0) {
//                 temp_a1 = temp_s5->unkC;
//                 temp_a0_2 = temp_a1->unk5C;
//                 if (temp_a0_2->unk0 != 0xF01) {
//                     temp_v0_7 = GetAbsolutePosition(arg0->unk10, 0, 0, 0);
//                     arg0->unk10->unk48 = 0;
//                     arg0->unk10->unk18 = (s32) temp_v0_7->unk0;
//                     temp_a0_3 = arg0->unk10;
//                     temp_a0_3->unk1C = (s32) temp_v0_7->unk4;
//                     arg0->unk10->unk20 = (s32) temp_v0_7->unk8;
//                     temp_s1 = rand(temp_a0_3) % 200;
//                     temp_s0_5 = rand() % 100;
//                     temp_v0_8 = rand();
//                     temp_hi = MULT_HI(temp_v0_8, 0x51EB851F);
//                     temp_s5->unk14 = 0x1EU;
//                     temp_s5->unk4 = (s16) (temp_s1 - 0x64);
//                     temp_s5->unk6 = (s16) (temp_s0_5 - 0xC8);
//                     arg0->unk20 = 0;
//                     temp_s5->unkA = 0U;
//                     temp_s5->unk8 = (s16) ((temp_v0_8 - (((temp_hi >> 6) - (temp_v0_8 >> 0x1F)) * 0xC8)) - 0x64);
//                     arg0->unk54 = 0U;
//                     return;
//                 }
//                 temp_s0_6 = arg0 + 0x20;
//                 if (temp_a0_2->unk2 == 0x37) {
//                     if (is_character_state_present_on_stage_(temp_s0_6->unkC, temp_a1) != 0) {
//                         temp_v1_8 = temp_s0_6->unk10;
//                         if (temp_v1_8 != 0) {
//                             temp_s0_6->unkC->unk60 = temp_v1_8;
//                             temp_s0_6->unkC->unk74 = (s32) arg0->unk0->unk58;
//                         }
//                     }
//                     temp_s0_6->unkC = NULL;
//                     temp_s5->unkC = temp_a1;
//                     NowReturnNormal(temp_a1);
//                     temp_s5->unk14 = 0x258U;
//                     goto block_71;
//                 }
//                 if ((temp_a1->unk4 & 0x40) && ((temp_a1->unk0 & 0xF0) != 0xA0)) {
//                     NowReturnNormal(temp_a1, temp_a1);
//                     return;
//                 }
//             } else {
// block_78:
//                 if (arg0->unkC != NULL) {
//                     arg0->unk54 = 0xFFU;
// block_80:
//                     arg0->unkC(arg0);
//                     DeleteConflict(arg0->unk10);
//                     temp_v0_9 = arg0->unk54;
//                     if (temp_v0_9 != 0) {
//                         AdtMessageBox(&D_800121CC, arg0->unk8, temp_v0_9);
//                     }
//                     arg0->unk0 = NULL;
//                     arg0->unkC = NULL;
//                     return;
//                 }
//             }
//             break;
//         case 3:
//             if ((is_character_state_present_on_stage_(temp_s5->unkC) == 0) || (temp_v0_10 = temp_s5->unk14 - 1, temp_s5->unk14 = temp_v0_10, ((temp_v0_10 << 0x10) == 0)) || (temp_v1_9 = temp_s5->unkC, (temp_v1_9->unk8 <= 0))) {
//                 goto block_78;
//             }
//             temp_v1_10 = temp_v1_9->unk2;
//             if ((temp_v1_10 != 0x10) && (temp_v1_10 != 8) && (temp_v1_10 != 7) && (temp_v1_10 != 0xF) && ((rand() % 30) < 3)) {
//                 temp_s0_7 = temp_s5->unkC;
//                 if ((temp_s0_7->unk0 & 0xF0) == 0xA0) {
//                     if ((ActionHalt == 0) && (temp_s0_7->unk8 > 0)) {
//                         dispose_weapon_data_of_char_(temp_s0_7, 3);
//                         var_a1 = 0x1000;
//                         goto block_95;
//                     }
//                 } else if ((ActionHalt == 0) && (temp_s0_7->unk8 > 0)) {
//                     dispose_weapon_data_of_char_(temp_s0_7, 3);
//                     var_a1 = 0x100B;
// block_95:
//                     UpdateMotion(temp_s0_7->unk5C, var_a1);
//                     temp_s0_7->unk2 = 0xF;
//                     temp_v0_11 = temp_s0_7->unk5C->unk10;
//                     MoveHumanoid(temp_s0_7, temp_v0_11->unk2, temp_v0_11->unk3);
//                 }
//                 Sound(temp_s5->unkC, 6);
//             }
//             break;
//         }
//     }
// }
