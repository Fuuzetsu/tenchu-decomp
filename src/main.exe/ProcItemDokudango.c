#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemDokudango(struct tag_TItem *item);
 *     ITEM.C:1632, 141 src lines, frame 128 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
extern s32 is_humanoid_on_stage_(Humanoid *human);
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
    enum
    {
        DOKUDANGO_MODE_ROLL = 0,
        DOKUDANGO_MODE_SEARCH = 1,
        DOKUDANGO_MODE_EAT = 2,
        DOKUDANGO_MODE_POISON = 3,
        DOKUDANGO_PICKUP_RANGE = 500,
        DOKUDANGO_EAT_RANGE = 1000,
        DOKUDANGO_ROLL_DELAY = 30,
        DOKUDANGO_EAT_FRAME = 55,
        DOKUDANGO_POISON_DURATION = 600,
        DOKUDANGO_REACTION_PERIOD = 30,
        DOKUDANGO_REACTION_CHANCE = 3
    };
    Sprite3D *model;
    param_dokudango *param;

    model = item->model.sprite;
    param = &item->param.dokudango;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        param_dokudango *restore_param;

        restore_param = param;
        if (is_humanoid_on_stage_(restore_param->eater) != 0 &&
            restore_param->org_think != 0)
        {
            restore_param->eater->think[0] = restore_param->org_think;
            restore_param->eater->target.archive = item->owner.human->model;
        }
        restore_param->eater = 0;
        item->mode = DOKUDANGO_MODE_ROLL;
        return;
    }

    if (item->mode < DOKUDANGO_MODE_EAT &&
        (MoveKorogari(item, &param->koro),
         param->koro.status == KORO_WATER))
    {
        if (item->proc == 0)
        {
            return;
        }
        DISPOSE_ITEM(item);
        return;
    }
    else
    {
        if (item->mode < DOKUDANGO_MODE_POISON)
        {
            UpdateCoordinate(item->locate);
            model->locate = item->locate->locate;
            DrawSprite(model);
        }

        switch (item->mode)
        {
        case DOKUDANGO_MODE_ROLL:
        {
            u16 roll_countdown;

            roll_countdown = param->count - 1;
            param->count = roll_countdown;
            if ((s16)roll_countdown > 0)
            {
                return;
            }
            item->mode++;
            return;
        }

        case DOKUDANGO_MODE_SEARCH:
        {
            TFindItemTarget search_state;
            TFindItemTarget *search_setup;
            TFindItemTarget *search;
            VECTOR *item_position;
            Humanoid **human_cursor;
            Humanoid *nearest_target;
            Humanoid *candidate;
            Humanoid *eater;
            Humanoid *scan_result;
            s32 nearest_distance;
            s32 owner_distance;
            s32 human_index;
            s32 candidate_distance;

            if ((GameClock & 1) != 0)
            {
                return;
            }
            nearest_target = 0;
            nearest_distance = DOKUDANGO_RANGE;
            search_setup = &search_state;
            item_position = (VECTOR *)item->locate->locate.coord.t;
            owner_distance = nearest_distance;
            search_setup->i = 0;
            search_setup->pos.vx = item_position->vx;
            search = &search_state;
            search->pos.vy = item_position->vy;
            search->pos.vz = item_position->vz;
            search->find_dist = nearest_distance;

            while (1)
            {
                human_index = search->i;
                human_cursor = HumanGroup + human_index;
                while (1)
                {
                    if (human_index < Humans)
                    {
                        candidate = *human_cursor;
                        if (candidate->life > 0 &&
                            candidate->motion->mid != MOT_ACTION &&
                            (candidate->attribute & ATTR_SUSPEND) == 0)
                        {
                            candidate_distance =
                                GetVectorDistance(&search->pos,
                                                  candidate->locate);
                            if (candidate_distance < search->find_dist)
                            {
                                goto hit;
                            }
                        }
                        /* GCC folds this unsigned pointer progression after
                         * flow; its extra human_cursor reference replaces
                         * the old allocation-only wrapper. */
                        human_cursor = (Humanoid **)(
                            ((u32)human_cursor + (u32)human_cursor) -
                            (u32)human_cursor) + 1;
                        human_index++;
                        continue;
                    }
                    scan_result = 0;
                    break;
                }
            check:
                if (scan_result == 0)
                {
                    break;
                }
                if ((search_state.find->type & PAGE_MASK) != PAGE_BOSS &&
                    search_state.find->life != HUMANOID_LIFE_INACTIVE &&
                    search_state.dist < nearest_distance)
                {
                    if (search_state.find != item->owner.human)
                    {
                        goto set_target;
                    }
                    owner_distance = search_state.dist;
                }
                continue;
            hit:
                scan_result = candidate;
                do
                {
                    search->find = candidate;
                    search->dist = candidate_distance;
                    search->i = human_index + 1;
                } while (0);
                goto check;
            set_target:
                nearest_target = search_state.find;
                nearest_distance = search_state.dist;
                continue;
            }

            if (owner_distance < DOKUDANGO_PICKUP_RANGE)
            {
                PARAM_ITEM_LAUNCH drop_request;

                drop_request.type = item->type;
                drop_request.user.human = item->owner.human;
                drop_request.start.vx = item->locate->locate.coord.t[0];
                drop_request.start.vy = item->locate->locate.coord.t[1];
                drop_request.start.vz = item->locate->locate.coord.t[2];
                drop_request.end.vx = 0;
                drop_request.end.vy = 0;
                drop_request.end.vz = 0;
                if (item->proc != 0)
                {
                    item->mode = ITEM_MODE_DISPOSE;
                    item->proc(item);
                    DeleteConflict(item->locate);
                    if (item->mode != DOKUDANGO_MODE_ROLL)
                    {
                        AdtMessageBox(msg_item_dispose_fail, item->type,
                                      (u32)item->mode);
                    }
                    item->owner.human = 0;
                    item->proc = 0;
                }
                ReqItemDrop(&drop_request);
                return;
            }

            if (nearest_target == 0)
            {
                return;
            }
            {
                param_dokudango *restore_param;

                restore_param = &item->param.dokudango;
                if (is_humanoid_on_stage_(restore_param->eater) != 0 &&
                    restore_param->org_think != 0)
                {
                    restore_param->eater->think[0] = restore_param->org_think;
                    restore_param->eater->target.archive =
                        item->owner.human->model;
                }
                restore_param->eater = 0;
            }
            param->eater = nearest_target;
            if (nearest_target->target.archive == item->owner.human->model &&
                (nearest_target->attribute & ATTR_PHASE) == PHASE_CALM)
            {
                param->org_think = nearest_target->think[0];
                param->eater->target.model = item->locate;
                param->eater->think[0] = Think1target;
            }
            else
            {
                param->org_think = 0;
            }

            if (nearest_distance >= DOKUDANGO_EAT_RANGE)
            {
                return;
            }
            eater = param->eater;
            if (eater->status == STAT_DAMAGE || eater->status == STAT_STATE ||
                eater->status == STAT_ATTACK || eater->life <= 0)
            {
                return;
            }
            if (ActionHalt == 0)
            {
                MotionDataType *motion_data;

                dispose_weapon_data_of_char_(eater, ATTACK_CANCEL_ALL);
                UpdateMotion(eater->motion, MOT_ITEM_DRINK);
                eater->status = STAT_ITEM;
                motion_data = eater->motion->motion;
                MoveHumanoid(eater, motion_data->orderspd,
                             motion_data->sidespd);
            }
            if (param->eater->model->n > MODEL_PART_WEAPON_HAND_1)
            {
                item->locate->locate.super =
                    &param->eater->model
                         ->object[MODEL_PART_WEAPON_HAND_1]->locate;
                item->locate->locate.coord.t[0] = 0;
                item->locate->locate.coord.t[1] = 50;
                item->locate->locate.coord.t[2] = 0;
            }
            else
            {
                item->locate->locate.super =
                    &param->eater->model
                         ->object[MODEL_PART_BEAST_HAND_0]->locate;
                item->locate->locate.coord.t[0] = 0;
                item->locate->locate.coord.t[1] = 0;
                item->locate->locate.coord.t[2] = -150;
            }
            item->mode++;
            return;
        }

        case DOKUDANGO_MODE_EAT:
        {
            MotionManager *eating_motion;
            Humanoid *eater;

            if (is_humanoid_on_stage_(param->eater) == 0)
            {
                goto dispose_poison;
            }
            eater = param->eater;
            eating_motion = eater->motion;
            if (eating_motion->mid != MOT_ITEM_DRINK)
            {
                VECTOR *world_position;
                s32 random_x;
                s32 random_y;
                s32 random_z;

                world_position = GetAbsolutePosition(item->locate, 0, 0, 0);
                item->locate->locate.super = 0;
                item->locate->locate.coord.t[0] = world_position->vx;
                item->locate->locate.coord.t[1] = world_position->vy;
                item->locate->locate.coord.t[2] = world_position->vz;
                random_x = rand();
                random_x = random_x % 200;
                random_y = rand();
                random_y = random_y % 100;
                random_z = rand();
                random_z = random_z % 200;
                param->koro.vx = random_x - 100;
                param->koro.vy = random_y - 200;
                param->count = DOKUDANGO_ROLL_DELAY;
                param->koro.hint = 0;
                param->koro.status = KORO_NORMAL;
                param->koro.vz = random_z - 100;
                item->mode = DOKUDANGO_MODE_ROLL;
                return;
            }
            if (eating_motion->count == DOKUDANGO_EAT_FRAME)
            {
                Humanoid *saved_eater;
                param_dokudango *restore_param;

                saved_eater = eater;
                restore_param = &item->param.dokudango;
                if (is_humanoid_on_stage_(restore_param->eater) != 0 &&
                    restore_param->org_think != 0)
                {
                    restore_param->eater->think[0] = restore_param->org_think;
                    restore_param->eater->target.archive =
                        item->owner.human->model;
                }
                restore_param->eater = 0;
                param->eater = saved_eater;
                NowReturnNormal(saved_eater);
                param->count = DOKUDANGO_POISON_DURATION;
                item->mode++;
                return;
            }
            if ((eater->attribute & ATTR_ALERT) != 0 &&
                (eater->type & PAGE_MASK) != PAGE_BEAST)
            {
                NowReturnNormal(eater);
            }
            return;
        }
        case DOKUDANGO_MODE_POISON:
        {
            Humanoid *poisoned_eater;
            Humanoid *reaction_target;
            s32 poison_countdown;

            if (is_humanoid_on_stage_(param->eater) == 0)
            {
                goto dispose_poison;
            }
            poison_countdown = param->count - 1;
            param->count = poison_countdown;
            if ((poison_countdown << 16) == 0)
            {
                goto dispose_poison;
            }
            poisoned_eater = param->eater;
            if (poisoned_eater->life > 0)
            {
                goto poison_active;
            }
        dispose_poison:
        {
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }

        poison_active:
            if (poisoned_eater->status == STAT_DAMAGE ||
                poisoned_eater->status == STAT_STATE ||
                poisoned_eater->status == STAT_ATTACK ||
                poisoned_eater->status == STAT_ITEM)
            {
                return;
            }
            if (rand() % DOKUDANGO_REACTION_PERIOD >=
                DOKUDANGO_REACTION_CHANCE)
            {
                return;
            }
            reaction_target = param->eater;
            if ((reaction_target->type & PAGE_MASK) == PAGE_BEAST)
            {
                if (ActionHalt == 0 && reaction_target->life > 0)
                {
                    MotionDataType *motion_data;

                    dispose_weapon_data_of_char_(reaction_target,
                                                 ATTACK_CANCEL_ALL);
                    UpdateMotion(reaction_target->motion, MOT_DAMAGE);
                    reaction_target->status = STAT_ITEM;
                    motion_data = reaction_target->motion->motion;
                    MoveHumanoid(reaction_target, motion_data->orderspd,
                                 motion_data->sidespd);
                }
            }
            else if (ActionHalt == 0 && reaction_target->life > 0)
            {
                MotionDataType *motion_data;

                dispose_weapon_data_of_char_(reaction_target,
                                             ATTACK_CANCEL_ALL);
                UpdateMotion(reaction_target->motion, MOT_DAMAGE_CHOKE);
                reaction_target->status = STAT_ITEM;
                motion_data = reaction_target->motion->motion;
                MoveHumanoid(reaction_target, motion_data->orderspd,
                             motion_data->sidespd);
            }
            Sound(param->eater, CHAR_VOICE_HURT);
            return;
        }

        default:
            return;
        }
    }
}
