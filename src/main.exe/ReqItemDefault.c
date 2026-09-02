#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ReqItemDefault(struct Humanoid *user, enum TItemType ItemID);
 *     ITEM.C:3261, 24 src lines, frame 96 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * user
 *     param $a1       enum TItemType ItemID
 *     stack sp+16     struct PARAM_ITEM_LAUNCH param
 *     stack sp+56     struct VECTOR v
 *     stack sp+72     struct VECTOR v0
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

extern VECTOR vec_z_n100[]; /* {0,0,-100} */
extern int ReqItemUse(PARAM_ITEM_LAUNCH *p);

void ReqItemDefault(Humanoid *user, TItemType ItemID)
{
    PARAM_ITEM_LAUNCH param;
    VECTOR v;
    VECTOR v0;
    ModelArchiveType *pm;
    s32 rx;
    s32 ry;
    s32 rz;

    param.type = ItemID;
    param.user = user;
    param.start.vx = user->model->locate.coord.t[0];
    param.start.vy = user->model->locate.coord.t[1] - THROW_HEIGHT;
    param.start.vz = user->model->locate.coord.t[2];
    v = vec_z_n100[0];
    memset(&v0, 0, sizeof(v0));
    pm = param.user->model;
    if (CamState.Owner->model == pm && CamState.Mode == CMODE_DIRECTION)
    {
        GetVectorRotation(CAMERA_VIEWPOINT(&ViewInfo),
                          CAMERA_REFERENCE(&ViewInfo), &rx, &ry);
        rz = 0;
    }
    else
    {
        rx = pm->rotate.vx;
        rz = pm->rotate.vz;
        ry = pm->rotate.vy;
    }
    RotateVector(&v, rx, ry, rz);
    param.end.vx = param.start.vx + v.vx;
    param.end.vy = param.start.vy + v.vy;
    param.end.vz = param.start.vz + v.vz;
    ReqItemUse(&param);
}
