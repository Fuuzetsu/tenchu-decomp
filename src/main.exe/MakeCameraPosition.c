#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void MakeCameraPosition(struct VECTOR *orgpos, struct SVECTOR *orgrot, struct SVECTOR *campos, struct SVECTOR *ref, struct GsRVIEW2 *vDif);
 *     CAMERA.C:533, 66 src lines, frame 104 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s4       struct VECTOR * orgpos
 *     param $s5       struct SVECTOR * orgrot
 *     param $s7       struct SVECTOR * campos
 *     param $fp       struct SVECTOR * ref
 *     param stack+16  struct GsRVIEW2 * vDif
 *     reg   $v1       int fwRot
 *     reg   $s2       struct SVECTOR * tmp
 *     reg   $s6       struct SVECTOR * prot
 *     reg   $s1       struct MATRIX * pmat
 *     reg   $s4       struct VECTOR * pos
 *     reg   $s5       struct SVECTOR * rot
 *     reg   $a0       int d
 *     reg   $v1       int ret
 *     reg   $s0       int dz
 *     reg   $s3       int dx
 *     reg   $v0       int y2
 *     reg   $s0       int y1
 *     stack sp+24     struct SVECTOR dvec
 *     stack sp+32     struct VECTOR ret
 *     stack sp+48     struct VECTOR start
 *     reg   $a2       int N
 *     reg   $s4       struct VECTOR * start
 *     reg   $a0       struct SVECTOR * v
 *     reg   $s1       int n
 *     reg   $s4       int pz
 *     reg   $s2       int py
 *     reg   $s3       int px
 *     reg   $s5       int vz
 *     reg   $s6       int vy
 *     reg   $s7       int vx
 *     reg   $s0       int i
 *     stack sp+120    struct GsRVIEW2 * vdif
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

extern TMakeDifInfo ref;
extern TMakeDifInfo pnt;
extern SVECTOR scratch_rot_1f800040;
extern s32 scratch_trans_1f800094[2];

/* Retail declares this s16 here although the definition returns s32. */
extern short camera_terrain_pitch_(Humanoid *h);
extern void AntiWall(GsRVIEW2 *vinfo, GsRVIEW2 *target);
extern void MakeDifSub(VECTOR *src, VECTOR *target, VECTOR *dest, TMakeDifInfo *info);

static inline void TransformCameraPoint(SVECTOR *point, VECTOR *result,
                                        long *flag)
{
    RotTrans(point, result, flag);
}

s32 MakeCameraPosition(VECTOR *orgpos, SVECTOR *orgrot, SVECTOR *campos, GsRVIEW2 *vDif)
{
    GsRVIEW2 target;
    GsRVIEW2 *tp;
    VECTOR va, vb, vc, vd;
    long flag[2];
    TCameraStatus *cs;
    s32 fwRot;
    s32 d1, d2, d3;

    cs = &CamState;
    scratch_rot_1f800040.vx = orgrot->vx + camera_terrain_pitch_(cs->Owner);
    scratch_rot_1f800040.vy = orgrot->vy;
    scratch_rot_1f800040.vz = orgrot->vz;
    RotMatrixYXZ((SVECTOR *)TENCHU_SCRATCHPAD(0x40),
                 (MATRIX *)TENCHU_SCRATCHPAD(0x80));
    scratch_trans_1f800094[0] = orgpos->vx;
    scratch_trans_1f800094[1] = orgpos->vy;
    scratch_trans_1f800094[2] = orgpos->vz;
    SetRotMatrix((MATRIX *)TENCHU_SCRATCHPAD(0x80));
    SetTransMatrix((MATRIX *)TENCHU_SCRATCHPAD(0x80));

    RotTrans(campos, &va, flag);
    RotTrans(campos + 1, &vb, flag);
    TransformCameraPoint(campos + 2, &vc, flag);
    TransformCameraPoint(campos + 3, &vd, flag);

    fwRot = trace_ground_(&vc, &vd, CAMERA_VIEWPOINT(&target), 0);

    d1 = (-va.vx + vb.vx) * fwRot;
    if (d1 < 0)
        d1 += FIXED_TRUNC_BIAS;
    d2 = (-va.vy + vb.vy) * fwRot;
    target.vrx = (d1 >> FIXED_SHIFT) + va.vx;
    if (d2 < 0)
        d2 += FIXED_TRUNC_BIAS;
    d3 = (-va.vz + vb.vz) * fwRot;
    target.vry = (d2 >> FIXED_SHIFT) + va.vy;
    target.vrz = d3 / FIXED_ONE + va.vz;

    AntiWall(&ViewInfo, &target);
    tp = &target;

    if (cs->snap_pending == 1)
    {
        vDif->vpx = tp->vpx - ViewInfo.vpx;
        vDif->vpy = tp->vpy - ViewInfo.vpy;
        vDif->vpz = tp->vpz - ViewInfo.vpz;
        vDif->vrx = tp->vrx - ViewInfo.vrx;
        vDif->vry = tp->vry - ViewInfo.vry;
        vDif->vrz = tp->vrz - ViewInfo.vrz;
        cs->snap_pending = 0;
    }
    else
    {
        MakeDifSub(CAMERA_REFERENCE(&ViewInfo), CAMERA_REFERENCE(tp),
                   CAMERA_REFERENCE(vDif), &ref);
        MakeDifSub(CAMERA_VIEWPOINT(&ViewInfo), CAMERA_VIEWPOINT(tp),
                   CAMERA_VIEWPOINT(vDif), &pnt);
    }

    return fwRot;
}
