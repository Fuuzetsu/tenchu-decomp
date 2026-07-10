#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void HumanActionControl(struct Humanoid *human);
 *     MOTION.C:145, 21 src lines, frame 24 bytes, saved-reg mask 0x80000000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $a0       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern struct NodeIndexType *FieldIndex;
 *     extern short dtPAD;
 *     extern short dtCMD;
 *     extern short motID;
 *     extern struct SVECTOR *dtV;
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern struct MotionManager *dtM;
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
 * STATUS: NON_MATCHING — same length, ~12 instructions differ by register
 * assignment and schedule only.
 *
 * The target keeps `human` in $v1 and hoists the `attribute` halfword load
 * (`lhu 4($v1)`) above the three pointer loads; our draft caches through $v0 and
 * reads `attr` after them. Everything else — the gp stores of dtPAD/dtCMD/dtM,
 * the `andi 0x4000` test, the branch — is identical, so this is a pure
 * register-coloring / scheduling tie, not a source-structure miss.
 *
 * A long permuter run (agent batch, MOTION.C) did not beat the base. Statement
 * reordering of the `attr` read alone does not move the base register. Parked
 * per the sub-C early-stop protocol; `tools/regalloc.py HumanActionControl` is
 * the next thing to try if someone picks this up.
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
    attr = *(u16 *)&Me_MOTION_C->attribute;
    dtV = &Me_MOTION_C->vector;
    mid = *(u16 *)&motion->mid;
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
