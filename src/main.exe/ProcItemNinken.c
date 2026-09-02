#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "humanoid.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void ProcItemNinken(struct tag_TItem *item);
 *     ITEM.C:2368, 89 src lines, frame 64 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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

extern Humanoid *NINKEN_CHARACTER_PTR;
extern SVECTOR svec_y_n50[]; /* {0,-50,0} */

extern void MoveKorogari(TItem *item, param_korogari *param);
extern s32 is_humanoid_on_stage_(Humanoid *human);
extern void set_model_hide_(Humanoid *human, s16 hide);
extern void SetupThinkFunction(Humanoid *human, TThinkType think);
extern void TurnAroundAllItems(Humanoid *human);

void ProcItemNinken(TItem *item)
{
    enum
    {
        NINKEN_MODE_ROLL = 0,
        NINKEN_MODE_SPAWN = 1,
        NINKEN_MODE_ACTIVE = 2
    };
    param_ninken *param;
    s32 water;

    param = &item->param.ninken;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        Humanoid *slave;

        slave = param->slave;
        if (slave != 0)
        {
            NowReturnNormal(slave);
            param->slave->attribute |= ATTR_SUSPEND;
            param->slave->model->locate.coord.t[0] = NINKEN_PARK_POS;
            param->slave->model->locate.coord.t[1] = NINKEN_PARK_POS;
            param->slave->model->locate.coord.t[2] = NINKEN_PARK_POS;
            UpdateCoordinate((ModelType *)param->slave->model);
        }
        item->mode = NINKEN_MODE_ROLL;
        return;
    }

    water = KORO_WATER;
    switch (item->mode)
    {
    case NINKEN_MODE_ROLL:
    {
        u8 status;

        MoveKorogari(item, &param->koro);
        status = param->koro.status;
        if (status == water)
        {
            void (*dispose_proc)(TItem *);

            dispose_proc = item->proc;
            if (dispose_proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }
        if (status == KORO_STAY)
        {
            PARAM_ITEM_STAY saved_record;
            PARAM_ITEM_STAY rparam;
            PARAM_ITEM_LAUNCH launch_record;
            PARAM_ITEM_STAY *saved;
            PARAM_ITEM_LAUNCH *launch;

            memset(&rparam, 0, sizeof(PARAM_ITEM_STAY));
            rparam.type = item->type;
            rparam.locate.vx =
                item->locate->locate.coord.t[0];
            rparam.locate.vy =
                item->locate->locate.coord.t[1];
            rparam.locate.vz =
                item->locate->locate.coord.t[2];
            saved_record = rparam;

            if (item->proc != 0)
            {
                DISPOSE_ITEM(item);
            }

            saved = &saved_record;
            launch = &launch_record;
            launch_record.type = saved->type;
            launch->user = (Humanoid *)CONFLICT_OWNER_ITEM;
            launch_record.start.vx = saved->locate.vx;
            launch_record.start.vy = saved->locate.vy;
            launch_record.start.vz = saved->locate.vz;
            launch_record.end.vx = 0;
            launch_record.end.vy = 0;
            launch_record.end.vz = 0;
            launch_record.start.vy = GetAreaMapLevel(
                GlobalAreaMap, launch_record.start.vx,
                launch_record.start.vy,
                launch_record.start.vz, AREA_LEVEL_DEFAULT);
            ReqItemDrop(launch);
            return;
        }
        else
        {
            u16 count;

            count = param->count - 1;
            param->count = count;
            /* sll+blez u16-countdown test (cookbook class 3b): true at 0, or on
             * a 0xFFFF wrap from 0. Plain `count == 0` (beqz) does not match. */
            if ((count << 16) <= 0)
            {
                item->mode++;
            }
            UpdateCoordinate(item->locate);
            item->model->locate = item->locate->locate;
            DrawSprite((Sprite3D *)item->model);
            return;
        }
    }

    case NINKEN_MODE_SPAWN:
    {
        s32 create;

        create = 0;
        if (is_humanoid_on_stage_(NINKEN_CHARACTER_PTR) == 0 ||
            GetHumanoid(NINKEN) == 0)
        {
            create = 1;
        }
        if (create != 0)
        {
            NINKEN_CHARACTER_PTR = BreedLife(NINKEN, NINKEN_PARK_POS, NINKEN_PARK_POS,
                                             NINKEN_PARK_POS, 0);
            NINKEN_CHARACTER_PTR->attribute |= ATTR_SUSPEND;
        }

        {
            s32 valid;
            Humanoid *slave;
            VECTOR *position;
            VECTOR *query;
            MapVector *map;
            VECTOR pos;
            VECTOR work;
            MapVector map_result;

            position = &pos;
            query = &work;
            map = &map_result;
            pos.vx = item->locate->locate.coord.t[0];
            pos.vy = item->locate->locate.coord.t[1];
            pos.vz = item->locate->locate.coord.t[2];
            work.vx = position->vx;
            work.vy = position->vy;
            work.vz = position->vz;
            work.vy -= 2000;
            GetAreaMapVector(GlobalAreaMap, map, query, 500, AREA_LEVEL_DEFAULT);

            if (map_result.level >= position->vy - 500)
            {
                if (map_result.level < position->vy)
                {
                    position->vy = map_result.level;
                }
                valid = 1;
            }
            else
            {
                valid = 0;
            }
            if (valid == 0 ||
                (NINKEN_CHARACTER_PTR->attribute & ATTR_SUSPEND) == 0)
            {
                item->mode--;
                param->count = 15; /* retry the spawn shortly */
                return;
            }

            *(SVECTOR *)&work = svec_y_n50[0];
            SetSmoke(&pos, (SVECTOR *)&work, 10, 6);
            SoundEx(&pos, SE_SMOKE_PUFF);
            param->slave = NINKEN_CHARACTER_PTR;
            NINKEN_CHARACTER_PTR->status = STAT_NORMAL;
            slave = param->slave;
            slave->life = slave->lifemax;
            param->slave->model->locate.coord.t[0] = pos.vx;
            param->slave->model->locate.coord.t[1] = pos.vy;
            param->slave->model->locate.coord.t[2] = pos.vz;
            param->slave->model->rotate.vx = item->owner->model->rotate.vx;
            param->slave->model->rotate.vy = item->owner->model->rotate.vy;
            param->slave->model->rotate.vz = item->owner->model->rotate.vz;
            EquipWeapon(param->slave, WEAPON_SHEATHED);
            SetNowMotion(param->slave, MOT_STATE_SHEATHE, MOTION_MOVE_APPLY);
            param->slave->attribute &= ~ATTR_PHASE;
            param->slave->attribute = 0;
            param->slave->target = &item->owner->model->locate;
            param->slave->motion->count = 0;
            PlayMotion(param->slave->motion, 1);
            param->slave->attribute &= ~ATTR_SUSPEND;
            param->slave->model->object[MODEL_PART_WAIST]->attribute |= MODEL_ATTR_COLLIDE;
            set_model_hide_(param->slave, 0);
            param->slave->vector.vy = 0;
            item->mode++;
            param->count = NINKEN_DURATION;
            return;
        }
    }

    case NINKEN_MODE_ACTIVE:
    {
        u16 count;
        Humanoid *slave;

        if (is_humanoid_on_stage_(param->slave) == 0)
        {
            if (item->proc != 0)
            {
                item->mode = ITEM_MODE_DISPOSE;
                item->proc(item);
                DeleteConflict(item->locate);
                if (item->mode != NINKEN_MODE_ROLL)
                {
                    AdtMessageBox(msg_item_dispose_fail, item->type,
                                  (u32)item->mode);
                }
                item->owner = 0;
                item->proc = 0;
            }
        }

        count = param->count - 1;
        param->count = count;
        /* sll+blez u16-countdown test (cookbook class 3b): true at 0, or on
         * a 0xFFFF wrap from 0. Plain `count == 0` (beqz) does not match. */
        if ((count << 16) <= 0)
        {
            goto expire;
        }
        slave = param->slave;
        if (slave->life <= 0)
        {
            goto expire;
        }
        if ((slave->attribute & ATTR_SUSPEND) == 0)
        {
            goto active;
        }

    expire:
    {
        SVECTOR vec;

        vec = svec_y_n50[0];
        SetSmoke(MODEL_POSITION(param->slave->model),
                 &vec, 10, 6);
        SoundEx(MODEL_POSITION(param->slave->model), SE_SMOKE_PUFF);
        TurnAroundAllItems(param->slave);
        {
            void (*dispose_proc)(TItem *);

            dispose_proc = item->proc;
            if (dispose_proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }
    }

    active:
    {
        s32 owner_attribute;
        Humanoid *target;
        character_status status;

        if (GameClock % 15 != 0)
        {
            return;
        }
        status = slave->status;
        if (status == STAT_DAMAGE || status == STAT_STATE || status == STAT_ATTACK)
        {
            return;
        }

        owner_attribute = item->owner->attribute;
        item->owner->attribute = ATTR_SUSPEND;
        target = GetNearestHumanoid(param->slave, 10000);
        item->owner->attribute = owner_attribute;
        if (target != 0)
        {
            if (&target->model->locate == param->slave->target)
            {
                return;
            }
            SetupThinkFunction(param->slave, THINK_MIX_NINKEN);
            param->slave->target = &target->model->locate;
            param->slave->attribute |= PHASE_ALERT;
            EquipWeapon(param->slave, WEAPON_DRAWN);
            SetNowMotion(param->slave, MOT_STATE_DRAW, MOTION_MOVE_APPLY);
            return;
        }
        else
        {
            if (param->slave->target == &item->locate->locate)
            {
                return;
            }
            EquipWeapon(param->slave, WEAPON_SHEATHED);
            SetNowMotion(param->slave, MOT_STATE_SHEATHE, MOTION_MOVE_APPLY);
            param->slave->attribute &= ~ATTR_PHASE;
            SetupThinkFunction(param->slave, THINK_MIX_NONE);
            param->slave->target = &item->owner->model->locate;
            return;
        }
    }
    }
    }
    return;
}
