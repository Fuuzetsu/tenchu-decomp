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

/*
 * MakeCameraPosition (0x800309e0) — computes one frame's camera
 * viewpoint and look-at point, then writes the step ViewInfo should
 * take toward them into *vDif. Returns trace_ground_'s reach. Builds
 * the owner's view matrix in the scratchpad: orgrot goes to 0x1f800040
 * with its pitch biased by camera_terrain_pitch_(CamState.Owner) so
 * the camera follows the ground slope, RotMatrixYXZ writes the matrix
 * at 0x1f800080, orgpos becomes its translation, and it is installed
 * as both the rotation and translation matrix. Four SVECTORs from
 * campos are then RotTrans'd through it: [0] and [1] bracket the
 * look-at point, [2] and [3] are the endpoints of the eye ray.
 * trace_ground_ walks that ray, writes the eye position into the local
 * GsRVIEW2 target, and returns how far it got; that reach doubles as a
 * 12-bit fraction interpolating the look-at point from va toward vb,
 * so a blocked trace pulls the reference point in along with the eye.
 * AntiWall then nudges the whole view away from walls. Delivery
 * depends on CamState.snap_pending: when 1 all six components of vDif
 * get the raw target-minus-ViewInfo difference and the flag is cleared
 * (a hard cut); otherwise MakeDifSub eases the reference point and the
 * viewpoint separately through the `ref` and `pnt` scratch blocks.
 */

/*
 * STATUS: MATCHING — pure C, all 660 bytes / 165 instructions exact.
 *
 * TransformCameraPoint's pointer formal keeps the two expanded output
 * addresses independent, so cc1 rematerializes &vc and &vd for the following
 * trace_ground_ call. The late tp alias supplies the target's s0=&target base
 * without extending its lifetime over the earlier calls. Spelling each
 * component delta as -va + vb preserves the target's independent-load order.
 * Retail removed the demo's fourth `SVECTOR *ref` input; its fourth argument
 * is the surviving fifth `GsRVIEW2 *vDif` output, as shown by all six stores.
 */

extern TMakeDifInfo ref;
extern TMakeDifInfo pnt;
extern SVECTOR scratch_rot_1f800040;
extern s32 scratch_trans_1f800094[2];

/* Retail's own prototype drift (def: s32 return) -- byte-required: correcting it changes the caller. */
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

    fwRot = trace_ground_(&vc, &vd, (VECTOR *)&target, 0);

    d1 = -va.vx;
    d1 += vb.vx;
    d1 *= fwRot;
    if (d1 < 0)
        d1 += 0xFFF;
    d2 = (-va.vy + vb.vy) * fwRot;
    target.vrx = (d1 >> 12) + va.vx;
    if (d2 < 0)
        d2 += 0xFFF;
    d3 = (-va.vz + vb.vz) * fwRot;
    target.vry = (d2 >> 12) + va.vy;
    if (d3 < 0)
        d3 += 0xFFF;
    target.vrz = (d3 >> 12) + va.vz;

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
        MakeDifSub((VECTOR *)&ViewInfo.vrx, (VECTOR *)&tp->vrx,
                   (VECTOR *)&vDif->vrx, &ref);
        MakeDifSub((VECTOR *)&ViewInfo, (VECTOR *)tp, (VECTOR *)vDif, &pnt);
    }

    return fwRot;
}
