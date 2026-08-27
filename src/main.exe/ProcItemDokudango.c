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

extern void MoveKorogari(TItem *item, param_korogari *param);
extern s32 is_character_state_present_on_stage_(Humanoid *human);
extern s16 Think1target(void);

/*
 * Matching notes (2,468 bytes / 617 instructions):
 *  - The entry comparison and fast disposal use ITEM_MODE_DISPOSE, allowing
 *    CSE to retain its 0xff value in $s1 across MoveKorogari. The two later
 *    cleanup copies rematerialize their own 0xff values in $v1.
 *  - Each cleanup tests and calls item->proc directly.  Combined with the
 *    literal stores, this keeps the indirect target in $v0 and lets jump2
 *    merge the fast cleanup into the final physical copy after its mode store.
 *    A named proc local or a function-wide `ff` local changes that allocation.
 *  - The full cleanup sequence and mode-advance tail remain duplicated at
 *    their semantic exits so late cross-jumping can choose the target copies.
 */

void ProcItemDokudango(TItem *item)
{
    Sprite3D *model;
    param_dokudango *param;

    model = (Sprite3D *)item->model;
    param = &item->param.dokudango;
    if (item->mode == ITEM_MODE_DISPOSE)
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
        (MoveKorogari(item, &param->koro),
         param->koro.status == KORO_WATER))
    {
        if (item->proc == 0)
        {
            return;
        }
        item->mode = ITEM_MODE_DISPOSE;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != 0)
        {
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
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
                        candidate = *group;
                        if (candidate->life > 0 &&
                            candidate->motion->mid != MOT_ACTION &&
                            (candidate->attribute & ATTR_SUSPEND) == 0)
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
                PARAM_ITEM_LAUNCH param;

                param.type = item->type;
                param.user = item->owner;
                param.start.vx = item->locate->locate.coord.t[0];
                param.start.vy = item->locate->locate.coord.t[1];
                param.start.vz = item->locate->locate.coord.t[2];
                param.end.vx = 0;
                param.end.vy = 0;
                param.end.vz = 0;
                if (item->proc != 0)
                {
                    item->mode = ITEM_MODE_DISPOSE;
                    item->proc(item);
                    DeleteConflict(item->locate);
                    if (item->mode != 0)
                    {
                        AdtMessageBox(msg_item_dispose_fail, item->type,
                                      (u32)item->mode);
                    }
                    item->owner = 0;
                    item->proc = 0;
                }
                ReqItemDrop(&param);
                return;
            }

            if (target == 0)
            {
                return;
            }
            {
                param_dokudango *restore;

                restore = &item->param.dokudango;
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
                param->eater->think[0] = Think1target;
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
            if (human->status == STAT_DAMAGE || human->status == STAT_STATE ||
                human->status == STAT_ATTACK || human->life <= 0)
            {
                return;
            }
            if (ActionHalt == 0)
            {
                MotionDataType *motion;

                dispose_weapon_data_of_char_(human, 3);
                UpdateMotion(human->motion, 0xf01);
                human->status = STAT_ITEM;
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
                param->koro.vx = x - 100;
                param->koro.vy = y - 200;
                param->count = 30;
                param->koro.hint = 0;
                param->koro.status = KORO_NORMAL;
                param->koro.vz = z - 100;
                item->mode = 0;
                return;
            }
            if (motion->count == 0x37)
            {
                Humanoid *human;
                param_dokudango *restore;

                human = eater;
                restore = &item->param.dokudango;
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
            if ((eater->attribute & ATTR_ALERT) != 0 &&
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
            item->mode = ITEM_MODE_DISPOSE;
            item->proc(item);
            DeleteConflict(item->locate);
            if (item->mode != 0)
            {
                AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
            }
            item->owner = 0;
            item->proc = 0;
            return;
        }

        poison_active:
            if (eater->status == STAT_DAMAGE || eater->status == STAT_STATE ||
                eater->status == STAT_ATTACK || eater->status == STAT_ITEM)
            {
                return;
            }
            if (rand() % 30 >= 3)
            {
                return;
            }
            human = param->eater;
            if ((human->type & 0xf0) == PAGE_BEAST)
            {
                if (ActionHalt == 0 && human->life > 0)
                {
                    MotionDataType *motion;

                    dispose_weapon_data_of_char_(human, 3);
                    UpdateMotion(human->motion, 0x1000);
                    human->status = STAT_ITEM;
                    motion = human->motion->motion;
                    MoveHumanoid(human, motion->orderspd, motion->sidespd);
                }
            }
            else if (ActionHalt == 0 && human->life > 0)
            {
                MotionDataType *motion;

                dispose_weapon_data_of_char_(human, 3);
                UpdateMotion(human->motion, 0x100b);
                human->status = STAT_ITEM;
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
