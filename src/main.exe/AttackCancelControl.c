#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "afterimage.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackCancelControl(short mode);
 *     MOTION.C:726, 24 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       short mode
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 * END PSX.SYM */

/*
 * AttackCancelControl (0x8002736c, 0x17c bytes) — on cancelling an attack
 * (ATTACK_CANCEL_CONFLICTS), delete the conflict volume(s) of whichever
 * weapon ornament(s) the current `wpatk` implies are active, then
 * (ATTACK_CANCEL_AFTERIMAGES) drop any live afterimages; always stores
 * MOTION_MASK_ALL into `dtM->mask`.
 *
 * `wpatk` (item.h: s16 @0x8E) is read with the expected signed `lh`.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - **This IS a genuine `switch (weapon) { case 2: ...; case 3: ...; case 0:
 *    goto no_conflict; default: ...; }`, not an if/goto ladder** — despite
 *    Ghidra rendering it as literal nested if/else-if/goto (which LOOKS
 *    identical in body order) and despite the test sequence NOT resembling
 *    a balanced compare tree at first glance (2, then <3, then ==3). Ghidra's
 *    if/goto form and an explicit `if(wk<3){if(wk==0)goto…;}else if(wk==3)`
 *    ladder both compile to a DIFFERENT, 4-insns-longer shape (cc1 inverts
 *    which side of the `weapon<3` test is the fallthrough vs the branch target,
 *    unpredictably relative to naive if/else codegen) — only spelling it as
 *    a real `switch` reproduces the exact test+body layout. Values {0,2,3}
 *    plus `default` (covering 1 and anything else) is the tell; case 0's
 *    body is a bare `goto` OUT of the switch (skipping the shared tail
 *    entirely), not a `break`.
 *  - Two DIFFERENT paths reach the SAME `no_conflict` label — case 0's early
 *    `goto` and the outer `mode&1==0` skip — one C label serves both; the
 *    asm's apparent "two entry points" (one recomputing the afterimage bit,
 *    one reusing a copy precomputed in the other path's delay slot) falls
 *    out of reorg automatically, not from writing two labels.
 */

extern Humanoid *Me_MOTION_C;

void AttackCancelControl(s16 mode)
{
    weapon_kind weapon;
    ModelType *model;

    if ((mode & ATTACK_CANCEL_CONFLICTS) != 0)
    {
        weapon = Me_MOTION_C->wpatk;
        switch (weapon)
        {
        case FIST:
            DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_0]);
            model = Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_1];
            break;
        case JAW:
            model = Me_MOTION_C->model->object[MODEL_PART_BEAST_HAND_0];
            break;
        case NO_WEAPON:
            goto no_conflict;
        default:
            DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0]);
            model = Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_1];
            break;
        }
        DeleteConflict(model);
    }
no_conflict:
    if ((mode & ATTACK_CANCEL_AFTERIMAGES) != 0)
    {
        if (Me_MOTION_C->illusion[WEAPON_HAND_0] != 0)
        {
            DisposeAfterimage(Me_MOTION_C->illusion[WEAPON_HAND_0]);
            Me_MOTION_C->illusion[WEAPON_HAND_0] = 0;
        }
        if (Me_MOTION_C->illusion[WEAPON_HAND_1] != 0)
        {
            DisposeAfterimage(Me_MOTION_C->illusion[WEAPON_HAND_1]);
            Me_MOTION_C->illusion[WEAPON_HAND_1] = 0;
        }
    }
    dtM->mask = MOTION_MASK_ALL;
}
