#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"
#include "sound.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemShinsoku(struct tag_TItem *item);
 *     ITEM.C:1324, 55 src lines, frame 104 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

extern void spawn_smoke_burst_(VECTOR *pos, u16 spread, s16 divisor, s16 count);

void ProcItemShinsoku(TItem *item)
{
    enum
    {
        SHINSOKU_MODE_START = 0,
        SHINSOKU_MODE_WAIT = 1,
        SHINSOKU_MODE_ACTIVE = 2
    };
    param_shinsoku *param;
    VECTOR pos;

    param = &item->param.shinsoku;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->mode = SHINSOKU_MODE_START;
        return;
    }

    switch (item->mode)
    {
    case SHINSOKU_MODE_START:
        SetNowMotion(item->owner, MOT_ITEM_SHINSOKU, MOTION_MOVE_APPLY);
        Sound(item->owner, SE_ITEM_USE);
        item->mode++;
        return;

    case SHINSOKU_MODE_WAIT:
    {
        MotionManager *motion;

        motion = item->owner->motion;
        if (motion->mid != MOT_ITEM_SHINSOKU)
        {
            VECTOR *pos;
            Humanoid *human;
            s32 itemID;
            s32 rand_x;
            s32 rand_y;
            s32 rand_z;

            pos = GetAbsolutePosition(item->locate, 0, 0, 0);
            human = item->owner;
            itemID = item->type;
            {
                PARAM_ITEM_LAUNCH drop_request;

                ClearItemLaunchRequest(&drop_request);
                drop_request.type = itemID;
                drop_request.user = human;
                drop_request.start.vx = pos->vx;
                drop_request.start.vy = pos->vy;
                drop_request.start.vz = pos->vz;
                rand_x = rand();
                drop_request.end.vx = rand_x % 200 - 100;
                rand_y = rand();
                drop_request.end.vy = rand_y % 100 - 200;
                rand_z = rand();
                drop_request.end.vz = rand_z % 200 - 100;
                ReqItemDrop(&drop_request);
            }
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
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
        spawn_smoke_burst_(item->owner->locate, 150,
                           SMOKE_DRIFT_DIVISOR_DEFAULT, 8);
        param->count = SHINSOKU_DURATION;
        item->mode++;
        return;
    }

    case SHINSOKU_MODE_ACTIVE:
    {
        Humanoid *human;
        ModelArchiveType *model;
        s32 valid;
        u16 buttons;
        s32 rotate;
        VECTOR *apos;

        if (item->owner->motion->mid != MOT_ITEM_SHINSOKU)
        {
            if (item->proc == 0)
            {
                return;
            }
            DISPOSE_ITEM(item);
            return;
        }

        pos.vx = item->owner->model->locate.coord.t[0];
        pos.vy = item->owner->model->locate.coord.t[1];
        pos.vz = item->owner->model->locate.coord.t[2];
        pos.vx += param->vec.vx;
        pos.vy += param->vec.vy;
        pos.vz += param->vec.vz;
        apos = &pos;
        {
            VECTOR query_position;
            MapVector map;

            query_position.vx = apos->vx;
            query_position.vy = apos->vy;
            query_position.vz = apos->vz;
            query_position.vy -= 2000;
            GetAreaMapVector(GlobalAreaMap,
                             &map,
                             &query_position, 500, AREA_LEVEL_DEFAULT);
            if (map.level >= apos->vy - 500)
            {
                if (map.level < apos->vy)
                {
                    apos->vy = map.level;
                }
                valid = 1;
            }
            else
            {
                valid = 0;
            }
            if (valid != 0)
            {
                item->owner->model->locate.coord.t[0] = pos.vx;
                item->owner->model->locate.coord.t[1] = pos.vy;
                item->owner->model->locate.coord.t[2] = pos.vz;
            }

            if ((param->count & 3) == 0)
            {
                query_position = *MODEL_POSITION(item->owner->model);
                query_position.vy -= 300;
                set_impact_ex_(&query_position, 0, 2 * FIXED_ONE,
                               5 * FIXED_ONE, COLOR_GRAY, 0, 0, -30, 0x10,
                               IMPACT_SPRITE_SHINSOKU);
            }
        }
        if (CamState.Owner == item->owner)
        {
            SetCameraMode(CMODE_CROUCH);
        }

        human = item->owner;
        buttons = human->pad.data;
        if ((buttons & PADLright) != 0)
        {
            model = human->model;
            rotate = 0x40;
            model->rotate.vy += rotate;
            RotateVectorS(&param->vec, 0, rotate, 0);
        }
        else if ((buttons & PADLleft) != 0)
        {
            model = human->model;
            rotate = -0x40;
            model->rotate.vy += rotate;
            RotateVectorS(&param->vec, 0, rotate, 0);
        }

        param->count--;
        if (param->count != 0 && (item->owner->pad.trig & (PADRleft | PADRdown | PADRright | PADRup)) == 0)
        {
            return;
        }
        NowReturnNormal(item->owner);
        SetCameraMode(CMODE_NORMAL);
        if (item->proc == 0)
        {
            return;
        }
        DISPOSE_ITEM(item);
        return;
    }
    }
}
