#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void HumanActionControl(struct Humanoid *human);
 *     MOTION.C:145, 21 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     param $a0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern short dtPAD;
 *     extern short dtCMD;
 *     extern struct SVECTOR *dtV;
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern struct MotionManager *dtM;
 *     extern short motID;
 * END PSX.SYM */

/*
 * HumanActionControl (0x8001c80c, 0x124 bytes) — the per-humanoid, per-frame
 * top-level driver: snapshots the pad, latches the "d-globals" (dtL/dtR/dtV/
 * dtM/motID) other MOTION.C functions read, runs whichever of
 * DamageControl/FallCheck-then-{HangCheck,SwimCheck} applies, sanitizes a
 * jump-pad bit out of dtPAD, dispatches through `ActionFunc[human->status]`
 * (an indirect call through a proven 18-entry function-pointer table), and
 * finally runs MotionAndMove() unless the dispatched handler left
 * `D_80097F0E` at its reset sentinel (-1).
 *
 * THREE struct-field reads in this TU are UNSIGNED (`lhu`) against fields
 * item.h already proves SIGNED (`s16`) in other TUs — the same per-TU
 * load-width divergence as lifemax/attrib elsewhere: `human->attribute`
 * (@0x4), `human->attrib` (@0x28), and `human->motion->mid` (MotionManager
 * @0x0). Force each with a `*(u16 *)&field` pointer-reinterpret cast (a
 * value-level `(u16)` cast does NOT change the load width — only re-typing
 * the memory operand itself does).
 */

extern Humanoid *Me_MOTION_C;
extern u16 dtPAD;
extern s16 dtCMD;
extern s16 motID;
extern SVECTOR *dtV;
extern VECTOR *dtL;
extern SVECTOR *dtR;
extern MotionManager *dtM;
extern s16 D_80097F0E;
extern void (*ActionFunc[18])(void);
extern short GetCommand(PADtype *pad);
extern s16 FallCheck(void);
extern void HangCheck(void);
extern void SwimCheck(void);
extern void DamageControl(void);
extern short MotionAndMove(void);

#ifndef NON_MATCHING
INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/HumanActionControl", HumanActionControl);
#else
/*
 * STATUS: NON_MATCHING — same length, 11 of 292 bytes differ (down from an
 * earlier 31-byte park; a stale header previously described a v0/v1 register
 * tie that no longer applies).
 *
 * The `locate`/`motion`/`rotate`/`mid`/`attr`/`dtV` STATEMENT ORDER below was
 * found by exhaustively trying all 720 orderings (respecting `mid` needing
 * `motion` first) via a byte-diff scorer, plus separately sliding `dtV`
 * through all 10 possible positions — this specific order is the unique
 * minimum. It reproduces the target's whole register-coloring shape: the base
 * pointer reload lands in the SAME hard reg as the just-consumed `GetCommand`
 * return value (reused, not a fresh register) and survives through
 * locate/motion/rotate/attr to be overwritten IN PLACE by the `dtV` address
 * computation (`addu $2,$2,64`), exactly like the target's `addiu v0,v0,64`.
 *
 * RESIDUAL (11 bytes): the target orders [dtV-store, mid-load, andi] but this
 * draft orders [mid-load, andi, dtV-store] — a pure instruction-scheduling
 * tie among three MUTUALLY INDEPENDENT ready instructions (a store with no
 * consumer, and a load+andi chain), matching the cookbook's documented open
 * problem "sched's equal-priority tiebreak prefers the memory-unit insn"
 * (FileOption case 0xd class). Tried and failed to move it: every statement
 * position for `dtV` (10 positions), every ordering of the other 5
 * assignments (720 combos, scored via a fast standalone cc1|maspsx|as loop),
 * and a bounded permuter run (300s, --stop-on-zero -j4, ~12k iterations)
 * that plateaued at this exact residual and never found better. `attr &=
 * 0x4000;` as an explicit statement (vs inlining `attr & 0x4000` in the
 * `if`) has zero effect either way — cc1's scheduler treats the AND as
 * freely movable regardless of where the source states it. Untried per the
 * cookbook's own note on this class: lengthening the store's downstream
 * dependence chain, or deriving the andi's operand from a later-completing
 * value, to break the two candidates' simultaneous readiness.
 */
void HumanActionControl(Humanoid *human)
{
    VECTOR *locate;
    MotionManager *motion;
    SVECTOR *rotate;
    u16 attr;
    s16 mid;

    dtPAD = human->pad.data;
    Me_MOTION_C = human;
    dtCMD = GetCommand(&human->pad);
    D_80097F0E = -1;
    locate = Me_MOTION_C->locate;
    motion = Me_MOTION_C->motion;
    rotate = Me_MOTION_C->rotate;
    mid = *(u16 *)&motion->mid;
    attr = *(u16 *)&Me_MOTION_C->attribute;
    dtV = &Me_MOTION_C->vector;
    dtL = locate;
    dtR = rotate;
    dtM = motion;
    motID = mid;
    if ((attr & 0x4000) != 0)
    {
        DamageControl();
    }
    else
    {
        if (FallCheck() != 0)
        {
            HangCheck();
        }
        else
        {
            if ((*(u16 *)&Me_MOTION_C->attrib & 4) != 0)
            {
                SwimCheck();
            }
        }
    }
    if ((dtPAD & 4) != 0)
    {
        dtPAD = dtPAD & 0xFFF;
    }
    ActionFunc[Me_MOTION_C->status]();
    if (D_80097F0E != -1)
    {
        MotionAndMove();
    }
}
#endif
