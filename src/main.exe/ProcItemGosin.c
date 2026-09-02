#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "sound.h"
#include "effect.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcItemGosin(struct tag_TItem *item);
 *     ITEM.C:1804, 52 src lines, frame 88 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s3       struct tag_TItem * item
 *     reg   $s2       struct Humanoid * human
 *     reg   $s0       int itemID
 *     stack sp+24     struct PARAM_ITEM_LAUNCH p
 *     reg   $s3       struct tag_TItem * item
 *     reg   $s3       struct tag_TItem * item
 *     stack sp+24     struct VECTOR v
 * END PSX.SYM */

#include "item.h"

extern VECTOR vec_y_n1200_z_400; /* {0,-1200,400} */

void ProcItemGosin(TItem *item)
{
    enum
    {
        GOSIN_MODE_START = 0,
        GOSIN_MODE_WAIT = 1,
        GOSIN_MODE_ACTIVE = 2
    };
    PARAM_ITEM_LAUNCH drop_request;
    if (item->mode == ITEM_MODE_DISPOSE)
    {
        item->owner->active_item = ACTIVE_ITEM_NONE;
        item->mode = GOSIN_MODE_START;
        return;
    }
    switch (item->mode)
    {
    case GOSIN_MODE_START:
        SetNowMotion(item->owner, MOT_ITEM_KAENGEKI, MOTION_MOVE_APPLY);
        Sound(item->owner, SE_ITEM_USE);
        item->mode++;
        return;

    case GOSIN_MODE_WAIT:
    {
        MotionManager *mot;

        mot = item->owner->motion;
        if (mot->mid != MOT_ITEM_KAENGEKI)
        {
            VECTOR *pos;
            Humanoid *human;
            s32 itemID;

            pos = GetAbsolutePosition(item->locate, 0, 0, 0);
            human = item->owner;
            itemID = item->type;
            memset(&drop_request, 0, sizeof(PARAM_ITEM_LAUNCH));
            drop_request.type = itemID;
            drop_request.user = human;
            drop_request.start.vx = pos->vx;
            drop_request.start.vy = pos->vy;
            drop_request.start.vz = pos->vz;
            drop_request.end.vx = rand() % 200 - 100;
            drop_request.end.vy = rand() % 100 - 200;
            drop_request.end.vz = rand() % 200 - 100;
            ReqItemDrop(&drop_request);
            if (item->proc == 0)
                return;
            DISPOSE_ITEM(item);
            return;
        }
        if (mot->count != 0)
            return;
        if (mot->loop == 0)
            return;
        NowReturnNormal(item->owner);
        SetBleeds(
            GetAbsolutePosition(
                item->owner->model->object[MODEL_PART_TORSO], 0, 0, 0),
            600, 100, 20, 15, RGB24(180, 140, 30));
        item->owner->active_item = item->type;
        item->param.gosin.count = GOSIN_DURATION;
        item->mode++;
        return;
    }

    case GOSIN_MODE_ACTIVE:
    {
        s16 c;

        c = item->param.gosin.count - 1;
        item->param.gosin.count = c;
        if (c == 0)
        {
            if (item->proc == 0)
                return;
            DISPOSE_ITEM(item);
            return;
        }
        if ((c & 0x3f) != 0)
            return;
        *(VECTOR *)&drop_request = vec_y_n1200_z_400;
        set_impact_ex_((VECTOR *)&drop_request, &item->owner->model->locate,
                       FIXED_ONE, 6 * FIXED_ONE, COLOR_GRAY, 0,
                       (s16)(rand() % 360), 2, 120, IMPACT_SPRITE_GOSIN);
        return;
    }
    }
}
