#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SnapCameraTargetVector(void);
 *     CAMERA.C:106, 30 src lines, frame 72 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct VECTOR v
 *     stack sp+32     struct SVECTOR sv
 *     stack sp+40     struct SVECTOR sv2
 *     reg   $a0       struct VECTOR * target
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

void SnapCameraTargetVector(void)
{
    enum
    {
        VSHIFT = 5
    };
    VECTOR v;
    SVECTOR sv;
    SVECTOR sv2;
    VECTOR *target;
    s32 t1, t2, t3;

    memset(&sv, 0, sizeof(sv) + sizeof(sv2));
    ((VECTOR *)&sv)->vx = ViewInfo.vpx;
    ((VECTOR *)&sv)->vy = ViewInfo.vpy;
    ((VECTOR *)&sv)->vz = ViewInfo.vpz;
    v = *(VECTOR *)&sv;

    memset(&sv2, 0, sizeof(sv2));
    sv2.vx = (s16)ViewInfo.vrx - (s16)ViewInfo.vpx;
    sv2.vy = (s16)ViewInfo.vry - (s16)ViewInfo.vpy;
    sv2.vz = (s16)ViewInfo.vrz - (s16)ViewInfo.vpz;
    sv = sv2;
    VectorNormalSS(&sv, &sv2);

    t1 = sv2.vx;
    sv2.vx = (s16)(t1 / (1 << VSHIFT));
    t2 = sv2.vy;
    sv2.vy = (s16)(t2 / (1 << VSHIFT));
    t3 = sv2.vz;
    sv2.vz = (s16)(t3 / (1 << VSHIFT));

    target = GetAreaMapPassage(GlobalAreaMap, &v, &sv2, -1);
    if (target != 0)
    {
        CamState.TargetVector.vx = target->vx;
        CamState.TargetVector.vy = target->vy;
        CamState.TargetVector.vz = target->vz;
    }
    else
    {
        CamState.TargetVector.vx = CamState.Owner->model->locate.coord.t[0];
        CamState.TargetVector.vy = CamState.Owner->model->locate.coord.t[1];
        CamState.TargetVector.vz = CamState.Owner->model->locate.coord.t[2];
    }
}
