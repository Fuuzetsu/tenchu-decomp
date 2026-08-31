#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetCameraMode(enum TCameraMode mode);
 *     CAMERA.C:85, 9 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       enum TCameraMode mode
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct TCameraPos CamPosCriticalHit[3];
 *     extern struct GsRVIEW2 ViewInfo;
 * END PSX.SYM */

/*
 * SetCameraMode (0x800316b8) — camera mode dispatcher (CAMERA.C).
 * 16-entry jump-table switch; case bodies in source order 4, 15, {1,3}, 0,
 * default (memory order = source order), with case 4's critical-hit success
 * body as a labeled block between case 0 and default (goto target, reached
 * from inside the loop; it reuses the loop's cs pseudo in $s4).
 *
 * NOTE the retail TCameraStatus layout differs from the demo PSX.SYM one:
 * DirectionRX/RY sit at 0x18/0x1A (demo: 0x1C/0x1E) and 0x1C/0x1D are two
 * BYTE fields (OldMode as a byte reused as the critical-camera index + a
 * one-shot camera-snap flag), proven by the raw
 * sh 0x18/sh 0x1A/sb 0x1C/sb 0x1D in this function's own asm.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - Case 4's search loop is a hand-rolled goto loop (loop:/goto loop): no
 *    loop notes, so nothing hoists/forces the &va..&vd call args — each
 *    RotTrans/trace_ground_ frame-address arg re-materializes `addiu aN,sp,N`
 *    at its use. cs/tbl/fp are REAL locals assigned before the loop (that is
 *    what the "hoisted" $s4/$s7/$s3 are); cs's init lands as `addu s4,a1`
 *    because cse folds it onto the preceding OldMode-store's address pseudo.
 *  - The scratchpad STORES go through small extern symbols
 *    (scratch_rot_1f800040 / scratch_trans_1f800094) — a small-extern store
 *    is the one-op $at macro AND is MEM_IN_STRUCT_P, which keeps sched.c's
 *    true_dependence (its MEM_IN_STRUCT heuristic) from batching the
 *    rot->vx/vy/vz and pos->vx/vy/vz loads past the preceding stores: the pairs
 *    stay serialized through one register like the target. Flat
 *    *(s16 *)0x1F8000xx casts are NOT in-struct -> the loads batch (wrong);
 *    struct-pointer casts lose the constant-address macro form (wrong).
 *    The GTE-call ARGS stay literal casts (raw lui/ori), as the bytes show.
 *  - The two do{}while(0) wrappers are load-bearing:
 *    (a) around the scratch-rot stores + RotMatrixYXZ: flow.c counts refs at
 *        +1 loop depth, boosting rot's allocno above p so the allocation
 *        order is rot->$s0, p->$s1 (they come out swapped otherwise);
 *    (b) around the four RotTrans calls: the loop notes are sched1 barriers,
 *        pinning `i++` at the loop bottom (it otherwise floats into a
 *        load-delay slot mid-body), so reorg fills the bnez delay slot with
 *        it and the backjump's slot with the loop-top slti.
 *  - `RotTrans(p+2, pv = &vc, fp); RotTrans(p+3, pv = &vd, fp); pv = 0;`:
 *    each pv reassignment EVICTS pv's register from cse's value class for
 *    the previous &vN, and the dead `pv = 0;` (deleted by flow, zero bytes)
 *    evicts it from &vd's class — so the trace_ground_ args find no register
 *    equivalent and re-materialize fresh addius like the target. Without
 *    this, cse2 (which unlike cse1 does not stop at LOOP_END notes) folds
 *    the ddc args onto the RotTrans arg pseudos, which then need two extra
 *    callee-saved registers ($s0/$s2 + moves).
 *  - `camera = (TCameraPos *)(cs->OldMode * sizeof(*tbl) + (s32)tbl);` uses
 *    INTEGER addition in source order to emit `addu camera,index,base` (the
 *    natural `&tbl[index]` spelling normalizes to base+index and emits the
 *    operands swapped). Inline (no offset temp) so the lbu/sll/addu chain
 *    fuses into camera's register; the entry rand() result is a SEPARATE
 *    variable n (caller-saved $v1) — Ghidra's iVar4 double-role is an SSA
 *    artifact.
 *  - camera_terrain_pitch_ takes the Humanoid* (its own asm derefs $a0 at
 *    0x30/0x38/0x3C); passing cs->Owner ties the Owner temp to $a0.
 *  - hitf (a named flag for the ddc result compare) keeps the scc form
 *    slti/xori/bnez; an inline `if (... > 0x7ff)` compiles slti/beqz (short).
 */

