#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActHANG(void);
 *     MOTION.C:1663, 45 src lines, frame 32 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     reg   $s0       long y
 *
 * Globals it touches, as the original declared them:
 *     extern struct SVECTOR *dtV;
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern struct VECTOR *dtL;
 *     extern short motID;
 *     extern short motMODE;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

/*
 * ActHANG (0x80024748, 0x2bc bytes incl. jump table) — the ledge-hang action
 * state (MOTION.C's ActionFunc[] table). Motion ids 0xA00..0xA04: 0xA00
 * (hanging) climbs up while UP is held (raise dtL->vy by 100 until HangCheck
 * fails -> motion 0x803 falling), or starts a shimmy left (0xA02, LEFT) /
 * right (0xA03, RIGHT), or checks the ledge above (DOWN? 0x1000) via
 * GetAreaMapLevel before pulling up (0xA04); 0xA01 (reach) returns to 0xA00
 * when the motion runs out; 0xA02/0xA03 (shimmy) keep moving while LEFT/RIGHT
 * held else return to 0xA00, falling (0x803) if the grip breaks; 0xA04
 * (pull-up) ends in stand (0) or crouch (0x501, attribute & 0x40) and RETURNS
 * (skipping the shared tail). Shared tail: attribute & 0x8000 (damaged?)
 * knocks the character off the wall.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - A real jump-table `switch` on `(short)(dtM->mid - 0xA00)`: the (short)
 *    cast narrows the subtract to HImode — `lhu` (raw load) + HImode `addiu`
 *    + `sll/sra` re-widen, then expand_case's `sltiu 5` + table jump. A plain
 *    `switch (dtM->mid)` with 0xA00-based cases would subtract in SImode off
 *    an `lh` (no sll/sra) — wrong shape.
 *  - Case bodies are emitted in SOURCE order; the original's order is the
 *    MEMORY order 0, 2/3, 4, 1 (case 1 last, falling into the shared tail;
 *    its `motID = 0xA00; motMODE = 1;` island is the physically-last copy
 *    that case 0's 0xA02/0xA03 stores cross-jump onto, leaving each
 *    predecessor just its own `li` in the branch delay slot).
 *  - `motMODE = 1;` written literally per arm (never hoisted/shared);
 *    cc1's cross-jump does all the merging (same rule as ActSYURI.c).
 *  - dtPAD (u16) is `lhu`-loaded for the case-0 mask tests but the original
 *    spells case 2/3's test on a SIGNED view — `((s16)dtPAD & 0xA000)` gives
 *    the target's `lh`; attribute@0x4 likewise diverges per-site: `*(u16 *)&`
 *    for case 4's `& 0x40` (lhu) vs the plain s16 field for the tail's
 *    `& 0x8000` (lh).
 *  - Case 4's double `dtM->count` read reuses ONE dtM load across the
 *    `dtV->vy = -0x23` store (cse.c's MEM_IN_STRUCT_P heuristic: a varying
 *    struct store does not invalidate a fixed-address non-struct scalar
 *    load), while a multi-pred label forces the fresh dtM loads elsewhere.
 *  - The climb loop is a plain bottom-tested `do { } while (HangCheck());`
 *    (`y` = PSX.SYM's `long y`, callee-saved across the call); the case-0
 *    GetAreaMapLevel `1` (5th arg) and `motMODE = 1` unify into one
 *    call-crossing $s0 pseudo via cse, no named variable.
 *  - gp-externs: dtV, dtM, dtPAD, dtL, motID, motMODE, Me_MOTION_C.
 *    StagePlayer/GlobalAreaMap stay absolute.
 */

extern SVECTOR *dtV;
extern MotionManager *dtM;
extern u16 dtPAD;
extern VECTOR *dtL;
extern s16 motID;
extern s16 motMODE;
extern Humanoid *Me_MOTION_C;
extern Humanoid *StagePlayer;
extern u32 *GlobalAreaMap;

extern short HangCheck(void);
extern long GetAreaMapLevel(unsigned long *area, long x, long y, long z, int mode);
extern void Sound(Humanoid *h, int id);
extern void SetCameraMode(s32 mode);
extern void MoveHumanoid(Humanoid *h, short a, short b);

void ActHANG(void)
{
    long y;

    dtV->vy = 0;
    switch ((short)(dtM->mid - 0xA00))
    {
    case 0:
        if (dtPAD & 0x4000)
        {
            y = dtL->vy;
            do
            {
                y += 100;
                dtL->vy = y;
            } while (HangCheck() != 0);
            motID = 0x803;
            motMODE = 0;
        }
        else if (dtPAD & 0x2000)
        {
            motID = 0xA02;
            motMODE = 1;
        }
        else if (dtPAD & 0x8000)
        {
            motID = 0xA03;
            motMODE = 1;
        }
        else if ((dtPAD & 0x1000) && GetAreaMapLevel(GlobalAreaMap, dtL->vx, dtL->vy - 2000, dtL->vz, 1) != 0x80000000)
        {
            motID = 0xA04;
            motMODE = 1;
        }
        break;
    case 2:
    case 3:
        if (((s16)dtPAD & 0xA000) == 0)
        {
            motID = 0xA00;
            motMODE = 1;
        }
        else if (HangCheck() == 0)
        {
            motID = 0x803;
            motMODE = 0;
        }
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x1B);
        }
        break;
    case 4:
        if (dtM->count == 0 && dtM->loop != 0)
        {
            if (Me_MOTION_C == StagePlayer)
            {
                SetCameraMode(0);
            }
            if (*(u16 *)&Me_MOTION_C->attribute & 0x40)
            {
                motID = 0x501;
                motMODE = 1;
                return;
            }
            motID = 0;
            motMODE = 1;
            return;
        }
        if (dtM->count >= 0)
        {
            dtV->vy = -0x23;
            if (dtM->count > 0x28)
            {
                MoveHumanoid(Me_MOTION_C, 100, 0);
            }
        }
        break;
    case 1:
        if (dtM->count == 0 && dtM->loop != 0)
        {
            motID = 0xA00;
            motMODE = 1;
        }
        break;
    }
    if (Me_MOTION_C->attribute & 0x8000)
    {
        MoveHumanoid(Me_MOTION_C, -10, 0);
        motID = 0x803;
        motMODE = 0;
    }
}

