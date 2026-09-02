#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SnapCameraTargetVector(void);
 *     CAMERA.C:106, 30 src lines, frame 72 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     struct VECTOR v
 *     stack sp+32     struct SVECTOR sv
 *     stack sp+40     struct SVECTOR sv2
 *     reg   $a0       struct VECTOR * target
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

/*
 * SnapCameraTargetVector (0x8002fbc8, 0x1d4 bytes) — snaps CamState's
 * TargetVector either to the area-map "passage" point along the camera's
 * view->reference direction, or (if none) to the owner Humanoid's model
 * position.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - `v` (VECTOR) is built in the same storage as PSX.SYM's contiguous
 *    `sv`/`sv2` pair (together exactly 16 bytes) and THEN block-copied into
 *    `v` — the classic align-4 word block move (4 lw + 4 sw), not a
 *    direct per-field store into `v`. The frame is fully accounted for by
 *    PSX.SYM's 4 locals + the standard 16-byte outgoing-args area + 20
 *    bytes of saved regs + 4 bytes alignment pad (0x10+0x20+0x14+4=0x48),
 *    so there is no room for a hidden 5th local here. One 16-byte work vector
 *    supplies the VECTOR and paired-SVECTOR views of that storage.
 *    DEMO-VERIFIED (PSX.EXE @ 0x8002aad4): the original build does the
 *    identical dance — memset 16 bytes over both adjacent SVECTORs, three
 *    32-bit ViewInfo stores through sv's memory, 4-word block copy into v
 *    (the demo zeroes and reuses sv where retail uses sv2 for the second
 *    phase — a variable-role swap, same construct). The casts are the
 *    original author's own scratch-buffer idiom, not a matching artifact.
 *  - `sv2.vx/vy/vz = (s16)ViewInfo.vrx - (s16)ViewInfo.vpx` etc. are
 *    NARROWING uses of the s32 GsRVIEW2 fields (result stores into a
 *    16-bit SVECTOR field) — cc1 emits `lhu`, not `lh`, for both operands.
 *  - `sv = sv2;` (plain SVECTOR struct assignment, align 2) reproduces the
 *    target's `lwl/lwr`+`swl/swr` copy immediately before the
 *    `VectorNormalSS(&sv, &sv2)` call.
 *  - Retail evolves CAMERA.C's original `VSHIFT` from 8 to 5. The
 *    signed truncate-toward-zero `>> VSHIFT` sequence needs THREE
 *    SEPARATE `s32` temps (t1/t2/t3), one per axis — not one reused temp.
 *    gcc 2.8.1 never splits a pseudo's live range, so a single shared temp
 *    pins the whole computation to one register and serializes it; the
 *    target's asm shows axis N+1's raw load already in flight (a second
 *    register) while axis N's shift is still executing (a 2-deep software
 *    pipeline that needs three independently-allocated pseudos).
 *  - The final if/else is Ghidra's condition INVERTED: Ghidra renders
 *    `if (target == NULL) {fallback} else {success}`, but the target's
 *    asm has the SUCCESS body as the fallthrough (falls straight into a
 *    `j` past the fallback body) and the NULL-fallback body as the branch
 *    target that falls straight into the epilogue — the opposite-polarity
 *    shape, so the source is `if (target != NULL) {success} else
 *    {fallback}`.
 *  - `CamState.Owner->model->locate.coord.t[0..2]` (the fallback path) is
 *    read fresh via THREE separate `Owner->model` dereferences (Ghidra's
 *    own SSA rendering shows a distinct `pVVar2`-less reload each time) —
 *    do not cache `model` in a local.
 */

void SnapCameraTargetVector(void)
{
    enum
    {
        VSHIFT = 5
    };
    VECTOR v;
    VECTOR work;
    VECTOR *target;
    s32 t1, t2, t3;

    memset(&work, 0, sizeof(VECTOR));
    work.vx = ViewInfo.vpx;
    work.vy = ViewInfo.vpy;
    work.vz = ViewInfo.vpz;
    v = work;

    memset(&((SVECTOR *)&work)[1], 0, sizeof(SVECTOR));
    ((SVECTOR *)&work)[1].vx = (s16)ViewInfo.vrx - (s16)ViewInfo.vpx;
    ((SVECTOR *)&work)[1].vy = (s16)ViewInfo.vry - (s16)ViewInfo.vpy;
    ((SVECTOR *)&work)[1].vz = (s16)ViewInfo.vrz - (s16)ViewInfo.vpz;
    *(SVECTOR *)&work = ((SVECTOR *)&work)[1];
    VectorNormalSS((SVECTOR *)&work, &((SVECTOR *)&work)[1]);

    t1 = ((SVECTOR *)&work)[1].vx;
    ((SVECTOR *)&work)[1].vx = (s16)(t1 / (1 << VSHIFT));
    t2 = ((SVECTOR *)&work)[1].vy;
    ((SVECTOR *)&work)[1].vy = (s16)(t2 / (1 << VSHIFT));
    t3 = ((SVECTOR *)&work)[1].vz;
    ((SVECTOR *)&work)[1].vz = (s16)(t3 / (1 << VSHIFT));

    target = GetAreaMapPassage(GlobalAreaMap, &v,
                               &((SVECTOR *)&work)[1], -1);
    if (target != 0)
    {
        CamState.TargetVector.vx = target->vx;
        CamState.TargetVector.vy = target->vy;
        CamState.TargetVector.vz = target->vz;
    }
    else
    {
        CamState.TargetVector.vx = CamState.Owner->model->locate.coord.t[0];
        CamState.TargetVector.vy = CamState.Owner->model->locate.coord.t[1];
        CamState.TargetVector.vz = CamState.Owner->model->locate.coord.t[2];
    }
}
