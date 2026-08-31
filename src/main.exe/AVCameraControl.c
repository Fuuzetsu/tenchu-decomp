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
 * wide copies in the 2/3 arm: `move_base` preserves its signed extension,
 * while `speed` preserves CameraSpeed's signed load. Both optimize away as
 * storage, but without them cc1 uses modulo-short `lhu` arithmetic and
 * coalesces the input/result into the wrong register.
 */

extern void Camera(void);

void AVCameraControl(void)
{
    SVECTOR vect;
    long xx;
    long zz;
    long len;
    short ry;
    long move_base;
    long speed;

    xx = ViewInfo.vpx - ViewInfo.vrx;
    zz = ViewInfo.vpz - ViewInfo.vrz;
    len = SquareRoot0(xx * xx + zz * zz);
    ry = GetDirection(xx, zz, 0);

    switch (CameraPanMode)
    {
    case 0:
        return;
    case 1:
        Camera();
        return;
    case 2:
    case 3:
        move_base = ry;
        /* The twin `speed = CameraSpeed;` on both arms is byte-required
         * (hoisting it above the if mismatches). */
        if (CameraPanMode == 2)
        {
            speed = CameraSpeed;
            ry = move_base + speed;
        }
        else
        {
            speed = CameraSpeed;
            ry = move_base - speed;
        }
        GetMoveSpeed(&vect, ry, len, 0);
        goto apply_move;
    case 4:
    case 5:
        ViewInfo.vpy += (CameraPanMode == 4) ? -CameraSpeed : CameraSpeed;
        break;
    case 6:
    case 7:
        if (CameraPanMode == 6)
        {
            len -= CameraSpeed;
        }
        else
        {
            len += CameraSpeed;
        }
        GetMoveSpeed(&vect, ry, len, 0);

    apply_move:
        ViewInfo.vpx = ViewInfo.vrx + vect.vx;
        ViewInfo.vpz = ViewInfo.vrz + vect.vz;
        break;
    case 8:
        ViewInfo.vrx = CameraTarget->locate->vx;
        ViewInfo.vry = CameraTarget->locate->vy - CameraTarget->height + 300;
        ViewInfo.vrz = CameraTarget->locate->vz;
        break;
    }

    GsSetRefView2(&ViewInfo);
}
