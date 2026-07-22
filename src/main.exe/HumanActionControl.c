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
 *     extern short motMODE;
 *     extern struct SVECTOR *dtV;
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern struct MotionManager *dtM;
 *     extern short motID;
 *     extern void (*ActionFunc[18])();
 * END PSX.SYM */

/*
 * HumanActionControl (0x8001c80c, 0x124 bytes) — the per-humanoid, per-frame
 * top-level driver: snapshots the pad, latches the "d-globals" (dtL/dtR/dtV/
 * dtM/motID) other MOTION.C functions read, runs whichever of
 * DamageControl/FallCheck-then-{HangCheck,SwimCheck} applies, sanitizes a
 * jump-pad bit out of dtPAD, dispatches through `ActionFunc[human->status]`
 * (an indirect call through a proven 18-entry function-pointer table), and
 * finally runs MotionAndMove() unless the dispatched handler left
 * `motMODE` at its reset sentinel (-1).
 *
 * THREE struct-field reads in this TU are UNSIGNED (`lhu`) against fields
 * item.h already proves SIGNED (`s16`) in other TUs — the same per-TU
 * load-width divergence as lifemax/attrib elsewhere: `human->attribute`
 * (@0x4), `human->map.attrib` (@0x28), and `human->motion->mid` (MotionManager
 * @0x0). A value-level `(u16)` cast does NOT change the load width — only
 * re-typing the memory operand itself does. But the TWO spellings that do
 * that are NOT interchangeable, and picking the wrong one costs 11 bytes:
 *
 *   - `*(u16 *)&field` is an INDIRECT_REF: it clears the RTL memory
 *     operand's MEM_IN_STRUCT_P (`/s`) marker.
 *   - `((View *)p)->field` stays a COMPONENT_REF: it KEEPS `/s`.
 *
 * `/s` is load-bearing here because gcc 2.8.1 `sched.c`'s `anti_dependence()`
 * dismisses a load->store dependence only when the LOAD is in-struct with a
 * varying address and the STORE is a non-struct fixed address. All five
 * d-global stores are non-struct fixed-address (`sw v0,%gp_rel(dtV)`), so a
 * `/s` load never pins them, but a cast load does. `mid` is read BEFORE the
 * `dtV` store, so spelling it `*(u16 *)&motion->mid` hands insn `sw dtV` a
 * REG_DEP_ANTI on the mid load, which (a) forbids the store from issuing
 * above it and (b) raises the store's sched priority from 3 to 4, tying it
 * with the `andi` — and sched.c's equal-priority `potential_hazard` tiebreak
 * then hands the slot to the memory-unit store. The MotionManagerU view kills
 * that anti-dep. Only `sw dtV` is demoted by this: dtL/dtR/dtM each consume a
 * *load* result (load-use cost 2 -> priority 4 regardless), while dtV consumes
 * the `addiu`'s result (cost 1 -> priority 3). See docs/matching-cookbook.md,
 * "Reading cc1's RTL dumps".
 *
 * `attribute`/`attrib` are read but never stored, so their `/s` marker is
 * measured NEUTRAL (a struct view for `attr` also matches); they keep the
 * cheaper cast spelling rather than invent a padded Humanoid view. Only
 * `mid` — the one cast load that precedes a d-global store — is load-bearing.
 */

/* TU-local UNSIGNED view of MotionManager's leading `mid` (item.h keeps the
 * field `s16`: every other TU reads it signed, e.g. `(short)(dtM->mid - 0x100)`
 * in ActACTION.c). Reading through this view is what gives MOTION.C's `lhu`
 * while keeping the access a COMPONENT_REF — see the note above; the
 * `*(u16 *)&motion->mid` cast spelling costs 11 bytes. */
typedef struct
{
    u16 mid;                     /* 0x0 */
} MotionManagerU;

extern Humanoid *Me_MOTION_C;
extern u16 dtPAD;
extern s16 dtCMD;
extern s16 motID;
extern s16 motMODE;
extern void (*ActionFunc[18])(void);
extern short GetCommand(PADtype *pad);
extern s16 FallCheck(void);
extern short HangCheck(void);
extern short SwimCheck(void);
extern void DamageControl(void);
extern short MotionAndMove(void);

/*
 * The `locate`/`motion`/`rotate`/`mid`/`attr`/`dtV` STATEMENT ORDER below is
 * load-bearing: it reproduces the target's register-coloring shape. The base
 * pointer reload lands in the SAME hard reg as the just-consumed `GetCommand`
 * return value (reused, not a fresh register) and survives through
 * locate/motion/rotate/attr to be overwritten IN PLACE by the `dtV` address
 * computation, exactly like the target's `addiu v0,v0,64`. Moving the `dtV`
 * assignment above `mid` also fixes the schedule but lengthens the base's live
 * range past the `addiu` (sched1 then sinks `rotate` below the store), which
 * breaks that coalesce and costs 31 bytes.
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
    motMODE = -1;
    locate = Me_MOTION_C->locate;
    motion = Me_MOTION_C->motion;
    rotate = Me_MOTION_C->rotate;
    mid = ((MotionManagerU *)motion)->mid;
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
            if ((*(u16 *)&Me_MOTION_C->map.attrib & 4) != 0)
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
    if (motMODE != -1)
    {
        MotionAndMove();
    }
}
