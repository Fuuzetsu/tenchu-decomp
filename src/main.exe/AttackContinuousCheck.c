#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "afterimage.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short AttackContinuousCheck(struct BattleType *battle);
 *     MOTION.C:755, 12 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
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
 *     param $a0       struct BattleType * battle
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 * END PSX.SYM */

/*
 * AttackContinuousCheck (0x8001f180, 0x1A4 bytes) — gate a continuous-attack
 * follow-up by the current motion frame against BattleDB's per-move window
 * [contfrm-3, contfrm+3], then run the exact same wpatk-dispatched
 * conflict-volume cleanup + afterimage drop as AttackCancelControl (same
 * switch shape, same field offsets) before returning 1.
 *
 * Matching notes (docs/matching-cookbook.md):
 *  - Same genuine `switch (wk) { case 2: ...; case 3: ...; case 0: goto...;
 *    default: ...; }` shape as AttackCancelControl — see that file's header.
 *  - The illusion-disposal block is wrapped in `if (mode & 2)`, exactly
 *    AttackCancelControl's parameter test, but here `mode` is a LOCAL that
 *    only ever holds the literal 3 (set on case 0's early exit and again
 *    right after the shared `DeleteConflict(model);`) — never a real
 *    parameter. cc1 doesn't constant-fold `if (3 & 2)` away (that requires
 *    a literal at the expression site, not a variable merely known-constant
 *    by dataflow), so the dead andi+beqz survives in the binary exactly as
 *    Ghidra's own decompilation (which DOES prove it dead and renders a
 *    bare `return 1;`) hides.
 *  - `Me_MOTION_C->pad.time = 0;` must be written BEFORE `wk =
 *    Me_MOTION_C->wpatk;`, not after (Ghidra's own literal order). Keeping
 *    the signed field load adjacent to the switch dispatch lets combine
 *    select the target's single `lh`; an intervening store changes the
 *    generated instruction sequence.
 */

extern Humanoid *Me_MOTION_C;
extern MotionManager *dtM;

s16 AttackContinuousCheck(BattleType *battle)
{
    s16 wk;
    ModelType *model;
    s16 mode;

    if (dtM->count < battle->contfrm - 3) {
        return 0;
    }
    if (battle->contfrm + 3 < dtM->count) {
        return 0;
    }
    Me_MOTION_C->pad.time = 0;
    wk = Me_MOTION_C->wpatk;
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
        mode = 3;
        goto skip_mode2;
    default:
        DeleteConflict(Me_MOTION_C->model->object[0xD]);
        model = Me_MOTION_C->model->object[0xE];
        break;
    }
    DeleteConflict(model);
    mode = 3;
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
    return 1;
}
