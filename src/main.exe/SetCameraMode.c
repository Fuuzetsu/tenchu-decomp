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
 *     param $a0       enum TCameraMode mode
 *
 * Globals it touches, as the original declared them:
 *     extern struct TCameraStatus CamState;
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
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
 * critical-hit flag), proven by the raw sh 0x18/sh 0x1A/sb 0x1C/sb 0x1D in
 * this function's own asm.
 *
 * Matching notes (see docs/matching-cookbook.md):
 *  - Case 4's search loop is a hand-rolled goto loop (loop:/goto loop): no
 *    loop notes, so nothing hoists/forces the &va..&vd call args — each
 *    RotTrans/FUN_80039ddc frame-address arg re-materializes `addiu aN,sp,N`
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
 *    evicts it from &vd's class — so the FUN_80039ddc args find no register
 *    equivalent and re-materialize fresh addius like the target. Without
 *    this, cse2 (which unlike cse1 does not stop at LOOP_END notes) folds
 *    the ddc args onto the RotTrans arg pseudos, which then need two extra
 *    callee-saved registers ($s0/$s2 + moves).
 *  - `p = (SVECTOR *)(cs->OldMode * 0x20 + (s32)tbl);` — INTEGER addition in
 *    source order emits `addu p,index,base` (any pointer-arithmetic spelling
 *    normalizes to base+index at the tree level and emits the operands
 *    swapped). Inline (no offset temp) so the lbu/sll/addu chain fuses into
 *    p's register; the entry rand() result is a SEPARATE variable n
 *    (caller-saved $v1) — Ghidra's iVar4 double-role is an SSA artifact.
 *  - FUN_8002fd9c takes the Humanoid* (its own asm derefs $a0 at
 *    0x30/0x38/0x3C); passing cs->Owner ties the Owner temp to $a0.
 *  - hitf (a named flag for the ddc result compare) keeps the scc form
 *    slti/xori/bnez; an inline `if (... > 0x7ff)` compiles slti/beqz (short).
 */

extern TCameraStatus CamState;
extern GsRVIEW2 ViewInfo;
/* Critical-hit camera placements: 4 poses x 4 SVECTORs each. */
extern SVECTOR D_80089F50[][4];
/* Scratchpad work objects (0x1F800040 rotation SVECTOR, 0x1F800080 MATRIX
 * whose .t translation column sits at 0x1F800094). The STORES go through
 * these small extern symbols (assembler $at one-op macro + MEM_IN_STRUCT
 * serialization); the GTE-call ARGS are literal casts (raw lui/ori constant),
 * exactly as the target bytes mix them. Bound in config/symbols.main.exe.txt. */
extern SVECTOR scratch_rot_1f800040;
extern s32 scratch_trans_1f800094[2];

extern void GetVectorRotation(VECTOR *start, VECTOR *end, s32 *rx, s32 *ry);
extern short FUN_8002fd9c(Humanoid *h);
extern int FUN_80039ddc(VECTOR *a, VECTOR *b, int c, int d);


void SetCameraMode(s32 mode)
{
    VECTOR va;
    VECTOR vb;
    VECTOR vc;
    VECTOR vd;
    long flag;
    s32 rx;
    s32 ry;
    TCameraStatus *cs;
    SVECTOR (*tbl)[4];
    long *fp;
    VECTOR *pos;
    SVECTOR *rot;
    SVECTOR *p;
    VECTOR *pv;
    s32 n;
    s32 i;
    s32 hitf;

    switch (mode) {
    case 4:
        n = rand();
        CamState.OldMode = n % 4;
        i = 0;
        cs = &CamState;
        tbl = D_80089F50;
        fp = &flag;
    loop:
        if (!(i < 4)) goto giveup;
        cs->OldMode = cs->OldMode + 1;
        if (cs->OldMode > 3) cs->OldMode = 0;
        p = (SVECTOR *)(cs->OldMode * 0x20 + (s32)tbl);
        pos = cs->Owner->locate;
        rot = cs->Owner->rotate;
        do {
            scratch_rot_1f800040.vx = rot->vx + FUN_8002fd9c(cs->Owner);
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
        do {
            RotTrans(p, &va, fp);
            RotTrans(p + 1, &vb, fp);
            RotTrans(p + 2, pv = &vc, fp);
            RotTrans(p + 3, pv = &vd, fp);
        } while (0);
        pv = 0;
        hitf = FUN_80039ddc(&vc, &vd, 0, 0) > 0x7ff;
        i++;
        if (hitf) goto hit;
        goto loop;
    giveup:
        CamState.OldMode = 0;
        break;
    case 15:
        CamState.DirectionRX = 0;
        CamState.DirectionRY = 0;
        CamState.Mode = 1;
        CamState.OldMode = mode;
        break;
    case 1:
    case 3:
        if (CamState.Mode != 1 && CamState.Mode != 13) {
            GetVectorRotation((VECTOR *)&ViewInfo, (VECTOR *)&ViewInfo.vrx, &rx, &ry);
            rx = rx - CamState.Owner->model->rotate.vx;
            ry = ry - CamState.Owner->model->rotate.vy;
            rx = (rx + 0x2800) % 0x1000 - 0x800;
            ry = (ry + 0x2800) % 0x1000 - 0x800;
            CamState.DirectionRX = 0;
            CamState.DirectionRY = ry;
        }
        CamState.Mode = 1;
        CamState.OldMode = mode;
        break;
    case 0:
        if (CamState.Owner->pad.data & 4) {
            SetCameraMode(1);
            return;
        }
        CamState.Mode = mode;
        break;
    hit:
        cs->Mode = mode;
        cs->CriticalHit = 1;
        break;
    default:
        CamState.Mode = mode;
        break;
    }
}
