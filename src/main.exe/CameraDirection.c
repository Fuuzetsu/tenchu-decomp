#include "common.h"
#include "main.exe.h"

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

/*
 * The free-look camera (CMODE_DIRECTION): stick input pans the aim angles
 * (clamped to ~80 deg up/down and 90 deg sideways; while the PREVIOUS mode
 * was SIGHT the stick is halved and the L1 release check is skipped), and
 * the eye sits at head height nudged toward open space, looking down the
 * rotated forward ray.
 */
#include "item.h"
#include "padcmd.h"

extern void push_from_walls_(VECTOR *pos, s32 amount);

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
    GetPadXY(0, &x, &y);
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