/* Scratchpad work objects (0x1F800040 rotation SVECTOR, 0x1F800080 MATRIX
 * whose .t translation column sits at 0x1F800094). The STORES go through
 * these small extern symbols (assembler $at one-op macro + MEM_IN_STRUCT
 * serialization); the GTE-call ARGS are literal casts (raw lui/ori constant),
 * exactly as the target bytes mix them. Bound in config/symbols.main.exe.txt. */
extern SVECTOR scratch_rot_1f800040;
extern s32 scratch_trans_1f800094[2];

/* Retail's own prototype drift (def: s32 return) -- byte-required: correcting it changes the caller. */
extern short camera_terrain_pitch_(Humanoid *h);

void SetCameraMode(TCameraMode mode)
{
    enum
    {
        MaxCriticalValiation = 3
    };
    VECTOR va;
    VECTOR vb;
    VECTOR vc;
    VECTOR vd;
    long flag;
    s32 rx;
    s32 ry;
    TCameraStatus *cs;
    TCameraPos *tbl;
    long *fp;
    VECTOR *pos;
    SVECTOR *rot;
    TCameraPos *camera;
    VECTOR *pv;
    s32 i;
    s32 hitf;

    switch (mode)
    {
    case CMODE_CRITICAL_HIT:
        /* The eight other carriers in this arm are all load-bearing;
         * removing one each costs cs 5, hitf 12, tbl 21, pv 32, fp 62,
         * camera 70, pos+rot 110, and the whole plain graph 94. Only
         * `n` was staging, and it is gone. */
        CamState.OldMode = rand() % (MaxCriticalValiation + 1);
        i = 0;
        cs = &CamState;
        tbl = CamPosCriticalHit;
        fp = &flag;
    loop:
        if (!(i < MaxCriticalValiation + 1))
            goto giveup;
        cs->OldMode++;
        if (cs->OldMode > MaxCriticalValiation)
            cs->OldMode = 0;
        camera = (TCameraPos *)(cs->OldMode * sizeof(*tbl) + (s32)tbl);
        pos = cs->Owner->locate;
        rot = cs->Owner->rotate;
        do
        {
            scratch_rot_1f800040.vx = rot->vx + camera_terrain_pitch_(cs->Owner);
            scratch_rot_1f800040.vy = rot->vy;
            scratch_rot_1f800040.vz = rot->vz;
            RotMatrixYXZ((SVECTOR *)TENCHU_SCRATCHPAD(0x40),
                         (MATRIX *)TENCHU_SCRATCHPAD(0x80));
        } while (0);
        scratch_trans_1f800094[0] = pos->vx;
        scratch_trans_1f800094[1] = pos->vy;
        scratch_trans_1f800094[2] = pos->vz;
        SetRotMatrix((MATRIX *)TENCHU_SCRATCHPAD(0x80));
        SetTransMatrix((MATRIX *)TENCHU_SCRATCHPAD(0x80));
        do
        {
            RotTrans(&camera->r1, &va, fp);
            RotTrans(&camera->r2, &vb, fp);
            RotTrans(&camera->p1, pv = &vc, fp);
            RotTrans(&camera->p2, pv = &vd, fp);
        } while (0);
        pv = 0;
        hitf = trace_ground_(&vc, &vd, 0, 0) > 0x7ff;
        i++;
        if (hitf)
            goto hit;
        goto loop;
    giveup:
        CamState.OldMode = 0;
        break;
    case CMODE_AIM:
        CamState.DirectionRX = 0;
        CamState.DirectionRY = 0;
        CamState.Mode = CMODE_DIRECTION;
        CamState.OldMode = mode;
        break;
    case CMODE_DIRECTION:
    case CMODE_SIGHT:
        if (CamState.Mode != CMODE_DIRECTION && CamState.Mode != CMODE_LOCK)
        {
            GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
            rx -= CamState.Owner->model->rotate.vx;
            ry -= CamState.Owner->model->rotate.vy;
            rx = (rx + 2 * ANGLE_FULL + ANGLE_HALF) % ANGLE_FULL - ANGLE_HALF;
            ry = (ry + 2 * ANGLE_FULL + ANGLE_HALF) % ANGLE_FULL - ANGLE_HALF;
            CamState.DirectionRX = 0;
            CamState.DirectionRY = ry;
        }
        CamState.Mode = CMODE_DIRECTION;
        CamState.OldMode = mode;
        break;
    case CMODE_NORMAL:
        if (CamState.Owner->pad.data & PADL1)
        {
            SetCameraMode(CMODE_DIRECTION);
            return;
        }
        CamState.Mode = mode;
        break;
    hit:
        cs->Mode = mode;
        cs->snap_pending = 1;
        break;
    default:
        CamState.Mode = mode;
        break;
    }
}
