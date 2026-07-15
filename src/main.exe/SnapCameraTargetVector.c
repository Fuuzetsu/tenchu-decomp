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
 *  - `v` (VECTOR) is built by writing ViewInfo.vpx/vpy/vpz through a
 *    VECTOR-typed cast of `&sv` (PSX.SYM's declared SVECTOR, whose memory
 *    is contiguous with `sv2` — together exactly 16 bytes) and THEN
 *    block-copying that scratch into `v` — the classic align-4
 *    `*(VECTOR*)a = *(VECTOR*)b` word block move (4 lw + 4 sw), not a
 *    direct per-field store into `v`. The frame is fully accounted for by
 *    PSX.SYM's 4 locals + the standard 16-byte outgoing-args area + 20
 *    bytes of saved regs + 4 bytes alignment pad (0x10+0x20+0x14+4=0x48),
 *    so there is no room for a hidden 5th local here — this cast-through-sv
 *    shape is the only way to build `v` from just `v`/`sv`/`sv2`.
 *  - `sv2.vx/vy/vz = (s16)ViewInfo.vrx - (s16)ViewInfo.vpx` etc. are
 *    NARROWING uses of the s32 TViewInfo fields (result stores into a
 *    16-bit SVECTOR field) — cc1 emits `lhu`, not `lh`, for both operands.
 *  - `sv = sv2;` (plain SVECTOR struct assignment, align 2) reproduces the
 *    target's `lwl/lwr`+`swl/swr` copy immediately before the
 *    `VectorNormalSS(&sv, &sv2)` call.
 *  - The round-toward-negative-infinity `>>5` (divide by 32) needs THREE
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

typedef struct
{
    s32 vpx, vpy, vpz;           /* 0x00 */
    s32 vrx, vry, vrz;           /* 0x0C */
} TViewInfo;

typedef struct
{
    VECTOR TargetVector;         /* 0x00 */
    Humanoid *Owner;             /* 0x10 */
    s32 Mode;                    /* 0x14 */
    s16 DirectionRX;             /* 0x18 */
    s16 DirectionRY;             /* 0x1A */
    u8 OldMode;                  /* 0x1C */
    u8 CriticalHit;              /* 0x1D */
} TCameraStatus;

extern TCameraStatus CamState;
extern TViewInfo ViewInfo;
extern unsigned long *GlobalAreaMap;

extern void VectorNormalSS(SVECTOR *v0, SVECTOR *v1);
extern VECTOR *GetAreaMapPassage(unsigned long *area, VECTOR *pos, SVECTOR *vect, short n);

void SnapCameraTargetVector(void)
{
    VECTOR v;
    SVECTOR sv, sv2;
    VECTOR *target;
    s32 t1, t2, t3;

    memset(&sv, 0, 0x10);
    ((VECTOR *)&sv)->vx = ViewInfo.vpx;
    ((VECTOR *)&sv)->vy = ViewInfo.vpy;
    ((VECTOR *)&sv)->vz = ViewInfo.vpz;
    v = *(VECTOR *)&sv;

    memset(&sv2, 0, 8);
    sv2.vx = (s16)ViewInfo.vrx - (s16)ViewInfo.vpx;
    sv2.vy = (s16)ViewInfo.vry - (s16)ViewInfo.vpy;
    sv2.vz = (s16)ViewInfo.vrz - (s16)ViewInfo.vpz;
    sv = sv2;
    VectorNormalSS(&sv, &sv2);

    t1 = sv2.vx;
    if (t1 < 0)
        t1 += 0x1f;
    sv2.vx = (s16)(t1 >> 5);
    t2 = sv2.vy;
    if (t2 < 0)
        t2 += 0x1f;
    sv2.vy = (s16)(t2 >> 5);
    t3 = sv2.vz;
    if (t3 < 0)
        t3 += 0x1f;
    sv2.vz = (s16)(t3 >> 5);

    target = GetAreaMapPassage(GlobalAreaMap, &v, &sv2, -1);
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
