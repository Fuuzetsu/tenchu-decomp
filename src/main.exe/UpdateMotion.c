#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short UpdateMotion(struct MotionManager *mmp, short mid);
 *     ACTION.C:182, 34 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $s0       struct MotionManager * mmp
 *     param $t0       short mid
 *     reg   $a0       struct MotionRegistType * mrp
 *     reg   $a2       short i
 *     reg   $a1       short j
 *     reg   $a3       short * xyz
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionRegistType MOTcommon[41];
 * END PSX.SYM */

/*
 * UpdateMotion (0x8001b65c, 0x278 bytes) — switch mmp's active motion to
 * `mid`, unless it's already the current motion (returns -1, a no-op).
 * Search mmp's own registered-motion table (mmp->motreg, sentinel
 * mid == -1) for a row matching `mid`; if none has a non-NULL `motion`
 * there, fall back to searching the global common table MOTcommon the same
 * way. If neither table has a usable row, returns 0 (failure). Otherwise
 * installs the found MotionDataType* as mmp->motion, latches mmp->mid,
 * derives mmp->count (a signed byte, motion->sweep sign-extended) and
 * mmp->loop = 0, clamps mmp->n to min(mmp->model->n, mmp->motion->n), calls
 * SetupSpline(mmp) to reseed the interpolation state, then re-normalizes
 * every bone's rotation (mmp->model->object[i]->rotate, cast to a short[3]
 * array of vx/vy/vz) into canonical range: first snap a value that
 * overshot by nearly a half turn back a full turn (abs > 0x800 -> +-0x1000),
 * then wrap it into 0..0xFFF via a floor-divide-by-0x1000 subtract. Returns
 * 1 on success.
 *
 * Matching notes:
 *  - Same duplicate_loop_exit_test bottom-tested search shape as
 *    GetMotionID.c/GetAttackDBID.c, but with the two sub-conditions in the
 *    OPPOSITE priority: the entry/bottom test here is `mrp[i].mid != mid`
 *    (continue searching) and the in-body break check is
 *    `mrp[i].mid == -1` (hit the sentinel without a match) — i.e.
 *    `while (mrp[i].mid != mid) { if (mrp[i].mid == -1) break; i++; }`,
 *    not GetMotionID's `while (reg[i].mid != -1) { if (reg[i].mid == mid)
 *    break; i++; }`. Confirmed by reading both loops' actual entry-test
 *    register (which value each `beq` compares against first).
 *  - `mrp` is reused for BOTH searches (own table, then MOTcommon) — same
 *    single-pointer-variable-reused shape as SearchMotion.c's `mpd`.
 *  - `i` is reused across all three loops in the function (both searches,
 *    then the bone loop); `mmp->model->n` vs `mmp->motion->n` (a `u8`,
 *    zero-extended by the `lbu`) are min'd into it before being stashed
 *    into mmp->n — no separate temp local exists (matches PSX.SYM's 4
 *    extra locals: mrp/i/j/xyz only), so the min is written directly
 *    against `i`, reused as scratch between the two search loops and the
 *    bone loop.
 *  - Each bone's rotation base is computed in one expression,
 *    `(s16 *)&mmp->model->object[i]->rotate` (ModelType.rotate @ 0x50) —
 *    no separate ModelType* local, matching PSX.SYM again.
 *  - The two per-component (x/y/z, `j` in 0..2) fixups are each a single
 *    self-referencing statement (`xyz[j] = ...xyz[j]...`): cc1 loads
 *    xyz[j] once and reuses the register for every use on the RHS before
 *    the one store, exactly like the asm's single `lh`/single `sh` per
 *    fixup (two `lh`s total per component, one per statement).
 *  - The min MUST be the ternary `(motion->n < model->n) ? motion->n :
 *    model->n` — fold/expr.c's min/max path loads BOTH operands into SI
 *    registers first, then `i = op_model; if (motion < model) i = op_motion`
 *    where the conditional arm is a register MOVE (`move a0,v1`). The
 *    two-statement form (`i = model->n; if (motion->n < i) i = motion->n;`)
 *    re-expands the memory read in the arm as a zero_extend:HI (i is s16) that
 *    cannot CSE with the compare's zero_extend:SI — a reload (`lbu`) plus a
 *    whole different coloring (13 instructions). This one spelling fixed 22 of
 *    the 26 differing bytes.
 *  - The abs MUST be spelled INLINE in the comparison —
 *    `if (((t < 0) ? -t : t) > 0x800)` — so it becomes ABS_EXPR -> the MIPS
 *    `abssi2` insn: ONE opaque insn whose output template is exactly
 *    `bgez;move;subu %0,$0,%0` (negu of the COPY, numeric `1:` label).
 *    Assigning the same ternary to a named temp (`at = (t<0) ? -t : t;`) goes
 *    through expr.c's COND_EXPR singleton path instead: separate copy/branch/
 *    neg-in-place insns that cse's canon_reg immediately rewrites to
 *    `negu v0,v1` (operand canonicalized to the class's first reg — the source
 *    variable always outlives the temp, so the temp can never win
 *    make_regs_eqv's qty_first promotion).
 *  - The +-0x1000 snap is ONE assignment whose ternary arms update t IN PLACE
 *    and whose CONDITION re-reads the memory:
 *    `xyz[j] = (xyz[j] < 0) ? (t += 0x1000) : (t -= 0x1000);`
 *    Three things hang on this exact spelling: the compound arms give the
 *    in-place `addiu v1,v1,+-0x1000`; the single assignment expands the
 *    destination address BEFORE the branch (expand_assignment computes to_rtx
 *    first), which cse folds into a pre-branch copy of the lh's address
 *    (`move v0,a0`, stolen into the bgez delay slot) with the store through
 *    the copy (`sh v1,0(v0)`); and the mem-read in the CONDITION (not `t < 0`,
 *    byte-identical semantics since t == xyz[j] here) shifts the pre-cse
 *    reference structure so cse1's taken-path window keeps the copy alive in
 *    both arms instead of canonicalizing the store back to `0(a0)` (found by
 *    the permuter after hand analysis pinned everything but this last cell;
 *    `t < 0` in the condition = 4 bytes off: nop instead of the move, sh via
 *    a0).
 */

