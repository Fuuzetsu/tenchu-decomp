#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcSightShot(struct tag_TItem *item);
 *     ITEM.C:939, 83 src lines, frame 128 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s1       struct tag_TItem * item
 *     reg   $v1       struct param_launch * param
 *     reg   $s1       struct tag_TItem * item
 *     reg   $a1       struct ModelType * model
 *     stack sp+16     struct PARAM_ITEM_LAUNCH param
 *     stack sp+56     struct SVECTOR rot
 *     stack sp+64     int rx
 *     stack sp+68     int ry
 *     stack sp+72     struct PARAM_ITEM_LAUNCH param
 *     reg   $s1       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE TargetSprite[1];
 *     extern struct GsOT *OTablePt;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

#include "item.h"
#include <psxsdk/libgs.h>

extern Humanoid *SearchItemTarget2(Humanoid *owner, SVECTOR *rot,
                                   VECTOR *start, VECTOR *target);
extern int ReqItemLaunch(PARAM_ITEM_LAUNCH *p);

void ProcSightShot(TItem *item)
{
    param_launch *launch;
    s32 dispose_mode;
    PARAM_ITEM_LAUNCH param;
    SVECTOR rot;
    int rx;
    int ry;
    Humanoid *human;

    launch = &item->param.launch;
    dispose_mode = ITEM_MODE_DISPOSE;
    if (item->mode == dispose_mode)
    {
        item->owner->item[ITEM_N] = 0;
        item->mode = ITEM_MODE_START;
        return;
    }

    human = item->owner;
    if (human->item[ITEM_N] == 0)
    {
        u8 item_count;

        item_count = human->item[item->type];
        if (item_count != ITEM_INFINITE)
        {
            human->item[item->type] = item_count + 1;
        }
        SetCameraMode(CMODE_DIRECTION);
        if (item->proc != 0)
        {
            DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
        }
        return;
    }

    if (human->motion->mid != MOT_SYURI)
    {
        VECTOR *pos;
        Humanoid *drop_owner;
        s32 itemID;

        pos = GetAbsolutePosition(item->locate, 0, 0, 0);
        drop_owner = item->owner;
        itemID = item->type;
        memset(&param, 0, sizeof(PARAM_ITEM_LAUNCH));
        param.type = itemID;
        param.user = drop_owner;
        param.start.vx = pos->vx;
        param.start.vy = pos->vy;
        param.start.vz = pos->vz;
        param.end.vx = rand() % 200 - 100;
        param.end.vy = rand() % 100 - 200;
        param.end.vz = rand() % 200 - 100;
        ReqItemDrop(&param);

        if (item->proc != 0)
        {
            DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
        }
        return;
    }

{
    u8 count;
    ModelArchiveType *model;

    count = launch->count;
    if (count != 0)
    {
        launch->count--;
    }
    if ((item->owner->pad.data & PADRup) != 0)
    {
        if (launch->count != 0)
        {
            return;
        }
        SetCameraMode(CMODE_SIGHT);
        GsSortSprite(TargetSprite, OTablePt, 0);
        return;
    }

    count = launch->count;
    model = item->owner->model;
    if (count == 0)
    {
        GsRVIEW2 *view;

        param.type = item->type;
        param.user = item->owner;
        param.start.vx = item->locate->locate.coord.t[0];
        param.start.vy = item->locate->locate.coord.t[1];
        param.start.vz = item->locate->locate.coord.t[2];
        view = &ViewInfo;
        GetVectorRotation(CAMERA_VIEWPOINT(view), CAMERA_REFERENCE(view),
                          &rx, &ry);
        rot.vz = 0;
        rot.vx = rx;
        rot.vy = ry;
        SearchItemTarget2(param.user, &rot, CAMERA_VIEWPOINT(view),
                          &param.end);
        if (item->proc != 0)
        {
            DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
        }
        SetCameraMode(CMODE_LOCK);
    }
    else
    {
        param.type = item->type;
        param.user = item->owner;
        param.start.vx = item->locate->locate.coord.t[0];
        param.start.vy = item->locate->locate.coord.t[1];
        param.start.vz = item->locate->locate.coord.t[2];
        SearchItemTarget2(param.user, &model->rotate, &param.start,
                          &param.end);
        if (item->proc != 0)
        {
            DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
        }
    }
    ReqItemLaunch(&param);
}
}
