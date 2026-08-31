#include "common.h"
#include "main.exe.h"

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

/*
 * CameraType1 (0x80030c74) — select a third-person camera placement from
 * the owner's state and nearby map geometry, then generate the view delta.
 *
 * Byte-matched retail extent: 2628 bytes / 657 instructions. CameraScratch
 * overlays the initializer, wall-probe vectors, and mutually-exclusive
 * camera presets exactly as the original 0x98-byte frame does. The stick-left
 * preset starts after the probe pair; the knockback arm holds a second
 * preset separately because both are live across the first camera call.
 *
 * The union looks invented and is not; three ways of removing it were
 * measured (cc1 gives every sibling block its own stack slot and never
 * overlaps by lifetime — see the cookbook):
 *   - a `TCameraPos campos;` inside each preset arm: +304 lines, because
 *     each arm then owns a separate 32-byte slot;
 *   - plain function-scope `init`/`vecl`/`vecr`/`campos`: +408, and the
 *     frame grows to 168 against retail's 152 — exactly the 16 bytes that
 *     `init` stops sharing;
 *   - dropping `init` and building `pos` in place: the frame lands on 152
 *     exactly, but +288, because retail really does keep both objects and
 *     copy sp+40 to sp+24 between them.
 * PSX.SYM's local list is not a target here, and the reason is now
 * exact: those eight `campos`/`ref` pairs are MakeCameraPosition's own
 * two parameters, recorded once per site because the demo INLINED it.
 * Retail calls it (three `jal MakeCameraPosition` here), so there is no
 * scope to recover -- the pairs are a callee's frame, not this
 * function's locals. See the cookbook rule on reading nested records.
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
    /* Fresh pl->model reads (not mad): byte-required (measured). */
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

        levmap = levfl == LEVEL_NONE ? FL : 0;
        if (levfr == LEVEL_NONE)
            levmap |= FR;
        if (levbl == LEVEL_NONE)
            levmap |= BL;
        if (levbr == LEVEL_NONE)
            levmap |= BR;

        /* The full four-bit mask is byte-required even though levmap only
         * ever holds these bits (dropping it recolors the compare; measured). */
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
        if ((u16)(mid - MOT_DAMAGE_LAUNCH_BACK) <=
            MOT_DAMAGE_DOWNED - MOT_DAMAGE_LAUNCH_BACK)
        {
            cs->Mode = CMODE_KNOCKBACK;
            break;
        }
        /* (s16) re-extends the lhu-loaded mid: byte-required (measured). */
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
                               &scratch.camera.r1, vDif) <= 0x800)
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
