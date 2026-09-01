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

/*
 * AVCameraSetup (0x80051074, 0x1b4 bytes) — sets ViewInfo's target position
 * (vpx/vpy/vpz) from a camera-command CVA row (read through CVAnow, the
 * CVA script cursor) whose camera-cut kind sub-dispatches: fixed point uses
 * position x/y/z@0x4/0x6/0x8, scaled *100;
 * 0..3 = orbit CameraTarget (the active camera-owner Humanoid, set by
 * CVAsequence to StagePlayer) at a computed angle via GetMoveSpeed, offset
 * by CameraTarget->locate; 5 = re-target a NEW humanoid (GetHumanoid(event's
 * `.p`@0xA), bailing with no GsSetRefView2 call if not found) and adopt
 * it as the new CameraTarget. item.h's Humanoid (rotate@0x3C, locate@0x38,
 * height@0xE) accounts for every field read here.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `event->payload.camera_cut.kind` is read ONCE and reused across the
 *    whole dispatch (no reload) — plain repeated member access CSEs to one load
 *    within the function's single extended basic block, no named local
 *    needed (matches PSX.SYM listing no such local).
 *  - Dispatch body order differs from test order: the tests fire 4, <5,
 *    ==5 while bodies remain orbit, 4, 5. A real switch over cases 0..5 is
 *    the exact source lever: expand_case emits that test order and preserves
 *    the lexical body order without explicit labels.
 *  - The orbit branch's angle (`(u16)rotate->vy + mode*0x400`, explicitly
 *    unsigned per Ghidra's own `(ushort)` cast — SVECTOR.vy is otherwise
 *    signed) is computed ONCE into a shared `s32` temp, stored to
 *    `vect.pad` (a plain field write, matching Ghidra's own
 *    `local_10.pad = (short)iVar2` rendering) THEN passed (truncated) as
 *    GetMoveSpeed's `ry` — writing it twice inline would double the
 *    load/shift instead of reusing one register.
 *  - **The `ordr` default-3000-else-override must be the inline ternary
 *    call ARGUMENT itself**, not a preceding `ordr = cond ? a : b;`
 *    statement (even though the value is used nowhere else): assigning it
 *    to a named local first — whether via if/else, a temp read of
 *    `event->payload.camera_cut.parameters.orbit.distance`, or a ternary —
 *    makes cc1 read it a
 *    SECOND time (an extra unsigned `lhu`, since a bare argument pass
 *    doesn't need sign extension) instead of reusing the first (signed)
 *    read from the zero-test, costing 3 extra instructions and shuffling
 *    the whole function's register colors. Folding the ternary directly
 *    into the call's 3rd argument position fixed it in one edit.
 */

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
