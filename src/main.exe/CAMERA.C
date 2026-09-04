#include "common.h"
#include "main.exe.h"
#include "effect.h"
#include "item.h"
#include "padcmd.h"
#include <psxsdk/libgpu.h>

/*
 * Retail adds four camera helpers and no longer contains the demo's
 * QuakeCamera. The translation-unit manifest retains both builds' orders.
 */

extern SVECTOR WallProbeL;
extern SVECTOR WallProbeR;
extern SVECTOR CamVecL[];
extern SVECTOR CamVecR[];
extern TCameraPos CamPosStickL;
extern TCameraPos CamPosStickR;
extern TCameraPos CamPosPeepL;
extern TCameraPos CamPosPeepR;
extern TCameraPos CamPosCrouch;
extern TCameraPos CamPosRun;
extern TCameraPos CamPosSwim;
extern TCameraPos CamPosKnockback;
extern TCameraPos CamPosKnockbackAlt;
extern TCameraPos CamPosHang;
extern TMakeDifInfo ref;
extern TMakeDifInfo pnt;

extern u16 DEBUG_PAD_HELD_;
extern u16 DEBUG_PAD_PRESS_;
extern s16 DEBUG_CAMERA_INDEX_;
extern TCameraPos *DEBUG_CAMERA_BASE_;
extern SVECTOR *DEBUG_CAMERA_SLOTS_[N_DEBUG_CAMERA_SLOTS];
extern char *DEBUG_CAMERA_LABELS_[N_DEBUG_CAMERA_SLOTS];
extern s32 Projection;

extern char str_mark_l[];     /* (L) */
extern char str_mark_r[];     /* (R) */
extern char str_mark_alert[]; /* (!) */
extern char fmt_camera_edit[];
extern char fmt_owner_r[];    /* OWNER: (%d, %d, %d) R:%d */
extern char str_newline[];    /* "\n" */

/* Views into the rotation and translation parts of a scratchpad matrix. */
extern SVECTOR scratch_rot_1f800040;
extern s32 scratch_trans_1f800094[2];
extern s32 scratch_trans_z_1f80009c;


void CameraDirection(Humanoid *pl, GsRVIEW2 *vDif);

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
    VECTOR v = {
        .vx = ViewInfo.vpx,
        .vy = ViewInfo.vpy,
        .vz = ViewInfo.vpz
    };
    SVECTOR sv = {
        .vx = (s16)ViewInfo.vrx - (s16)ViewInfo.vpx,
        .vy = (s16)ViewInfo.vry - (s16)ViewInfo.vpy,
        .vz = (s16)ViewInfo.vrz - (s16)ViewInfo.vpz
    };
    SVECTOR sv2;
    VECTOR *target;
    s32 t1, t2, t3;

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

