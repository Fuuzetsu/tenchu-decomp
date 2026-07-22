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
 *     reg   $a1       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern struct CVAType *CVAnow;
 *     extern struct Humanoid *CameraTarget;
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

/*
 * AVCameraSetup (0x80051074, 0x1b4 bytes) — sets ViewInfo's target position
 * (vpx/vpy/vpz) from a "camera event" record (CHOSEN_EVENT_LIST_THING_
 * LOCATION, already-named — a cursor into CVAdata) whose
 * `.mode`@0x2 dispatches: 4 = fixed point (x/y/z@0x4/0x6/0x8, scaled *100);
 * 0..3 = orbit CameraTarget (the active camera-owner Humanoid, set by
 * CVAsequence to StagePlayer) at a computed angle via GetMoveSpeed, offset
 * by CameraTarget->locate; 5 = re-target a NEW humanoid (GetHumanoid(event's
 * `.p`@0xA), bailing with no GsSetRefView2 call if not found) and adopt
 * it as the new CameraTarget. item.h's Humanoid (rotate@0x3C, locate@0x38,
 * height@0xE) accounts for every field read here.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `event->id` is read ONCE and reused across the whole dispatch (no
 *    reload) — a plain repeated `event->id` dereference CSEs to one load
 *    within the function's single extended basic block, no named local
 *    needed (matches PSX.SYM listing no such local).
 *  - **Dispatch body order differs from test order**: the tests fire
 *    4, <5, ==5 but the target lays the bodies out <5(orbit), 4, 5 in
 *    memory (the orbit body ends with an explicit jump to the shared
 *    tail; the `==4` body also jumps; the `==5` body is textually LAST
 *    and falls through with no trailing jump). A plain if/else-if always
 *    inlines each body right after its own test, giving the wrong layout
 *    — this needs the cookbook's `if (cond) goto L;` ladder, with the
 *    goto LABELS placed in the target's memory order while the tests stay
 *    in their own (different) order.
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
 *    `event->p`, or a ternary — makes cc1 read `event->p` a
 *    SECOND time (an extra unsigned `lhu`, since a bare argument pass
 *    doesn't need sign extension) instead of reusing the first (signed)
 *    read from the zero-test, costing 3 extra instructions and shuffling
 *    the whole function's register colors. Folding the ternary directly
 *    into the call's 3rd argument position fixed it in one edit.
 */



extern void GetMoveSpeed(SVECTOR *vect, s16 ry, s16 ordr, s16 side);

void AVCameraSetup(void)
{
    CVAType *event;
    Humanoid *human;
    SVECTOR vect;
    s32 ry;

    event = CVAnow;
    if (event->id == 4)
    {
        goto case4;
    }
    if (event->id < 5)
    {
        goto case_orbit;
    }
    if (event->id == 5)
    {
        goto case5;
    }
    goto tail;

case_orbit:
    if (event->id >= 0)
    {
        ry = (u16)CameraTarget->rotate->vy + (event->id << 10);
        vect.pad = (s16)ry;
        GetMoveSpeed(&vect, (s16)ry, (event->p != 0) ? event->p : 3000, 0);
        ViewInfo.vpx = CameraTarget->locate->vx + vect.vx;
        ViewInfo.vpy = (CameraTarget->locate->vy - CameraTarget->height) + 300;
        ViewInfo.vpz = CameraTarget->locate->vz + vect.vz;
    }
    goto tail;

case4:
    ViewInfo.vpx = event->x * 100;
    ViewInfo.vpy = event->y * 100;
    ViewInfo.vpz = event->z * 100;
    goto tail;

case5:
    human = GetHumanoid(event->p);
    if (human == 0)
    {
        return;
    }
    ViewInfo.vpx = human->locate->vx;
    ViewInfo.vpy = (human->locate->vy - human->height) + 300;
    ViewInfo.vpz = human->locate->vz;
    CameraTarget = human;

tail:
    GsSetRefView2(&ViewInfo);
}
