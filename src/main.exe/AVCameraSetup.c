#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AVCameraSetup(void);
 *     CHRANIM.C:289, 29 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct SVECTOR vect
 *     reg   $a1       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern struct CVAType *CVAnow;
 *     extern struct Humanoid *CameraTarget;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

void AVCameraSetup(void)
{
    CVAType *event;
    Humanoid *human;
    SVECTOR vect;
    s32 ry;

    event = CVAnow;
    switch (event->payload.camera_cut.kind)
    {
    case CVA_CAMERA_CUT_TARGET_RELATIVE_BASE:
    case CVA_CAMERA_CUT_TARGET_RELATIVE_QUARTER_TURN:
    case CVA_CAMERA_CUT_TARGET_RELATIVE_HALF_TURN:
    case CVA_CAMERA_CUT_TARGET_RELATIVE_THREE_QUARTER_TURN:
        ry = (u16)CameraTarget->rotate->vy +
             event->payload.camera_cut.kind * ANGLE_QUADRANT;
        vect.pad = (s16)ry;
        GetMoveSpeed(&vect, (s16)ry,
                     (event->payload.camera_cut.parameters.orbit.distance != 0)
                         ? event->payload.camera_cut.parameters.orbit.distance
                         : CVA_CAMERA_DEFAULT_ORBIT_DISTANCE,
                     0);
        ViewInfo.vpx = CameraTarget->locate->vx + vect.vx;
        ViewInfo.vpy = (CameraTarget->locate->vy - CameraTarget->height) +
                       CVA_CAMERA_TARGET_HEIGHT_OFFSET;
        ViewInfo.vpz = CameraTarget->locate->vz + vect.vz;
        break;

    case CVA_CAMERA_CUT_FIXED_POSITION:
        ViewInfo.vpx = event->payload.camera_cut.parameters.fixed.position.x *
                       CVA_CAMERA_POSITION_SCALE;
        ViewInfo.vpy = event->payload.camera_cut.parameters.fixed.position.y *
                       CVA_CAMERA_POSITION_SCALE;
        ViewInfo.vpz = event->payload.camera_cut.parameters.fixed.position.z *
                       CVA_CAMERA_POSITION_SCALE;
        break;

    case CVA_CAMERA_CUT_HUMANOID_POSITION:
        human = GetHumanoid(
            event->payload.camera_cut.parameters.humanoid.actor);
        if (human == 0)
        {
            return;
        }
        ViewInfo.vpx = human->locate->vx;
        ViewInfo.vpy = (human->locate->vy - human->height) +
                       CVA_CAMERA_TARGET_HEIGHT_OFFSET;
        ViewInfo.vpz = human->locate->vz;
        CameraTarget = human;
        break;
    }

    GsSetRefView2(&ViewInfo);
}
