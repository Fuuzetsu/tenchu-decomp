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
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 *  - The illusion-disposal block is wrapped in
 *    `if (mode & ATTACK_CANCEL_AFTERIMAGES)`, exactly AttackCancelControl's
 *    parameter test, but here `mode` is a LOCAL that only ever holds
 *    ATTACK_CANCEL_ALL (set on case 0's early exit and again right after the
 *    shared `DeleteConflict(model);`) — never a real parameter. cc1 doesn't
 *    constant-fold the known-true test away (that requires a literal at the
 *    expression site, not a variable merely known-constant by dataflow), so
 *    the dead andi+beqz survives in the binary exactly as Ghidra's own
 *    decompilation (which DOES prove it dead and renders a bare `return 1;`)
 *    hides.
 *  - `Me_MOTION_C->pad.time = 0;` must be written BEFORE `wk =
 *    Me_MOTION_C->wpatk;`, not after (Ghidra's own literal order). Keeping
 *    the signed field load adjacent to the switch dispatch lets combine
 *    select the target's single `lh`; an intervening store changes the
 *    generated instruction sequence.
 */

extern Humanoid *Me_MOTION_C;

s16 AttackContinuousCheck(BattleType *battle)
{
    s16 wk;
    ModelType *model;
    s16 mode;

    if (dtM->count < battle->contfrm - 3)
    {
        return 0;
    }
    if (battle->contfrm + 3 < dtM->count)
    {
        return 0;
    }
    Me_MOTION_C->pad.time = 0;
    wk = Me_MOTION_C->wpatk;
    switch (wk)
    {
    case FIST:
        DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_0]);
        model = Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_1];
        break;
    case JAW:
        model = Me_MOTION_C->model->object[MODEL_PART_BEAST_HAND_0];
        break;
    case NO_WEAPON:
        mode = ATTACK_CANCEL_ALL;
        goto no_conflict;
    default:
        DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0]);
        model = Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_1];
        break;
    }
    DeleteConflict(model);
    mode = ATTACK_CANCEL_ALL;
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
    return 1;
}
