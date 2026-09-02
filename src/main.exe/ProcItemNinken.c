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

/* Retail reuses the completed spawn-query vector for the case-1 smoke
 * direction. The case-2 `vec` is a separate block local in the original
 * source and shares the outer frame slot here. */
typedef union
{
    struct
    {
        PARAM_ITEM_STAY saved;
        u8 pad0[4];
        PARAM_ITEM_STAY rparam;
        u8 pad1[4];
        PARAM_ITEM_LAUNCH launch;
    } drop;
    struct
    {
        VECTOR pos;
        VECTOR work;
        MapVector map;
    } spawn;
    SVECTOR vec;
} ProcItemNinkenScratch;

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
    ProcItemNinkenScratch scratch;

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
            PARAM_ITEM_STAY *saved;
            PARAM_ITEM_LAUNCH *launch;

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
                DISPOSE_ITEM(item);
            }

            saved = &scratch.drop.saved;
            launch = &scratch.drop.launch;
            scratch.drop.launch.type = saved->type;
            launch->user = (Humanoid *)CONFLICT_OWNER_ITEM;
            scratch.drop.launch.start.vx = saved->locate.vx;
            scratch.drop.launch.start.vy = saved->locate.vy;
            scratch.drop.launch.start.vz = saved->locate.vz;
            scratch.drop.launch.end.vx = 0;
            scratch.drop.launch.end.vy = 0;
            scratch.drop.launch.end.vz = 0;
            scratch.drop.launch.start.vy = GetAreaMapLevel(
                GlobalAreaMap, scratch.drop.launch.start.vx,
                scratch.drop.launch.start.vy,
                scratch.drop.launch.start.vz, AREA_LEVEL_DEFAULT);
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
        s32 valid;
        Humanoid *slave;
        VECTOR *position;
        VECTOR *query;
        MapVector *map;

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

        position = &scratch.spawn.pos;
        query = &scratch.spawn.work;
        map = &scratch.spawn.map;
        scratch.spawn.pos.vx = item->locate->locate.coord.t[0];
        scratch.spawn.pos.vy = item->locate->locate.coord.t[1];
        scratch.spawn.pos.vz = item->locate->locate.coord.t[2];
        scratch.spawn.work.vx = position->vx;
        scratch.spawn.work.vy = position->vy;
        scratch.spawn.work.vz = position->vz;
        scratch.spawn.work.vy -= 2000;
        GetAreaMapVector(GlobalAreaMap, map, query, 500, AREA_LEVEL_DEFAULT);

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
            (NINKEN_CHARACTER_PTR->attribute & ATTR_SUSPEND) == 0)
        {
            item->mode--;
            param->count = 15; /* retry the spawn shortly */
            return;
        }

        *(SVECTOR *)&scratch.spawn.work = svec_y_n50[0];
        SetSmoke(&scratch.spawn.pos, (SVECTOR *)&scratch.spawn.work, 10, 6);
        SoundEx(&scratch.spawn.pos, SE_SMOKE_PUFF);
        param->slave = NINKEN_CHARACTER_PTR;
        NINKEN_CHARACTER_PTR->status = STAT_NORMAL;
        slave = param->slave;
        slave->life = slave->lifemax;
        param->slave->model->locate.coord.t[0] = scratch.spawn.pos.vx;
        param->slave->model->locate.coord.t[1] = scratch.spawn.pos.vy;
        param->slave->model->locate.coord.t[2] = scratch.spawn.pos.vz;
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
        scratch.vec = svec_y_n50[0];
        SetSmoke(MODEL_POSITION(param->slave->model),
                 &scratch.vec, 10, 6);
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
