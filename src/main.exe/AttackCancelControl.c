#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AttackCancelControl(short mode);
 *     MOTION.C:726, 24 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     param $a0       short mode
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 * END PSX.SYM */

/*
 * AttackCancelControl (0x8002736c, 0x17c bytes) — on cancelling an attack
 * (mode bit 0), delete the conflict volume(s) of whichever weapon
 * ornament(s) the current `weapon_kind` implies are active, then (mode bit
 * 1) drop any live afterimages; always clamps `dtM->mask` to 0x7fff.
 *
 * `weapon_kind` (item.h: u16 @0x8E) is read `lh` (SIGNED) here — the
 * per-TU-divergent load-width precedent already established for
 * lifemax/attrib; cast at this divergent use.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - **This IS a genuine `switch (wk) { case 2: ...; case 3: ...; case 0:
 *    goto skip_mode2; default: ...; }`, not an if/goto ladder** — despite
 *    Ghidra rendering it as literal nested if/else-if/goto (which LOOKS
 *    identical in body order) and despite the test sequence NOT resembling
 *    a balanced compare tree at first glance (2, then <3, then ==3). Ghidra's
 *    if/goto form and an explicit `if(wk<3){if(wk==0)goto…;}else if(wk==3)`
 *    ladder both compile to a DIFFERENT, 4-insns-longer shape (cc1 inverts
 *    which side of the `wk<3` test is the fallthrough vs the branch target,
 *    unpredictably relative to naive if/else codegen) — only spelling it as
 *    a real `switch` reproduces the exact test+body layout. Values {0,2,3}
 *    plus `default` (covering 1 and anything else) is the tell; case 0's
 *    body is a bare `goto` OUT of the switch (skipping the shared tail
 *    entirely), not a `break`.
 *  - Two DIFFERENT paths reach the SAME `skip_mode2` label — case 0's early
 *    `goto` and the outer `mode&1==0` skip — one C label serves both; the
 *    asm's apparent "two entry points" (one recomputing `mode&2` fresh, one
 *    reusing a copy precomputed in the other path's delay slot) falls out
 *    of reorg automatically, not from writing two labels.
 */

extern Humanoid *Me_MOTION_C;
extern MotionManager *dtM;
extern void DisposeAfterimage(void *afi);

void AttackCancelControl(s16 mode)
{
    s16 wk;
    ModelType *model;

    if ((mode & 1) != 0)
    {
        wk = (s16)Me_MOTION_C->weapon_kind;
        switch (wk)
        {
        case 2:
            DeleteConflict(Me_MOTION_C->model->object[8]);
            model = Me_MOTION_C->model->object[0xB];
            break;
        case 3:
            model = Me_MOTION_C->model->object[2];
            break;
        case 0:
            goto skip_mode2;
        default:
            DeleteConflict(Me_MOTION_C->model->object[0xD]);
            model = Me_MOTION_C->model->object[0xE];
            break;
        }
        DeleteConflict(model);
    }
skip_mode2:
    if ((mode & 2) != 0)
    {
        if (Me_MOTION_C->illusion[0] != 0)
        {
            DisposeAfterimage(Me_MOTION_C->illusion[0]);
            Me_MOTION_C->illusion[0] = 0;
        }
        if (Me_MOTION_C->illusion[1] != 0)
        {
            DisposeAfterimage(Me_MOTION_C->illusion[1]);
            Me_MOTION_C->illusion[1] = 0;
        }
    }
    dtM->mask = 0x7FFF;
}