extern void SetupSpline(MotionManager *mmp);

s16 UpdateMotion(MotionManager *mmp, s16 mid)
{
    MotionRegistType *mrp;
    MotionDataType *md;
    s16 i;
    s16 j;
    s16 *xyz;
    s32 t;
    s16 sweep;
    s32 at;

    if (mid == mmp->mid)
        return -1;

    mrp = mmp->motreg;
    i = 0;
    while (mrp[i].mid != mid)
    {
        if (mrp[i].mid == -1)
            break;
        i++;
    }
    if (mrp[i].motion == 0)
    {
        mrp = MOTcommon;
        i = 0;
        while (mrp[i].mid != mid)
        {
            if (mrp[i].mid == -1)
                break;
            i++;
        }
        if (mrp[i].motion == 0)
            return 0;
    }

    md = mrp[i].motion;
    mmp->mid = mid;
    mmp->motion = md;
    sweep = md->sweep;
    mmp->count = sweep;
    if (sweep & 0x80)
        mmp->count = sweep - 0x100;
    mmp->loop = 0;
    i = (mmp->motion->n < mmp->model->n) ? mmp->motion->n : mmp->model->n;
    mmp->n = i;
    SetupSpline(mmp);

    for (i = 0; i < mmp->model->n; i++)
    {
        xyz = (s16 *)&mmp->model->object[i]->rotate;
        for (j = 0; j < 3; j++)
        {
            t = xyz[j];
            if (((t < 0) ? -t : t) > 0x800)
                xyz[j] = (xyz[j] < 0) ? (t += 0x1000) : (t -= 0x1000);
            t = xyz[j];
            at = t;
            if (t < 0)
                at = t + 0xFFF;
            xyz[j] = t - ((at >> 12) << 12);
        }
    }
    return 1;
}
