#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void CameraType1(struct Humanoid *pl, struct GsRVIEW2 *vDif);
 *     CAMERA.C:603, 176 src lines, frame 200 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
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

/*
 * CameraType1 (0x80030c74) — select a third-person camera placement from
 * the owner's state and nearby map geometry, then generate the view delta.
 *
 * Byte-matched retail extent: 2628 bytes / 657 instructions. CameraScratch
 * overlays the initializer, wall-probe vectors, and mutually-exclusive
 * camera presets exactly as the original 0x98-byte frame does. The stick-left
 * preset starts after the probe pair; the critical transition holds a second
 * preset separately because both are live across the first camera call.
 */

#include "item.h"

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

extern s32 MakeCameraPosition(VECTOR *orgpos, SVECTOR *orgrot,
                              SVECTOR *campos, GsRVIEW2 *vDif);

void CameraType1(Humanoid *pl, GsRVIEW2 *vDif)
{
    enum
    {
        FL = 1,
        FR = 2,
        BL = 4,
        BR = 8
    };
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
        s32 levmap;

        scratch.probe.vecl = CamVecL[0];
        scratch.probe.vecr = CamVecR[0];
        RotateVectorS(&scratch.probe.vecl,
                      mad->rotate.vx, mad->rotate.vy, 0);
        RotateVectorS(&scratch.probe.vecr,
                      mad->rotate.vx, mad->rotate.vy, 0);
        levfl = GetAreaMapLevel(GlobalAreaMap,
                                mad->locate.coord.t[0] + scratch.probe.vecl.vx,
                                mad->locate.coord.t[1],
                                mad->locate.coord.t[2] + scratch.probe.vecl.vz, 1);
        levfr = GetAreaMapLevel(GlobalAreaMap,
                                mad->locate.coord.t[0] + scratch.probe.vecr.vx,
                                mad->locate.coord.t[1],
                                mad->locate.coord.t[2] + scratch.probe.vecr.vz, 1);
        levbr = GetAreaMapLevel(GlobalAreaMap,
                                mad->locate.coord.t[0] - scratch.probe.vecl.vx,
                                mad->locate.coord.t[1],
                                mad->locate.coord.t[2] - scratch.probe.vecl.vz, 1);
        levbl = GetAreaMapLevel(GlobalAreaMap,
                                mad->locate.coord.t[0] - scratch.probe.vecr.vx,
                                mad->locate.coord.t[1],
                                mad->locate.coord.t[2] - scratch.probe.vecr.vz, 1);

        levmap = levfl == LEVEL_NONE ? FL : 0;
        if (levfr == LEVEL_NONE)
            levmap |= FR;
        if (levbl == LEVEL_NONE)
            levmap |= BL;
        if (levbr == LEVEL_NONE)
            levmap |= BR;

        if ((levmap & (FL | FR | BL | BR)) == BL)
        {
            CamState.Mode = CMODE_PEEP_R;
            break;
        }
        else if ((levmap & (FL | FR | BL | BR)) == BR)
        {
            CamState.Mode = CMODE_PEEP_L;
            break;
        }
        else if ((levmap & (FL | FR)) == FL)
        {
            CamState.Mode = CMODE_STICK_R;
            break;
        }
        else if ((levmap & (FL | FR)) == FR)
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
        if ((u16)(mid - MOT_DAMAGE_LAUNCH_BACK) < 5)
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
                               &scratch.camera.r1, vDif) < 0x801)
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