s32 camera_terrain_pitch_(Humanoid *human)
{
    enum
    {
        DIV = 16,
        ROTMAX = 341
    };
    AreaNodeType *node;
    s16 x;
    s16 z;
    s16 xspan;
    s16 zspan;
    s32 yy;
    s32 height0;
    s32 height1;
    s32 xshift;
    s32 zshift;
    s32 delta;
    s32 angle;

    if (human->map.area == 0)
        return 0;

    xshift = -rsin(human->rotate->vy) / DIV;
    zshift = -rcos(human->rotate->vy) / DIV;

    x = human->locate->vx / 10;
    z = human->locate->vz / 10;
    node = human->map.area;
    yy = (u16)node->y;
    x -= (u16)node->x1;
    xspan = (u16)node->x2 - (u16)node->x1 + 1;
    z -= (u16)node->z1;
    zspan = (u16)node->z2 - (u16)node->z1 + 1;

    switch (node->attribute & (MAP_SLOPE_X | MAP_SLOPE_Z))
    {
    case MAP_SLOPE_X:
        yy += x * node->dy / xspan;
        break;
    case MAP_SLOPE_Z:
        yy += z * node->dy / zspan;
        break;
    }
    height0 = (short)yy * 10;
    if (height0 == LEVEL_NONE)
        return 0;

    x = (human->locate->vx + xshift) / 10;
    z = (human->locate->vz + zshift) / 10;
    node = human->map.area;
    yy = (u16)node->y;
    x -= (u16)node->x1;
    xspan = (u16)node->x2 - (u16)node->x1 + 1;
    z -= (u16)node->z1;
    zspan = (u16)node->z2 - (u16)node->z1 + 1;

    switch (node->attribute & (MAP_SLOPE_X | MAP_SLOPE_Z))
    {
    case MAP_SLOPE_X:
        yy += x * node->dy / xspan;
        break;
    case MAP_SLOPE_Z:
        yy += z * node->dy / zspan;
        break;
    }
    height1 = (short)yy * 10;
    if (height1 == LEVEL_NONE)
        return 0;

    delta = height1 - height0;
    if (delta <= -700)
        return 0;

    angle = ratan2(delta, 256);
    if (angle > ROTMAX)
        angle = ROTMAX;
    else if (angle < -ROTMAX)
        angle = -ROTMAX;
    return angle;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void MakeDifSub(struct VECTOR *src, struct VECTOR *target, struct VECTOR *dest, struct TMakeDifInfo *info);
 *     CAMERA.C:377, 40 src lines, frame 56 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $t0       struct VECTOR * src
 *     param $a1       struct VECTOR * target
 *     param $s5       struct VECTOR * dest
 *     param $s4       struct TMakeDifInfo * info
 *     reg   $s2       int len
 *     reg   $s6       int mspd
 *     reg   $s0       long theta
 *     reg   $s0       long dx
 *     reg   $s3       long dy
 *     reg   $s1       long dz
 *     stack sp+16     struct SVECTOR nv
 *     reg   $v1       struct SVECTOR * a
 *     reg   $s0       struct SVECTOR * b
 *     reg   $s0       long slab
 *     reg   $s0       long sla
 *     reg   $s1       long ip
 * END PSX.SYM */

static void MakeDifSub(VECTOR *src, VECTOR *target, VECTOR *dest,
                       TMakeDifInfo *info)
{
    s32 dx, dy, dz;
    s32 len;
    SVECTOR nv;
    s32 theta;
    s32 ip;
    s32 lenA, lenB;
    s32 slab;
    s32 sla;
    s32 spd;
    s32 t;
    s32 mspd;

    dx = target->vx - src->vx;
    dy = target->vy - src->vy;
    dz = target->vz - src->vz;
    len = GetVectorLength(dx, dy, dz);
    if (len == 0)
    {
        setVector(dest, 0, 0, 0);
        return;
    }

    ip = len >> info->div;

    nv.vx = dx;
    nv.vy = dy;
    nv.vz = dz;

    {
        SVECTOR *a = &nv;
        SVECTOR *b = &info->bef;

        theta = (s16)dx * info->bef.vx + a->vy * b->vy + a->vz * b->vz;
        lenA = GetVectorLength((s16)dx, a->vy, a->vz);
        lenB = GetVectorLength(info->bef.vx, b->vy, b->vz);
    }
    slab = lenA * lenB;

    if (theta >= 0x7FFFF)
    {
        t = theta;
        if (theta < 0)
        {
            t = theta + FIXED_TRUNC_BIAS;
        }
        theta = t >> FIXED_SHIFT;
        t = slab;
        if (slab < 0)
        {
            t = slab + FIXED_TRUNC_BIAS;
        }
        slab = t >> FIXED_SHIFT;
    }

    if (slab == 0)
    {
        sla = 0;
    }
    else
    {
        sla = (theta << FIXED_SHIFT) / slab;
    }

    mspd = info->spd * (sla + FIXED_ONE);
    if (mspd < 0)
    {
        mspd += 2 * FIXED_ONE - 1;
    }
    spd = mspd >> (FIXED_SHIFT + 1);
    spd += info->ac;
    if (ip < spd)
    {
        spd = ip;
    }

    info->spd = (s16)spd;
    setVector(dest, (nv.vx * spd) / len, (nv.vy * spd) / len, (nv.vz * spd) / len);
    info->bef.vx = nv.vx;
    info->bef.vy = nv.vy;
    info->bef.vz = nv.vz;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AntiWall(struct GsRVIEW2 *vinfo, struct GsRVIEW2 *target);
 *     CAMERA.C:432, 91 src lines, frame 88 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct GsRVIEW2 * vinfo
 *     param $s2       struct GsRVIEW2 * target
 *     stack sp+24     struct SVECTOR vsL
 *     stack sp+32     struct SVECTOR vsR
 *     reg   $s3       int lvR
 *     reg   $s0       int rmap
 *     stack sp+40     struct SVECTOR av
 *     stack sp+48     int rx
 *     stack sp+52     int ry
 *     reg   $s5       int sx
 *     reg   $s3       int sy
 *     reg   $s0       int sz
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

void AntiWall(GsRVIEW2 *vinfo, GsRVIEW2 *target)
{
    VECTOR vsL;
    VECTOR vsR;
    int lvR;
    enum camera_probe_mask rmap;
    VECTOR av;
    int rx;
    int ry;
    int sx;
    int sy;
    int sz;

    GetVectorRotation(CAMERA_VIEWPOINT(target), CAMERA_REFERENCE(target),
                      &rx, &ry);
    ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vz = 0;
    ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vx = rx;
    ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vy = ry;
    RotMatrixYXZ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS,
                 (MATRIX *)TENCHU_SCRATCHPAD(0x40));
    SetRotMatrix((MATRIX *)TENCHU_SCRATCHPAD(0x40));

    ApplyRotMatrix(&WallProbeL, &vsL);
    ApplyRotMatrix(&WallProbeR, &vsR);

    lvR = GetAreaMapLevel(GlobalAreaMap,
                          target->vpx + vsR.vx,
                          target->vpy + vsR.vy,
                          target->vpz + vsR.vz, AREA_LEVEL_DEFAULT);
    rmap = CAMERA_PROBE_CLEAR;
    if (GetAreaMapLevel(GlobalAreaMap,
                        target->vpx + vsL.vx,
                        target->vpy + vsL.vy,
                        target->vpz + vsL.vz, AREA_LEVEL_DEFAULT) <= target->vpy)
    {
        rmap = CAMERA_PROBE_FRONT_LEFT;
        FntPrint(str_mark_l);
    }
    if (lvR <= target->vpy)
    {
        rmap |= CAMERA_PROBE_FRONT_RIGHT;
        FntPrint(str_mark_r);
    }

    rmap &= CAMERA_PROBE_FRONT_MASK;
    if (rmap != CAMERA_PROBE_CLEAR)
    {
        av.vx = 0;
        av.vy = 0;
        av.vz = 0;
        if (rmap == CAMERA_PROBE_FRONT_LEFT)
        {
            ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vx = WALL_AVOID_PUSH;
            ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vy = 0;
            ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vz = 0;
            ApplyRotMatrix((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS, &av);
        }
        if (rmap == CAMERA_PROBE_FRONT_RIGHT)
        {
            ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vx = -WALL_AVOID_PUSH;
            ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vy = 0;
            ((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS)->vz = 0;
            ApplyRotMatrix((SVECTOR *)TENCHU_SCRATCHPAD_ADDRESS, &av);
        }

        sx = av.vx / 2;
        sy = av.vy / 2;
        sz = av.vz / 2;
        if (GetAreaMapLevel(GlobalAreaMap,
                            target->vpx + av.vx + sx,
                            target->vpy + av.vy + sy,
                            target->vpz + av.vz + sz,
                            AREA_LEVEL_DEFAULT) <= target->vpy)
        {
            av.vx = sx / 2;
            av.vy = sy / 2;
            av.vz = sz / 2;
            FntPrint(str_mark_alert);
        }
        target->vpx += av.vx;
        target->vpy += av.vy;
        target->vpz += av.vz;
    }
}

void push_from_walls_(VECTOR *pos, s32 amount)
{
    MapVector v1;
    MapVector v2;
    s32 half;
    u32 vec;
    u16 xAdj;
    s32 signedX;
    u16 zAdj;

    GetAreaMapVector(GlobalAreaMap, &v1, pos, amount, AREA_LEVEL_DEFAULT);
    if (v1.level == LEVEL_NONE)
    {
        return;
    }
    if (v1.vector == 0)
    {
        return;
    }
    half = amount / 2;
    GetAreaMapVector(GlobalAreaMap, &v2, pos, half, AREA_LEVEL_DEFAULT);
    vec = v2.vector;
    if (vec == 0)
    {
        amount = half;
        vec = v1.vector;
    }
    xAdj = RefrectMove[vec][0];
    zAdj = RefrectMove[vec][1];
    if (xAdj != 0)
    {
        signedX = (s16)xAdj;
    }
    else
    {
        signedX = (s16)xAdj;
    }
    if (signedX > 0)
    {
        pos->vx += amount;
    }
    else if (signedX < 0)
    {
        pos->vx -= amount;
    }
    if ((s16)zAdj > 0)
    {
        pos->vz += amount;
    }
    else if ((s16)zAdj < 0)
    {
        pos->vz -= amount;
    }
}

void debug_output_edit_camera_settings(s16 pad)
{
    enum
    {
        CAMERA_EDIT_STEP = 50
    };
    SVECTOR *camera;
    char *format;
    s32 marker;
    s32 i;

    DEBUG_PAD_PRESS_ = DEBUG_PAD_HELD_;
    DEBUG_PAD_HELD_ = pad;
    DEBUG_PAD_PRESS_ = DEBUG_PAD_HELD_ & (DEBUG_PAD_HELD_ ^ DEBUG_PAD_PRESS_);

    if (DEBUG_PAD_PRESS_ & PADL1)
    {
        DEBUG_CAMERA_INDEX_++;
        if (DEBUG_CAMERA_INDEX_ >= N_DEBUG_CAMERA_SLOTS)
        {
            DEBUG_CAMERA_INDEX_ = 0;
        }
    }

    camera = DEBUG_CAMERA_SLOTS_[DEBUG_CAMERA_INDEX_];
    if (DEBUG_PAD_HELD_ & PADLup)
    {
        camera->vz -= CAMERA_EDIT_STEP;
    }
    if (DEBUG_PAD_HELD_ & PADLdown)
    {
        camera->vz += CAMERA_EDIT_STEP;
    }
    if (DEBUG_PAD_HELD_ & PADRup)
    {
        camera->vy -= CAMERA_EDIT_STEP;
    }
    if (DEBUG_PAD_HELD_ & PADRdown)
    {
        camera->vy += CAMERA_EDIT_STEP;
    }
    if (DEBUG_PAD_HELD_ & PADRleft)
    {
        camera->vx -= CAMERA_EDIT_STEP;
    }
    if (DEBUG_PAD_HELD_ & PADRright)
    {
        camera->vx += CAMERA_EDIT_STEP;
    }

    if ((DEBUG_PAD_HELD_ & (PADL2 | PADR2)) == (PADL2 | PADR2))
    {
        *DEBUG_CAMERA_BASE_ = CamPosDefault;
    }

    i = 0;
    format = fmt_camera_edit;
    while (i < N_DEBUG_CAMERA_SLOTS)
    {
        marker = ' ';
        if (DEBUG_CAMERA_INDEX_ == i)
        {
            marker = '*';
        }
        FntPrint(format, marker, DEBUG_CAMERA_LABELS_[i],
                 DEBUG_CAMERA_SLOTS_[i]->vx, DEBUG_CAMERA_SLOTS_[i]->vy,
                 DEBUG_CAMERA_SLOTS_[i]->vz);
        i++;
    }
}

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

static inline void TransformCameraPoint(SVECTOR *point, VECTOR *result,
                                        long *flag)
{
    RotTrans(point, result, flag);
}

static s32 MakeCameraPosition(VECTOR *orgpos, SVECTOR *orgrot,
                              SVECTOR *campos, GsRVIEW2 *vDif)
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
    scratch_trans_z_1f80009c = orgpos->vz;
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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void CameraType1(struct Humanoid *pl, struct GsRVIEW2 *vDif);
 *     CAMERA.C:603, 176 src lines, frame 200 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $s5       struct Humanoid * pl
 *     param $s3       struct GsRVIEW2 * vDif
 *     reg   $s2       struct ModelArchiveType * mad
 *     stack sp+24     struct VECTOR pos
 *     stack sp+40     struct SVECTOR vecl
 *     stack sp+48     struct SVECTOR vecr
 *     reg   $s0       int levfl
 *     reg   $s1       int levfr
 *     reg   $v0       int levbl
 *     reg   $s2       int levbr
 *     reg   $s0       int levmap
 *     stack sp+56     struct SVECTOR campos
 *     stack sp+64     struct SVECTOR ref
 *     stack sp+72     struct SVECTOR campos
 *     stack sp+80     struct SVECTOR ref
 *     stack sp+88     struct SVECTOR campos
 *     stack sp+96     struct SVECTOR ref
 *     stack sp+104    struct SVECTOR campos
 *     stack sp+112    struct SVECTOR ref
 *     stack sp+120    struct SVECTOR campos
 *     stack sp+128    struct SVECTOR ref
 *     stack sp+136    struct SVECTOR campos
 *     stack sp+144    struct SVECTOR ref
 *     stack sp+152    struct SVECTOR campos
 *     stack sp+160    struct SVECTOR ref
 *     stack sp+56     struct SVECTOR campos
 *     stack sp+64     struct SVECTOR ref
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct TCameraPos CamPosCriticalHit[3];
 * END PSX.SYM */

typedef union
{
    VECTOR init;
    TCameraPos camera;
    struct
    {
        SVECTOR vecl;
        SVECTOR vecr;
        TCameraPos stick_l_camera;
    } probe;
} CameraScratch;

/* the CAMERA.C statics, named by the CMODE (or trigger) each one serves;
 * CamPosKnockback fires on mids 0x1005..0x1009/0x100C, CamPosHang on
 * status 7. CamVecL/R seed the probe's vecl/vecr. */
void CameraType1(Humanoid *pl, GsRVIEW2 *vDif)
{
    ModelArchiveType *mad;
    VECTOR pos;
    CameraScratch scratch;
    TCameraPos alternate;
    TCameraStatus *cs;

    mad = pl->model;
    memset(&scratch.init, 0, sizeof(scratch.init));
    scratch.init.vx = pl->model->locate.coord.t[0];
    scratch.init.vy = pl->model->locate.coord.t[1] - CAMERA_EYE_HEIGHT;
    scratch.init.vz = pl->model->locate.coord.t[2];
    pos = scratch.init;
    mad->attribute |= MODEL_ATTR_NOCULL;

    switch (CamState.Owner->status)
    {
    case STAT_STICKON:
    {
        s32 levfl;
        s32 levfr;
        s32 levbl;
        s32 levbr;
        enum camera_probe_mask levmap;

        scratch.probe.vecl = CamVecL[0];
        scratch.probe.vecr = CamVecR[0];
        RotateVectorS(&scratch.probe.vecl,
                      mad->rotate.vx, mad->rotate.vy, 0);
        RotateVectorS(&scratch.probe.vecr,
                      mad->rotate.vx, mad->rotate.vy, 0);
        levfl = GetAreaMapLevel(GlobalAreaMap,
                                mad->locate.coord.t[0] + scratch.probe.vecl.vx,
                                mad->locate.coord.t[1],
                                mad->locate.coord.t[2] + scratch.probe.vecl.vz,
                                AREA_LEVEL_STEP_DOWN);
        levfr = GetAreaMapLevel(GlobalAreaMap,
                                mad->locate.coord.t[0] + scratch.probe.vecr.vx,
                                mad->locate.coord.t[1],
                                mad->locate.coord.t[2] + scratch.probe.vecr.vz,
                                AREA_LEVEL_STEP_DOWN);
        levbr = GetAreaMapLevel(GlobalAreaMap,
                                mad->locate.coord.t[0] - scratch.probe.vecl.vx,
                                mad->locate.coord.t[1],
                                mad->locate.coord.t[2] - scratch.probe.vecl.vz,
                                AREA_LEVEL_STEP_DOWN);
        levbl = GetAreaMapLevel(GlobalAreaMap,
                                mad->locate.coord.t[0] - scratch.probe.vecr.vx,
                                mad->locate.coord.t[1],
                                mad->locate.coord.t[2] - scratch.probe.vecr.vz,
                                AREA_LEVEL_STEP_DOWN);

        levmap = levfl == LEVEL_NONE ? CAMERA_PROBE_FRONT_LEFT
                                     : CAMERA_PROBE_CLEAR;
        if (levfr == LEVEL_NONE)
            levmap |= CAMERA_PROBE_FRONT_RIGHT;
        if (levbl == LEVEL_NONE)
            levmap |= CAMERA_PROBE_BACK_LEFT;
        if (levbr == LEVEL_NONE)
            levmap |= CAMERA_PROBE_BACK_RIGHT;

        if ((levmap & CAMERA_PROBE_ALL_MASK) == CAMERA_PROBE_BACK_LEFT)
        {
            CamState.Mode = CMODE_PEEP_R;
            break;
        }
        else if ((levmap & CAMERA_PROBE_ALL_MASK) == CAMERA_PROBE_BACK_RIGHT)
        {
            CamState.Mode = CMODE_PEEP_L;
            break;
        }
        else if ((levmap & CAMERA_PROBE_FRONT_MASK) ==
                 CAMERA_PROBE_FRONT_LEFT)
        {
            CamState.Mode = CMODE_STICK_R;
            break;
        }
        else if ((levmap & CAMERA_PROBE_FRONT_MASK) ==
                 CAMERA_PROBE_FRONT_RIGHT)
        {
            CamState.Mode = CMODE_STICK_L;
            break;
        }
        else
        {
            CamState.Mode = CMODE_NORMAL;
            break;
        }
    }
    case STAT_SQUAT:
        CamState.Mode = CMODE_CROUCH;
        break;
    case STAT_SWIM:
        CamState.Mode = CMODE_SWIM;
        break;
    case STAT_HANG:
        CamState.Mode = CMODE_HANG;
        break;
    case STAT_CHASE:
        cs = &CamState;
        if (cs->Owner->motion->mid != MOT_CHASE)
            break;
        cs->Mode = CMODE_RUN;
        break;
    case STAT_STATE:
        cs = &CamState;
        if (cs->Owner->motion->mid != MOT_STATE_CLIMB)
            break;
        cs->Mode = CMODE_RUN;
        break;
    case STAT_DAMAGE:
    {
        u16 mid;

        cs = &CamState;
        mid = cs->Owner->motion->mid;
        if ((u16)(mid - MOT_DAMAGE_LAUNCH_BACK) <=
            MOT_DAMAGE_DOWNED - MOT_DAMAGE_LAUNCH_BACK)
        {
            cs->Mode = CMODE_KNOCKBACK;
            break;
        }
        if ((s16)mid != MOT_DAMAGE_GETUP)
            break;
        cs->Mode = CMODE_KNOCKBACK;
        break;
    }
    default:
        break;
    }

    switch (CamState.Mode)
    {
    case CMODE_CRITICAL_HIT:
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &CamPosCriticalHit[CamState.OldMode].r1, vDif);
        return;
    case CMODE_STICK_L:
        scratch.probe.stick_l_camera = CamPosStickL;
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &scratch.probe.stick_l_camera.r1, vDif);
        return;
    case CMODE_STICK_R:
        scratch.camera = CamPosStickR;
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &scratch.camera.r1, vDif);
        return;
    case CMODE_PEEP_L:
        scratch.camera = CamPosPeepL;
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &scratch.camera.r1, vDif);
        return;
    case CMODE_PEEP_R:
        scratch.camera = CamPosPeepR;
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &scratch.camera.r1, vDif);
        return;
    case CMODE_CROUCH:
        scratch.camera = CamPosCrouch;
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &scratch.camera.r1, vDif);
        CamState.Mode = CMODE_NORMAL;
        return;
    case CMODE_RUN:
        scratch.camera = CamPosRun;
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &scratch.camera.r1, vDif);
        CamState.Mode = CMODE_NORMAL;
        return;
    case CMODE_SWIM:
        scratch.camera = CamPosSwim;
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &scratch.camera.r1, vDif);
        CamState.Mode = CMODE_NORMAL;
        return;
    case CMODE_KNOCKBACK:
        scratch.camera = CamPosKnockback;
        alternate = CamPosKnockbackAlt;

        if (MakeCameraPosition(&pos, &pl->model->rotate,
                               &scratch.camera.r1, vDif) <= ANGLE_HALF)
        {
            MakeCameraPosition(&pos, &pl->model->rotate,
                               &alternate.r1, vDif);
        }
        CamState.Mode = CMODE_NORMAL;
        return;
    case CMODE_HANG:
        scratch.camera = CamPosHang;
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &scratch.camera.r1, vDif);
        CamState.Mode = CMODE_NORMAL;
        return;
    default:
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &CamPos.r1, vDif);
        return;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetCameraMode(enum TCameraMode mode);
 *     CAMERA.C:85, 9 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       enum TCameraMode mode
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct TCameraPos CamPosCriticalHit[3];
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

