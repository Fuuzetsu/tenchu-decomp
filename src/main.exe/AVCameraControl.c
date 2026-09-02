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

extern void Camera(void);

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
