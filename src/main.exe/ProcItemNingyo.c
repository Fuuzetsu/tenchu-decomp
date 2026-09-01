#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"
#include "sound.h"

typedef union
{
    struct
    {
        SVECTOR smoke_velocity;
        PARAM_ITEM_LAUNCH request;
    } drop;
    struct
    {
        VECTOR scale;
        VECTOR source_scale;
    } growth;
    struct
    {
        VECTOR position;
        VECTOR source_position;
    } impact;
} ProcItemNingyoScratch;

extern SVECTOR svec_y_n25[]; /* {0,-25,0} */
extern u8 NingyoCount;

extern void MoveKorogari(TItem *item, param_korogari *param);
extern short DrawModel(ModelType *objp);

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemNingyo(struct tag_TItem *item);
 *     ITEM.C:1882, 132 src lines, frame 112 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s5       struct tag_TItem * item
 *     reg   $s4       struct param_ningyo * param
 *     reg   $a0       int cid
 *     reg   $s5       struct tag_TItem * item
 *     stack sp+24     struct SVECTOR sv
 *     stack sp+32     struct VECTOR v
 *     reg   $s5       struct tag_TItem * item
 *     reg   $v0       int t
 *     reg   $a0       struct ModelType * model
 *     reg   $s2       int i
 *     reg   $s0       struct Humanoid * human
 *     reg   $s1       long len
 *     reg   $v0       struct Humanoid * human
 *     reg   $v0       struct Humanoid * human
 *     reg   $a1       int i
 *     reg   $s5       struct tag_TItem * item
 *     stack sp+48     struct VECTOR pos
 *     reg   $s4       struct param_korogari * param
 *     reg   $s4       struct param_korogari * param
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct TCameraStatus CamState;
 *     extern struct Humanoid *HumanGroup[32];
 *     extern struct ModelType *NingyoModel;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct SVECTOR ConflictDistance;
 * END PSX.SYM */

/* MATCH (retail): the pure-C body has the exact 0x68 frame, 564 instructions,
 * exact 36/12/33/1 branch/jump/call/return inventory, and target
 * item/param/sentinel homes s3/s4/s5.  The drop path's direct model load
 * preserves the target owner/type/model load order in s2/s1/s0.
 *
 * Clearing the short-lived request pointer after memset breaks the stack-
 * address CSE that otherwise occupies s3.  Reusing the model pointer for its
 * embedded position then makes the derived-address and all three shared
 * modulus-constant sequences exact. Separate base/result conflict pointers
 * and the constant one-shot make the mode-1 address and constant ordering
 * exact. Two unsigned item identities replace the dispose pair and
 * InsertConflict wrapper: together their six flow references keep item above
 * param and preserve the indirect-call delay slots without CFG artifacts. */
