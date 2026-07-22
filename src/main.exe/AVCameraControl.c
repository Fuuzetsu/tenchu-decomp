#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AVCameraControl(void);
 *     CHRANIM.C:322, 44 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     stack sp+16     struct SVECTOR vect
 *     reg   $s1       long xx
 *     reg   $s0       long zz
 *     reg   $s2       long len
 *     reg   $a1       short ry
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern short CameraPanMode;
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
 * while `speed` preserves D_80097CC8's signed load. Both optimize away as
 * storage, but without them cc1 uses modulo-short `lhu` arithmetic and
 * coalesces the input/result into the wrong register.
 */

extern s16 D_80097CC8;

extern s16 GetDirection(s32 dx, s32 dz, s16 roty);
extern void GetMoveSpeed(SVECTOR *vect, s16 ry, s16 ordr, s16 side);
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
        if (CameraPanMode == 2)
        {
            speed = D_80097CC8;
            ry = move_base + speed;
        }
        else
        {
            speed = D_80097CC8;
            ry = move_base - speed;
        }
        GetMoveSpeed(&vect, ry, len, 0);
        goto apply_move;
    case 4:
    case 5:
        ViewInfo.vpy += (CameraPanMode == 4) ? -D_80097CC8 : D_80097CC8;
        break;
    case 6:
    case 7:
        if (CameraPanMode == 6)
        {
            len -= D_80097CC8;
        }
        else
        {
            len += D_80097CC8;
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