void SetCameraMode(TCameraMode mode)
{
    VECTOR va;
    VECTOR vb;
    VECTOR vc;
    VECTOR vd;
    long flag;
    s32 rx;
    s32 ry;
    TCameraStatus *cs;
    TCameraPos *tbl;
    long *fp;
    VECTOR *pos;
    SVECTOR *rot;
    TCameraPos *camera;
    VECTOR *pv;
    s32 i;
    s32 hitf;

    switch (mode)
    {
    case CMODE_CRITICAL_HIT:
        CamState.OldMode = rand() % N_CRITICAL_CAMERA_POSITIONS;
        i = 0;
        cs = &CamState;
        tbl = CamPosCriticalHit;
        fp = &flag;
        for (;;)
        {
            if (i < N_CRITICAL_CAMERA_POSITIONS)
            {
                cs->OldMode++;
                if (cs->OldMode > MaxCriticalValiation)
                    cs->OldMode = 0;
                camera = (TCameraPos *)(cs->OldMode * sizeof(*tbl) + (s32)tbl);
                pos = cs->Owner->locate;
                rot = cs->Owner->rotate;
                {
                    scratch_rot_1f800040.vx = rot->vx + camera_terrain_pitch_(cs->Owner);
                    scratch_rot_1f800040.vy = rot->vy;
                    scratch_rot_1f800040.vz = rot->vz;
                    RotMatrixYXZ((SVECTOR *)TENCHU_SCRATCHPAD(0x40),
                                 (MATRIX *)TENCHU_SCRATCHPAD(0x80));
                }
                scratch_trans_1f800094[0] = pos->vx;
                scratch_trans_1f800094[1] = pos->vy;
                scratch_trans_z_1f80009c = pos->vz;
                SetRotMatrix((MATRIX *)TENCHU_SCRATCHPAD(0x80));
                SetTransMatrix((MATRIX *)TENCHU_SCRATCHPAD(0x80));
                do
                {
                    RotTrans(&camera->r1, &va, fp);
                    RotTrans(&camera->r2, &vb, fp);
                    RotTrans(&camera->p1, pv = &vc, fp);
                    RotTrans(&camera->p2, pv = &vd, fp);
                } while (0);
                pv = 0;
                hitf = trace_ground_(&vc, &vd, 0, 0) > 0x7ff;
                i++;
                if (hitf)
                    goto hit;
                continue;
            }
            break;
        }
        CamState.OldMode = 0;
        break;
    case CMODE_AIM:
        CamState.DirectionRX = 0;
        CamState.DirectionRY = 0;
        CamState.Mode = CMODE_DIRECTION;
        CamState.OldMode = mode;
        break;
    case CMODE_DIRECTION:
    case CMODE_SIGHT:
        if (CamState.Mode != CMODE_DIRECTION && CamState.Mode != CMODE_LOCK)
        {
            GetVectorRotation(CAMERA_VIEWPOINT(&ViewInfo),
                              CAMERA_REFERENCE(&ViewInfo), &rx, &ry);
            rx -= CamState.Owner->model->rotate.vx;
            ry -= CamState.Owner->model->rotate.vy;
            rx = (rx + 2 * ANGLE_FULL + ANGLE_HALF) % ANGLE_FULL - ANGLE_HALF;
            ry = (ry + 2 * ANGLE_FULL + ANGLE_HALF) % ANGLE_FULL - ANGLE_HALF;
            CamState.DirectionRX = 0;
            CamState.DirectionRY = ry;
        }
        CamState.Mode = CMODE_DIRECTION;
        CamState.OldMode = mode;
        break;
    case CMODE_NORMAL:
        if (CamState.Owner->pad.data & PADL1)
        {
            SetCameraMode(CMODE_DIRECTION);
            return;
        }
        CamState.Mode = mode;
        break;
    hit:
        cs->Mode = mode;
        cs->snap_pending = 1;
        break;
    default:
        CamState.Mode = mode;
        break;
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void Camera(void);
 *     CAMERA.C:784, 110 src lines, frame 72 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+24     struct GsRVIEW2 vDif
 *     reg   $s1       short pad_dat
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern enum TSystemFlag SystemFlag;
 *     extern short SkipFrame;
 *     extern int Projection;
 * END PSX.SYM */

void Camera(void)
{
    GsRVIEW2 vDif;
    s16 pad_dat;

    pad_dat = GetPad(PAD_CONTROLLER_1);
    /* The debug owner menu stores AdtSelect's return here — an odd value
     * (its -1 cancel sentinel) is not a real Humanoid. */
    if ((s32)CamState.Owner & 1)
    {
        return;
    }

    switch ((s32)CamState.Mode)
    {
    case CMODE_DIRECTION:
        CameraDirection(CamState.Owner, &vDif);
        break;
    case CMODE_LOCK:
        vDif.vpx = 0;
        vDif.vpy = 0;
        vDif.vpz = 0;
        vDif.vrx = 0;
        vDif.vry = 0;
        vDif.vrz = 0;
        break;
    case CMODE_FALL:
        vDif.vpx = 0;
        vDif.vpy = 0;
        vDif.vpz = 0;
        vDif.vrx = CamState.Owner->model->locate.coord.t[0] - ViewInfo.vrx;
        vDif.vry = CamState.Owner->model->locate.coord.t[1] - ViewInfo.vry;
        vDif.vrz = CamState.Owner->model->locate.coord.t[2] - ViewInfo.vrz;
        break;
    default:
        if (CamState.Owner->pad.data & PADL1)
        {
            SetCameraMode(CMODE_DIRECTION);
            return;
        }
        CameraType1(CamState.Owner, &vDif);
        break;
    }
    ViewInfo.vrx += vDif.vrx;
    ViewInfo.vry += vDif.vry;
    ViewInfo.vrz += vDif.vrz;
    ViewInfo.vpx += vDif.vpx;
    ViewInfo.vpy += vDif.vpy;
    ViewInfo.vpz += vDif.vpz;
    GsSetRefView2(&ViewInfo);

    if ((SystemFlag & SYSFLAG_DEBUGPRINT) != 0 && SkipFrame != SKIPFRAME_SKIPPED &&
        (pad_dat & PADselect) != 0)
    {
        if (pad_dat & PADL2)
        {
            Projection = PROJECTION_DISTANCE;
        }
        if (pad_dat & PADLleft)
        {
            Projection--;
        }
        else if (pad_dat & PADLright)
        {
            Projection++;
        }
        FntPrint(fmt_owner_r, CamState.Owner->model->locate.coord.t[0],
                 CamState.Owner->model->locate.coord.t[1],
                 CamState.Owner->model->locate.coord.t[2],
                 CamState.Owner->model->rotate.vy);
        FntPrint(str_newline);
        GsSetProjection(Projection);
        debug_output_edit_camera_settings(pad_dat);
    }
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void CameraDirection(struct Humanoid *pl, struct GsRVIEW2 *vDif);
 *     CAMERA.C:898, 114 src lines, frame 160 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct Humanoid * pl
 *     param $s3       struct GsRVIEW2 * vDif
 *     stack sp+24     struct GsRVIEW2 target
 *     reg   $s2       struct ModelArchiveType * mad
 *     stack sp+56     struct SVECTOR r
 *     stack sp+64     struct SVECTOR CamLoc
 *     stack sp+72     struct VECTOR vc
 *     stack sp+88     struct MATRIX mat
 *     stack sp+120    int rx
 *     stack sp+124    int ry
 *     stack sp+128    short x
 *     stack sp+130    short y
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

void CameraDirection(Humanoid *pl, GsRVIEW2 *vDif)
{
    GsRVIEW2 target;
    ModelArchiveType *mad;
    SVECTOR r;
    VECTOR CamLoc;
    s32 rx;
    s32 ry;
    s16 x;
    s16 y;

    mad = pl->model;
    GetPadXY(PAD_CONTROLLER_1, &x, &y);
    if (CamState.OldMode == CMODE_SIGHT)
    {
        x = x / 2;
        y = y / 2;
    }
    else if ((CamState.Owner->pad.data & PADL1) == 0)
    {
        SetCameraMode(CMODE_NORMAL);
    }

    CamState.DirectionRX -= y;
    CamState.DirectionRY += x;
    if (CamState.DirectionRX > CAMERA_LOOK_LIMIT_X)
    {
        CamState.DirectionRX = CAMERA_LOOK_LIMIT_X;
    }
    else if (CamState.DirectionRX < -CAMERA_LOOK_LIMIT_X)
    {
        CamState.DirectionRX = -CAMERA_LOOK_LIMIT_X;
    }
    if (CamState.DirectionRY > CAMERA_LOOK_LIMIT_Y)
    {
        CamState.DirectionRY = CAMERA_LOOK_LIMIT_Y;
    }
    else if (CamState.DirectionRY < -CAMERA_LOOK_LIMIT_Y)
    {
        CamState.DirectionRY = -CAMERA_LOOK_LIMIT_Y;
    }

    rx = rsin(mad->rotate.vy) / 6;
    ry = rcos(mad->rotate.vy) / 6;
    r.vx = 0;
    r.vy = 0;
    r.vz = CAMERA_BOOM_LEN;
    RotateVectorS(&r,
                  mad->rotate.vx + CamState.DirectionRX,
                  mad->rotate.vy + CamState.DirectionRY,
                  mad->rotate.vz);

    CamLoc.vx = mad->locate.coord.t[0];
    CamLoc.vy = mad->locate.coord.t[1] - CAMERA_EYE_HEIGHT;
    CamLoc.vz = mad->locate.coord.t[2];
    push_from_walls_(&CamLoc, WALL_AVOID_PUSH);
    CamLoc.vx -= rx;
    CamLoc.vz -= ry;
    if (r.vy > 0)
    {
        CamLoc.vy -= r.vy;
    }
    else
    {
        CamLoc.vy += r.vy / 4;
    }

    target.vrx = CamLoc.vx - r.vx;
    target.vry = CamLoc.vy - r.vy;
    target.vrz = CamLoc.vz - r.vz;
    target.vpx = CamLoc.vx + r.vx;
    target.vpy = CamLoc.vy + r.vy;
    target.vpz = CamLoc.vz + r.vz;

    vDif->vrx = (target.vrx - ViewInfo.vrx) / 4;
    vDif->vry = (target.vry - ViewInfo.vry) / 4;
    vDif->vrz = (target.vrz - ViewInfo.vrz) / 4;
    vDif->vpx = (target.vpx - ViewInfo.vpx) / 8;
    vDif->vpy = (target.vpy - ViewInfo.vpy) / 8;
    vDif->vpz = (target.vpz - ViewInfo.vpz) / 8;
}

void initialise_default_player_cameras_(void)
{
    CamPos = CamPosDefault;
    DEBUG_CAMERA_SLOTS_[DEBUG_CAMERA_SLOT_R1] =
        &DEBUG_CAMERA_BASE_->r1;
    DEBUG_CAMERA_SLOTS_[DEBUG_CAMERA_SLOT_R2] =
        &DEBUG_CAMERA_BASE_->r2;
    DEBUG_CAMERA_SLOTS_[DEBUG_CAMERA_SLOT_P1] =
        &DEBUG_CAMERA_BASE_->p1;
    DEBUG_CAMERA_SLOTS_[DEBUG_CAMERA_SLOT_P2] =
        &DEBUG_CAMERA_BASE_->p2;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void MakeDif(struct GsRVIEW2 *vinfo, struct GsRVIEW2 *target, struct GsRVIEW2 *vdif);
 *     CAMERA.C:418, 8 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       struct GsRVIEW2 * vinfo
 *     param $a1       struct GsRVIEW2 * target
 *     param $a2       struct GsRVIEW2 * vdif
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

void MakeDif(GsRVIEW2 *vinfo, GsRVIEW2 *target, GsRVIEW2 *vdif)
{
    if (CamState.snap_pending == 1)
    {
        vdif->vpx = target->vpx - vinfo->vpx;
        vdif->vpy = target->vpy - vinfo->vpy;
        vdif->vpz = target->vpz - vinfo->vpz;
        vdif->vrx = target->vrx - vinfo->vrx;
        vdif->vry = target->vry - vinfo->vry;
        vdif->vrz = target->vrz - vinfo->vrz;
        CamState.snap_pending = 0;
    }
    else
    {
        MakeDifSub(CAMERA_REFERENCE(vinfo), CAMERA_REFERENCE(target),
                   CAMERA_REFERENCE(vdif), &ref);
        MakeDifSub(CAMERA_VIEWPOINT(vinfo), CAMERA_VIEWPOINT(target),
                   CAMERA_VIEWPOINT(vdif), &pnt);
    }
}