void ProcItemNingyo(TItem *item)
{
    enum
    {
        NINGYO_MODE_WAIT = 0,
        NINGYO_MODE_GROW = 1,
        NINGYO_MODE_ACTIVE = 2,
        MAX_ACTIVE_NINGYO = 3,
        ACTIVE_NINGYO_HP = 3,
        APPEAR_SMOKE_COUNT = 10,
        APPEAR_SMOKE_TIME = 6,
        DROP_HORIZONTAL_SPREAD = 200,
        DROP_VERTICAL_SPREAD = 100,
        DROP_UPWARD_SPEED = 200,
        GROWTH_SCALE_SHIFT = 8,
        GROWTH_FRAMES = FIXED_ONE >> GROWTH_SCALE_SHIFT,
        NINGYO_COLLISION_SIZE = 500,
        FIRST_RETARGET_DELAY = 3,
        RETARGET_INTERVAL = 30,
        NINGYO_LURE_RANGE = 10000,
        KNOCKBACK_Y_SPEED = 100,
        KNOCKBACK_SPREAD = 20
    };
    param_ningyo *param;
    s32 conflict_id;
    s32 dispose_mode;
    ProcItemNingyoScratch scratch;

    param = &item->param.ningyo;
    dispose_mode = ITEM_MODE_DISPOSE;
    if (item->mode == dispose_mode)
    {
        if (param->hp != NINGYO_HP)
        {
            s32 human_index;
            s32 human_count;
            Humanoid **human_cursor;

            human_count = Humans;
            if (human_count > 0)
            {
                s32 human_limit;
                TCameraStatus *camera_state;

                do
                {
                    human_index = 0;
                } while (0);
                camera_state = &CamState;
                human_limit = human_count;
                human_cursor = HumanGroup;
                do
                {
                    Humanoid *human;

                    human = *human_cursor;
                    if (human->target == item->locate)
                    {
                        human->target =
                            (ModelType *)camera_state->Owner->model;
                    }
                    human_index++;
                    human_cursor++;
                } while (human_index < human_limit);
            }
            NingyoCount--;
        }
        item->mode = NINGYO_MODE_WAIT;
        return;
    }

    MoveKorogari(item, &param->koro);
    if (param->koro.status == KORO_WATER)
    {
        goto dispose;
    }

    switch (item->mode)
    {
    case NINGYO_MODE_WAIT:
    {
        s32 activation_countdown;

        activation_countdown = param->count - 1;
        param->count = activation_countdown;
        if ((u8)activation_countdown == 0)
        {
            param->count = 0;
            item->mode++;
            scratch.drop.smoke_velocity = svec_y_n25[0];
            SetSmoke((VECTOR *)item->locate->locate.coord.t,
                     &scratch.drop.smoke_velocity,
                     APPEAR_SMOKE_COUNT, APPEAR_SMOKE_TIME);
            SoundEx((VECTOR *)item->locate->locate.coord.t, SE_SMOKE_PUFF);
            if (NingyoCount < MAX_ACTIVE_NINGYO)
            {
                param->hp = ACTIVE_NINGYO_HP;
                NingyoCount++;
                goto draw_mode0;
            }
            else
            {
                Humanoid *owner;
                s32 item_type;
                ModelType *model;
                PARAM_ITEM_LAUNCH *request;

                owner = item->owner;
                item_type = item->type;
                model = item->locate;
                request = &scratch.drop.request;
                memset(request, 0, sizeof(PARAM_ITEM_LAUNCH));
                request = 0;
                scratch.drop.request.type = item_type;
                scratch.drop.request.user = owner;
                {
                    VECTOR *position;

                    position = (VECTOR *)model->locate.coord.t;
                    scratch.drop.request.start.vx = position->vx;
                    scratch.drop.request.start.vy = position->vy;
                    scratch.drop.request.start.vz = position->vz;
                }
                scratch.drop.request.end.vx =
                    rand() % DROP_HORIZONTAL_SPREAD -
                    DROP_HORIZONTAL_SPREAD / 2;
                scratch.drop.request.end.vy =
                    rand() % DROP_VERTICAL_SPREAD - DROP_UPWARD_SPEED;
                scratch.drop.request.end.vz =
                    rand() % DROP_HORIZONTAL_SPREAD -
                    DROP_HORIZONTAL_SPREAD / 2;
                ReqItemDrop(&scratch.drop.request);
            }
        }
        else
        {
            goto draw_mode0;
        }

    dispose:
        /* One extra reference to `item`, folded away after flow.c has
         * already counted it -- allocation staging, not arithmetic. A
         * second copy of this used to sit before InsertConflict below;
         * the two were interchangeable and only one is needed. Simpler
         * identities do not work: x|x, x&x, x^0, x*1 and x+0 all fold
         * before the count. */
        item = (TItem *)(((u32)item + (u32)item) - (u32)item);
        if (item->proc == 0)
        {
            return;
        }
        item->mode = dispose_mode;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != NINGYO_MODE_WAIT)
        {
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        }
        item->owner = 0;
        item->proc = 0;
        return;

    draw_mode0:
        UpdateCoordinate(item->locate);
        item->model.sprite->locate = item->locate->locate;
        DrawSprite(item->model.sprite);
        return;
    }

    case NINGYO_MODE_GROW:
    {
        s32 new_conflict_id;
        s32 collision_size;
        s32 collision_offset_y;
        ConflictClass conflict_class;
        ConflictObjectType *conflict_pool;
        ConflictObjectType *conflict;

        param->count++;
        memset(&scratch.growth.source_scale, 0, sizeof(VECTOR));
        scratch.growth.source_scale.vx =
            param->count << GROWTH_SCALE_SHIFT;
        scratch.growth.source_scale.vy =
            param->count << GROWTH_SCALE_SHIFT;
        scratch.growth.source_scale.vz =
            param->count << GROWTH_SCALE_SHIFT;
        scratch.growth.scale = scratch.growth.source_scale;
        RotMatrixYXZ(&item->locate->rotate, &item->locate->locate.coord);
        ScaleMatrix(&item->locate->locate.coord, &scratch.growth.scale);
        item->locate->locate.flg = 0;
        NingyoModel->locate = item->locate->locate;
        DrawModel(NingyoModel);
        if (param->count < GROWTH_FRAMES)
        {
            return;
        }

        DeleteConflict(item->locate);
        new_conflict_id = InsertConflict(item->locate);
        conflict_pool = ConflictObject;
        conflict = conflict_pool + new_conflict_id;
        collision_offset_y = -NINGYO_COLLISION_SIZE / 2;
        collision_size = NINGYO_COLLISION_SIZE;
        /* empty one-shot: a sched1 region fence (an emptied debug print
         * reads the same way -- see DefaultActionHumanoid's header). */
        do
        {
        } while (0);
        conflict->common.tag = CONFLICT_OWNER_ITEM;
        conflict_class = CONFLICT_STAND | CONFLICT_SOFT;
        conflict->offset.vx = 0;
        conflict->offset.vz = 0;
        conflict->offset.vy = collision_offset_y;
        conflict->size.vz = collision_size;
        conflict->size.vy = collision_size;
        conflict->size.vx = collision_size;
        conflict->size.pad = conflict_class;
        item->collision.mode = conflict_class;
        item->collision.size = collision_size;
        item->collision.ofsY = collision_offset_y;
        item->collision.pause = 0;
        param->count = FIRST_RETARGET_DELAY;
        item->mode++;
        return;
    }

    case NINGYO_MODE_ACTIVE:
    {
        s32 retarget_countdown;

        if ((item->locate->attribute & MODEL_ATTR_CONFLICT) == 0)
        {
            conflict_id = CONFLICT_NONE;
        }
        else
        {
            conflict_id = GetConflictResult(item->locate, CONFLICT_NONE);
        }

        retarget_countdown = param->count - 1;
        param->count = retarget_countdown;
        if ((u8)retarget_countdown == 0)
        {
            s32 human_index;
            Humanoid **human_cursor;

            human_index = 0;
            human_cursor = HumanGroup;
            while (1)
            {
                Humanoid *human;
                s32 distance_to_decoy;

                if (human_index >= Humans)
                {
                    break;
                }
                human = *human_cursor;
                distance_to_decoy = GetVectorDistance(
                    (VECTOR *)item->locate->locate.coord.t,
                    human->locate);
                if (distance_to_decoy < NINGYO_LURE_RANGE &&
                    human->target != 0 &&
                    distance_to_decoy <
                        GetVectorDistance(
                            (VECTOR *)human->target->locate.coord.t,
                            human->locate) &&
                    ((u16)human->type & PAGE_MASK) != PAGE_BOSS)
                {
                    human->target = item->locate;
                }
                human_cursor++;
                human_index++;
            }
            param->count = RETARGET_INTERVAL;
        }
        else if (conflict_id != CONFLICT_NONE)
        {
            ConflictObjectType *conflict;
            ConflictObjectType *conflict_pool;
            ConflictClass conflict_class;

            conflict_pool = ConflictObject;
            conflict = &conflict_pool[conflict_id];
            conflict_class = conflict->size.pad;
            if (conflict_class == CONFLICT_HIT)
            {
                if (param->hp == 0)
                {
                    SetBleeds((VECTOR *)item->locate->locate.coord.t,
                              0, 30, 30, 30, COLOR_YELLOW);
                    SoundEx((VECTOR *)item->locate->locate.coord.t,
                            SE_SMOKE_PUFF);
                    if (item->proc != 0)
                    {
                        item->mode = ITEM_MODE_DISPOSE;
                        item->proc(item);
                        DeleteConflict(item->locate);
                        if (item->mode != NINGYO_MODE_WAIT)
                        {
                            AdtMessageBox(msg_item_dispose_fail, item->type,
                                          (u32)item->mode);
                        }
                        item->owner = 0;
                        item->proc = 0;
                    }
                    item->mode++;
                }
                else
                {
                    s32 delta_z;
                    s32 delta_x;
                    s32 knockback_x;

                    memset(&scratch.impact.source_position, 0,
                           sizeof(VECTOR));
                    scratch.impact.source_position.vx = conflict->position.vx;
                    scratch.impact.source_position.vy = conflict->position.vy;
                    scratch.impact.source_position.vz = conflict->position.vz;
                    scratch.impact.position = scratch.impact.source_position;
                    delta_x = -ConflictDistance.vx;
                    if (delta_x < 0)
                    {
                        delta_x += 15;
                    }
                    knockback_x = delta_x >> 4;
                    delta_z = -ConflictDistance.vz;
                    if (delta_z < 0)
                    {
                        delta_z += 15;
                    }
                    param->koro.vx = knockback_x;
                    param->koro.vy = -KNOCKBACK_Y_SPEED;
                    param->koro.vz = delta_z >> 4;
                    param->koro.hint = 0;
                    param->koro.status = KORO_NORMAL;
                    param->hp--;
                    SoundEx((VECTOR *)item->locate->locate.coord.t,
                            SE_PROJECTILE_HIT);
                }
            }
            else if (conflict_class != CONFLICT_SOFT)
            {
                s32 x_random;
                s32 z_random;
                s32 delta_x;
                s32 knockback_y;
                s32 delta_z;
                s16 knockback_x;
                s16 x_jitter;

                x_random = rand();
                delta_x = -ConflictDistance.vx;
                if (delta_x < 0)
                {
                    delta_x += 7;
                }
                knockback_x = delta_x >> 3;
                x_jitter = x_random % KNOCKBACK_SPREAD;
                knockback_y = 0;
                if (ConflictDistance.vy >= -NINGYO_COLLISION_SIZE)
                {
                    knockback_y = -KNOCKBACK_Y_SPEED;
                }
                z_random = rand();
                delta_z = -ConflictDistance.vz;
                if (delta_z < 0)
                {
                    delta_z += 7;
                }
                param->koro.vx = knockback_x + x_jitter -
                                 KNOCKBACK_SPREAD / 2;
                param->koro.vy = knockback_y;
                param->koro.hint = 0;
                param->koro.status = KORO_NORMAL;
                param->koro.vz = (delta_z >> 3) +
                                 z_random % KNOCKBACK_SPREAD -
                                 KNOCKBACK_SPREAD / 2;
            }
        }

        UpdateCoordinate(item->locate);
        NingyoModel->locate = item->locate->locate;
        DrawModel(NingyoModel);
        return;
    }
    }
    return;
}
