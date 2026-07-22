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

extern SVECTOR D_80097A28[];
extern SVECTOR D_80097A30[];
extern TCameraPos D_80011A64;
extern TCameraPos D_80011A84;
extern TCameraPos D_80011AA4;
extern TCameraPos D_80011AC4;
extern TCameraPos D_80011AE4;
extern TCameraPos D_80011B04;
extern TCameraPos D_80011B24;
extern TCameraPos D_80011B44;
extern TCameraPos D_80011B64;
extern TCameraPos D_80011B84;

extern s32 MakeCameraPosition(VECTOR *orgpos, SVECTOR *orgrot,
                              SVECTOR *campos, GsRVIEW2 *vDif);

void CameraType1(Humanoid *pl, GsRVIEW2 *vDif)
{
    enum { FL = 1, FR = 2, BL = 4, BR = 8 };
    ModelArchiveType *mad;
    VECTOR pos;
    CameraScratch scratch;
    TCameraPos alternate;
    TCameraStatus *cs;

    mad = pl->model;
    memset(&scratch.init, 0, sizeof(scratch.init));
    scratch.init.vx = pl->model->locate.coord.t[0];
    scratch.init.vy = pl->model->locate.coord.t[1] - 0x60E;
    scratch.init.vz = pl->model->locate.coord.t[2];
    pos = scratch.init;
    mad->attribute |= 2;

    switch ((s16)(CamState.Owner->status - 3)) {
    case 9:
    {
        s32 levfl;
        s32 levfr;
        s32 levbl;
        s32 levbr;
        s32 levmap;

        scratch.probe.vecl = D_80097A28[0];
        scratch.probe.vecr = D_80097A30[0];
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

        levmap = levfl == (s32)0x80000000 ? FL : 0;
        if (levfr == (s32)0x80000000)
            levmap |= FR;
        if (levbl == (s32)0x80000000)
            levmap |= BL;
        if (levbr == (s32)0x80000000)
            levmap |= BR;

        if ((levmap & (FL | FR | BL | BR)) == BL) {
            CamState.Mode = CMODE_PEEP_R;
            goto choose_camera;
        } else if ((levmap & (FL | FR | BL | BR)) == BR) {
            CamState.Mode = CMODE_PEEP_L;
            goto choose_camera;
        } else if ((levmap & (FL | FR)) == FL) {
            CamState.Mode = CMODE_STICK_R;
            goto choose_camera;
        } else if ((levmap & (FL | FR)) == FR) {
            CamState.Mode = CMODE_STICK_L;
            goto choose_camera;
        } else {
            CamState.Mode = CMODE_NORMAL;
            goto choose_camera;
        }
    }
    case 8:
        CamState.Mode = CMODE_CROUCH;
        goto choose_camera;
    case 0:
        CamState.Mode = CMODE_SWIM;
        goto choose_camera;
    case 7:
        CamState.Mode = 0x11;
        goto choose_camera;
    case 3:
        cs = &CamState;
        if (cs->Owner->motion->mid != 0x600)
            goto choose_camera;
        cs->Mode = CMODE_RUN;
        goto choose_camera;
    case 5:
        cs = &CamState;
        if (cs->Owner->motion->mid != 0x801)
            goto choose_camera;
        cs->Mode = CMODE_RUN;
        goto choose_camera;
    case 0xD:
    {
        u16 mid;

        cs = &CamState;
        mid = cs->Owner->motion->mid;
        if ((u16)(mid - 0x1005) < 5) {
            cs->Mode = 0x10;
            goto choose_camera;
        }
        if ((s16)mid != 0x100C)
            goto choose_camera;
        cs->Mode = 0x10;
        goto choose_camera;
    }
    default:
        goto choose_camera;
    }

choose_camera:
    switch (CamState.Mode) {
    case CMODE_CRITICAL_HIT:
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &CamPosCriticalHit[CamState.OldMode].r1, vDif);
        return;
    case CMODE_STICK_L:
        scratch.probe.stick_l_camera = D_80011A64;
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &scratch.probe.stick_l_camera.r1, vDif);
        return;
    case CMODE_STICK_R:
        scratch.camera = D_80011A84;
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &scratch.camera.r1, vDif);
        return;
    case CMODE_PEEP_L:
        scratch.camera = D_80011AA4;
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &scratch.camera.r1, vDif);
        return;
    case CMODE_PEEP_R:
        scratch.camera = D_80011AC4;
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &scratch.camera.r1, vDif);
        return;
    case CMODE_CROUCH:
        scratch.camera = D_80011AE4;
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &scratch.camera.r1, vDif);
        CamState.Mode = CMODE_NORMAL;
        return;
    case CMODE_RUN:
        scratch.camera = D_80011B04;
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &scratch.camera.r1, vDif);
        CamState.Mode = CMODE_NORMAL;
        return;
    case CMODE_SWIM:
        scratch.camera = D_80011B24;
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &scratch.camera.r1, vDif);
        CamState.Mode = CMODE_NORMAL;
        return;
    case 0x10:
        scratch.camera = D_80011B44;
        alternate = D_80011B64;

        if (MakeCameraPosition(&pos, &pl->model->rotate,
                               &scratch.camera.r1, vDif) < 0x801) {
            MakeCameraPosition(&pos, &pl->model->rotate,
                               &alternate.r1, vDif);
        }
        CamState.Mode = CMODE_NORMAL;
        return;
    case 0x11:
        scratch.camera = D_80011B84;
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &scratch.camera.r1, vDif);
        CamState.Mode = CMODE_NORMAL;
        return;
    default:
        MakeCameraPosition(&pos, &pl->model->rotate,
                           &CAMERA_R1.r1, vDif);
        return;
    }
}
