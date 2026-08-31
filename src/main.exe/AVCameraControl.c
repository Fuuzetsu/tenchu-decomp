#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AVCameraControl(void);
 *     CHRANIM.C:322, 44 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct SVECTOR vect
 *     reg   $s1       long xx
 *     reg   $s0       long zz
 *     reg   $s2       long len
 *     reg   $a1       short ry
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern short CameraPanMode;
 *     extern short CameraSpeed;
 *     extern struct Humanoid *CameraTarget;
 * END PSX.SYM */

/*
 * AVCameraControl (0x80051228) — applies the active cutscene-camera pan
 * mode: normal camera update, orbit rotation, vertical pan, zoom, or target
 * lock, then submits the updated ViewInfo.
 *
 * STATUS: MATCHING — 0x1F4 bytes plus the 9-word jump table.
 *
 * The 2/3 and 6/7 arms contain source-level GetMoveSpeed calls whose common
 * call sequence is merged by cross-jump. The short `ry` needs two ordinary
 * wide copies in the 2/3 arm: `base_angle` preserves its signed extension,
 * while `speed` preserves CameraSpeed's signed load. Both optimize away as
 * storage, but without them cc1 uses modulo-short `lhu` arithmetic and
 * coalesces the input/result into the wrong register.
 */

extern void Camera(void);

enum CameraPanTag
{
    CAMERA_PAN_DISABLED = 0,
    CAMERA_PAN_NORMAL_CAMERA = 1,
    CAMERA_PAN_ORBIT_ANGLE_INCREASE = 2,
    CAMERA_PAN_ORBIT_ANGLE_DECREASE = 3,
    CAMERA_PAN_UP = 4,
    CAMERA_PAN_DOWN = 5,
    CAMERA_PAN_ZOOM_IN = 6,
    CAMERA_PAN_ZOOM_OUT = 7,
    CAMERA_PAN_TRACK_TARGET = 8
};

void AVCameraControl(void)
{
    SVECTOR vect;
    long xx;
    long zz;
    long len;
    short ry;
    long base_angle;
    long speed;

    xx = ViewInfo.vpx - ViewInfo.vrx;
    zz = ViewInfo.vpz - ViewInfo.vrz;
    len = SquareRoot0(xx * xx + zz * zz);
    ry = GetDirection(xx, zz, 0);

    switch (CameraPanMode)
    {
    case CAMERA_PAN_DISABLED:
        return;
    case CAMERA_PAN_NORMAL_CAMERA:
        Camera();
        return;
    case CAMERA_PAN_ORBIT_ANGLE_INCREASE:
    case CAMERA_PAN_ORBIT_ANGLE_DECREASE:
        base_angle = ry;
        /* The twin `speed = CameraSpeed;` on both arms is byte-required
         * (hoisting it above the if mismatches). */
        if (CameraPanMode == CAMERA_PAN_ORBIT_ANGLE_INCREASE)
        {
            speed = CameraSpeed;
            ry = base_angle + speed;
        }
        else
        {
            speed = CameraSpeed;
            ry = base_angle - speed;
        }
        GetMoveSpeed(&vect, ry, len, 0);
        goto apply_eye_xz;
    case CAMERA_PAN_UP:
    case CAMERA_PAN_DOWN:
        ViewInfo.vpy += (CameraPanMode == CAMERA_PAN_UP)
                            ? -CameraSpeed
                            : CameraSpeed;
        break;
    case CAMERA_PAN_ZOOM_IN:
    case CAMERA_PAN_ZOOM_OUT:
        if (CameraPanMode == CAMERA_PAN_ZOOM_IN)
        {
            len -= CameraSpeed;
        }
        else
        {
            len += CameraSpeed;
        }
        GetMoveSpeed(&vect, ry, len, 0);

    apply_eye_xz:
        ViewInfo.vpx = ViewInfo.vrx + vect.vx;
        ViewInfo.vpz = ViewInfo.vrz + vect.vz;
        break;
    case CAMERA_PAN_TRACK_TARGET:
        ViewInfo.vrx = CameraTarget->locate->vx;
        ViewInfo.vry = CameraTarget->locate->vy - CameraTarget->height + 300;
        ViewInfo.vrz = CameraTarget->locate->vz;
        break;
    }

    GsSetRefView2(&ViewInfo);
}
