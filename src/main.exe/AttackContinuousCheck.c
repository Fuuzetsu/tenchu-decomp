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
