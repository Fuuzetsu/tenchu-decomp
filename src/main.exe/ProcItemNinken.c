#include "common.h"
#include "main.exe.h"
#include "appear.h"
#include "humanoid.h"
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

/* Retail reuses the spawn query slot for the case-1 smoke vector. The inner
 * union makes that lifetime overlap explicit and restores PSX.SYM's `vec`
 * name without casting a VECTOR. The case-2 `vec` is a separate block local
 * in the original source and shares the outer frame slot here. */
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
        union
        {
            VECTOR query;
            SVECTOR vec;
        } work;
        MapVector map;
    } spawn;
    SVECTOR vec;
} ProcItemNinkenScratch;

extern Humanoid *NINKEN_CHARACTER_PTR;
extern SVECTOR svec_y_n50[]; /* {0,-50,0} */

extern void MoveKorogari(TItem *item, param_korogari *param);
extern s32 is_character_state_present_on_stage_(Humanoid *human);
extern void set_model_hide_(Humanoid *human, s16 hide);
extern void SetupThinkFunction(Humanoid *human, TThinkType think);
extern void TurnAroundAllItems(Humanoid *human);

void ProcItemNinken(TItem *item)
{
    param_ninken *param;
    u8 ff;
    s32 water;
    ProcItemNinkenScratch scratch;

    param = &item->param.ninken;
    ff = ITEM_MODE_DISPOSE;
    if (item->mode == ff)
    {
        Humanoid *slave;

        slave = param->slave;
        if (slave != 0)
        {
            NowReturnNormal(slave);
            param->slave->attribute |= ATTR_SUSPEND;
            param->slave->model->locate.coord.t[0] = 999000;
            param->slave->model->locate.coord.t[1] = 999000;
            param->slave->model->locate.coord.t[2] = 999000;
            UpdateCoordinate((ModelType *)param->slave->model);
        }
        item->mode = 0;
        return;
    }

    water = KORO_WATER;
    switch (item->mode)
    {
    case 0:
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
            item->mode = ff;
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
                item->mode = ff;
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
                item->mode++;
            }
            UpdateCoordinate(item->locate);
            item->model->locate = item->locate->locate;
            DrawSprite((Sprite3D *)item->model);
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
            NINKEN_CHARACTER_PTR->attribute |= ATTR_SUSPEND;
        }

        position = &scratch.spawn.pos;
        query = &scratch.spawn.work.query;
        map = &scratch.spawn.map;
        scratch.spawn.pos.vx = item->locate->locate.coord.t[0];
        scratch.spawn.pos.vy = item->locate->locate.coord.t[1];
        scratch.spawn.pos.vz = item->locate->locate.coord.t[2];
        scratch.spawn.work.query.vx = position->vx;
        scratch.spawn.work.query.vy = position->vy;
        scratch.spawn.work.query.vz = position->vz;
        scratch.spawn.work.query.vy -= 2000;
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
            (NINKEN_CHARACTER_PTR->attribute & ATTR_SUSPEND) == 0)
        {
            item->mode--;
            param->count = 15;
            return;
        }

        scratch.spawn.work.vec = svec_y_n50[0];
        SetSmoke(&scratch.spawn.pos, &scratch.spawn.work.vec, 10, 6);
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
        set_model_hide_(param->slave, 0);
        param->slave->vector.vy = 0;
        item->mode++;
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
                    AdtMessageBox(msg_item_dispose_fail, item->type,
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
        if ((slave->attribute & ATTR_SUSPEND) == 0)
        {
            goto active;
        }

    expire:
        scratch.vec = svec_y_n50[0];
        SetSmoke((VECTOR *)param->slave->model->locate.coord.t,
                 &scratch.vec, 10, 6);
        SoundEx((VECTOR *)param->slave->model->locate.coord.t, 0x23);
        TurnAroundAllItems(param->slave);
        {
            void (*dispose_proc)(TItem *);

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
                AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
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
