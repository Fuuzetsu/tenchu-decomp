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
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
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
 * STATUS: NON_MATCHING — 26 of 632 bytes differ (13 instructions), pure
 * register-allocation coloring tie below the C level. The whole-image diff is
 * exactly those 26 bytes (the function is the correct LENGTH and every later
 * object is byte-identical), so the arithmetic/structure is right — only the
 * register colors differ. autorules found no win; the decomp-permuter was run
 * twice (~3400 iters then a full 600s bounded run, -j4 --best-only) and never
 * reached score 0 (best 210 vs base 580, and its score ignores stack slots).
 *
 * Root cause — ONE coloring decision cascades into all 13 diffs. In the
 * min `mmp->n = min(mmp->model->n, mmp->motion->n)`, the target colors the
 * result `i` to $a0 (coalesced with the `lh model->n` that loads into $a0) and
 * keeps `motion->n` alive in $v1 across the `slt` (whose result goes to $v0),
 * so the taken branch is a free `move a0,v1` with no reload. Our cc1 instead
 * keeps the `mmp->motion` POINTER alive in $a0, loads `motion->n` into $v0
 * (clobbered by the `slt`), and colors `i` to $a2 — forcing a reload
 * (`lbu a2,0(a0)`) in the branch. That single a0-vs-a2 choice then dictates:
 *   - the delay-slot fill of the SetupSpline call (target must store mmp->n
 *     BEFORE `move a0,s0` because `i` is in $a0; ours is free to put the store
 *     in the delay slot because `i` is in $a2);
 *   - the bone-fixup `negu` source ($v0 the copy vs $v1 the original — a CSE
 *     artifact, `at=-at` folds back to `negu v0,v1` regardless of source form);
 *   - the fixup store registers (target moves the address to $v0 and stores the
 *     value from $v1 reusing t's register; ours keeps the address in $a0 and
 *     the value in $v0).
 * None of these is reachable from C: verified by trying the min against the
 * field vs the local, an explicit s16/s32 `mn` temp (added an extra move and a
 * 4-byte length shift), reusing `j`, and both abs spellings — all identical or
 * worse. This is the reload/coloring class the cookbook marks permuter-immune.
 *
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
 */

extern MotionRegistType MOTcommon[41];
extern void SetupSpline(MotionManager *mmp);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/UpdateMotion", UpdateMotion);
#else
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
    i = mmp->model->n;
    if (mmp->motion->n < i)
        i = mmp->motion->n;
    mmp->n = i;
    SetupSpline(mmp);

    for (i = 0; i < mmp->model->n; i++)
    {
        xyz = (s16 *)&mmp->model->object[i]->rotate;
        for (j = 0; j < 3; j++)
        {
            t = xyz[j];
            at = (t < 0) ? -t : t;
            if (at > 0x800)
            {
                if (t < 0)
                    xyz[j] = t + 0x1000;
                else
                    xyz[j] = t - 0x1000;
            }
            t = xyz[j];
            at = t;
            if (t < 0)
                at = t + 0xFFF;
            xyz[j] = t - ((at >> 12) << 12);
        }
    }
    return 1;
}
#endif
