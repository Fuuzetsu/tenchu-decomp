#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ProcKaginawa(struct tag_TItem *item);
 *     ITEM.C:1026, 67 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s0       struct tag_TItem * item
 *     stack sp+16     int rx
 *     stack sp+20     int ry
 *     reg   $s0       struct tag_TItem * item
 *     reg   $v0       int dist
 *     reg   $s0       struct tag_TItem * item
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct GsSPRITE TargetSprite[1];
 *     extern struct GsOT *OTablePt;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

#include "item.h"

#include <psxsdk/libgs.h>

extern VECTOR vec_z_n20000; /* {0,0,-20000} */

void ProcKaginawa(TItem *item)
{
    void (*item_proc)(TItem *);
    Humanoid *owner;
    VECTOR v;
    VECTOR w;
    s32 rx, ry;
    s32 dist;
    item_mode dispose_mode;

    dispose_mode = ITEM_MODE_DISPOSE;
    if (item->mode == dispose_mode)
    {
        item->mode = ITEM_MODE_START;
        return;
    }
    owner = item->owner;
    if (owner->item[ITEM_N] == 0)
    {
        SetCameraMode(CMODE_DIRECTION);
        item_proc = item->proc;
        if (item_proc == 0)
            return;
        DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
    }
    else if (owner->motion->mid != MOT_KAGI)
    {
        item_proc = item->proc;
        if (item_proc == 0)
            return;
        DISPOSE_ITEM_WITH_MODE(item, dispose_mode);
    }
    else
    {
        GetVectorRotation(CAMERA_VIEWPOINT(&ViewInfo),
                          CAMERA_REFERENCE(&ViewInfo), &rx, &ry);
        if (item->owner->pad.data & PADRup)
        {
            if (rx < 0)
                GsSortSprite(TargetSprite, OTablePt, 0);
            return;
        }
        v = vec_z_n20000;
        RotateVector(&v, rx, ry, 0);
        w.vx = v.vx;
        w.vy = v.vy;
        w.vz = v.vz;
        w.vx += ViewInfo.vpx;
        w.vy += ViewInfo.vpy;
        w.vz += ViewInfo.vpz;
        trace_ground_(CAMERA_VIEWPOINT(&ViewInfo), &w,
                      &CamState.TargetVector, 0);
        v.vx /= 16;
        v.vy /= 16;
        v.vz /= 16;
        CamState.TargetVector.vx += v.vx;
        CamState.TargetVector.vy += v.vy;
        CamState.TargetVector.vz += v.vz;
        dist = GetVectorDistance(MODEL_POSITION(CamState.Owner->model), &CamState.TargetVector);
        if (rx > 0 || dist > 15000)
        {
            CamState.TargetVector = *MODEL_POSITION(CamState.Owner->model);
        }
        SetCameraMode(CMODE_LOCK);
        item->owner->item[ITEM_N] = 0;
        item_proc = item->proc;
        if (item_proc == 0)
            return;
        item->mode = dispose_mode;
        item->proc(item);
        DeleteConflict(item->locate);
        if (item->mode != ITEM_MODE_START)
            AdtMessageBox(msg_item_dispose_fail, item->type, (u32)item->mode);
        item->owner = 0;
        item->proc = 0;
    }
}
