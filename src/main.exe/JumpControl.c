#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void JumpControl(void);
 *     MOTION.C:367, 37 src lines, frame 24 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct VECTOR *dtL;
 *     extern struct MotionManager *dtM;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct Humanoid *StagePlayer;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct SVECTOR *dtV;
 *     extern short dtPAD;
 * END PSX.SYM */

extern Humanoid *Me_MOTION_C;

extern void spawn_smoke_burst_(VECTOR *pos, u16 spread, s16 divisor, s16 count);

void JumpControl(void)
{
    int id;

    spawn_smoke_burst_(dtL, 150, SMOKE_DRIFT_DIVISOR_DEFAULT, 8);
    if (GetMotionID(dtM, MOT_JUMP) < 0)
        return;

    if (motID == MOT_CHASE_DASH_FWD)
    {
        if (dtM->count < 11 &&
            GetMotionID(dtM, MOT_JUMP_RUN) >= 0)
        {
            SET_MOTION(MOT_JUMP_RUN, MOTION_MOVE_NONE);
            MoveHumanoid(Me_MOTION_C, RUN_JUMP_SPEED, 0);
            if (Me_MOTION_C == StagePlayer)
            {
                Sound(Me_MOTION_C, SE_JUMP_IMPACT);
            }
            Sound(Me_MOTION_C, SE_JUMP_MOVE);
        }
    }
    else
    {
        id = Me_MOTION_C->model->object[MODEL_PART_WAIST]->id;
        if (id >= 0)
        {
            dtL->vx = ConflictObject[id].position.vx;
            dtL->vz = ConflictObject[id].position.vz;
        }
        SET_MOTION(MOT_JUMP, MOTION_MOVE_NONE);
        dtV->vy = 0;
        if (dtPAD & PADLup)
        {
            if (GetMotionID(dtM, MOT_JUMP_FORWARD) >= 0)
            {
                SET_MOTION(MOT_JUMP_FORWARD, MOTION_MOVE_NONE);
            }
            MoveHumanoid(Me_MOTION_C, 100, 0);
        }
        else if (dtPAD & PADLdown)
        {
            if (GetMotionID(dtM, MOT_JUMP_BACK) >= 0)
            {
                SET_MOTION(MOT_JUMP_BACK, MOTION_MOVE_NONE);
            }
            MoveHumanoid(Me_MOTION_C, -100, 0);
        }
        else if (dtPAD & PADLright)
        {
            if (GetMotionID(dtM, MOT_JUMP_RIGHT) >= 0)
            {
                SET_MOTION(MOT_JUMP_RIGHT, MOTION_MOVE_NONE);
            }
            MoveHumanoid(Me_MOTION_C, 0, -100);
        }
        else if (dtPAD & PADLleft)
        {
            if (GetMotionID(dtM, MOT_JUMP_LEFT) >= 0)
            {
                SET_MOTION(MOT_JUMP_LEFT, MOTION_MOVE_NONE);
            }
            MoveHumanoid(Me_MOTION_C, 0, 100);
        }
        else
        {
            dtV->vz = 0;
            dtV->vx = 0;
        }
    }
}
