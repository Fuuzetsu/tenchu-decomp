#include "common.h"
#include "main.exe.h"
#include "padcmd.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActENGAGE(void);
 *     MOTION.C:1181, 56 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern short motID;
 *     extern short motMODE;
 *     extern short dtCMD;
 *     extern short ActionHalt;
 *     extern struct SVECTOR *dtR;
 *     extern struct SVECTOR *dtV;
 *     extern struct Humanoid *StagePlayer;
 *     extern long GameClock;
 *     extern struct VECTOR *dtL;
 *     extern short SelectedItem;
 * END PSX.SYM */

/*
 * ActENGAGE (0x80021270) — updates a humanoid's engage movement and selects
 * the next command, jump, attack, or item-use motion.
 *
 * Matching notes (1,388 bytes / 347 instructions):
 *  - The successful command and item arms repeat the complete
 *    motID/motMODE/return tail.  jump2 merges the stores onto the final
 *    0x602 arm while retaining the target's separate constant-load islands.
 *  - The two-way switch around the camera branch leaves a referenced case
 *    label that prevents an otherwise over-aggressive cross-jump merge.
 *  - Loading dtV before each component value gives the velocity pointer and
 *    component value the target's $v1/$v0 allocation.
 */

extern Humanoid *Me_MOTION_C;

extern void spawn_smoke_burst_(VECTOR *pos, u16 spread, s16 divisor, s16 count);
extern void JumpControl(void);
extern void AttackControl(void);

void ActENGAGE(void)
{
    short motion_id;
    short trig;

    switch (dtM->mid)
    {
    case MOT_ENGAGE_STANCE:
    {
        if (dtPAD & PADLright)
        {
            SET_MOTION(MOT_ENGAGE_TURN_R, 0);
            goto engage_case_post;
        }
        if (dtPAD & PADLleft)
        {
            SET_MOTION(MOT_ENGAGE_TURN_L, 0);
            goto engage_case_post;
        }
        if (dtCMD == CMD_LUNGE_BACK)
        {
            SET_MOTION(MOT_ATTACK_LUNGE_BACK, 1);
            goto engage_case_post;
        }
        if (dtCMD == CMD_FLIP)
        {
            SET_MOTION(MOT_JUMP_FLIP, 0);
            MoveHumanoid(Me_MOTION_C, CHASE_WALK_SPEED, 0);
            goto engage_case_post;
        }
        if (dtM->count != 0)
            goto engage_case_post;
        if (rand() % 20 != 0)
            goto engage_case_post;
        SET_MOTION(MOT_ATTACK_TAUNT, 1);
    engage_case_post:
        if (ActionHalt == -1 && dtM->count == 0)
        {
            motion_id = GetMotionID(dtM, MOT_ENGAGE_SHEATHE);
            if (motion_id < 0)
            {
                SET_MOTION(MOT_STATE_SHEATHE, 1);
            }
            else
            {
                SET_MOTION(MOT_ENGAGE_SHEATHE, 1);
            }
        }
        break;
    }

    case MOT_ENGAGE_TURN_R:
        dtR->vy += Me_MOTION_C->turn;
        if (dtM->count == 1)
            Sound(Me_MOTION_C, SE_TURN_STEP);
        if ((dtPAD & PADLright) == 0)
        {
            SET_MOTION(MOT_ENGAGE_STANCE, 1);
        }
        break;

    case MOT_ENGAGE_TURN_L:
        dtR->vy -= Me_MOTION_C->turn;
        if (dtM->count == 1)
            Sound(Me_MOTION_C, SE_TURN_STEP);
        if ((dtPAD & PADLleft) == 0)
        {
            SET_MOTION(MOT_ENGAGE_STANCE, 1);
        }
        break;

    case MOT_ENGAGE:
    {
        SVECTOR *velocity;
        int value;
        short count;

        velocity = dtV;
        value = velocity->vx;
        if (value != 0)
        {
            if (value > 0)
                value -= 4;
            else
                value += 4;
            velocity->vx = value;
        }
        velocity = dtV;
        value = velocity->vz;
        if (value != 0)
        {
            if (value > 0)
                value -= 4;
            else
                value += 4;
            velocity->vz = value;
        }
        count = --dtM->count;
        if (count < dtM->loop)
        {
            switch (dtPAD & PADLdown)
            {
            default:
                SET_MOTION(MOT_CHASE_BACK, 1);
                break;
            case 0:
                if (Me_MOTION_C == StagePlayer)
                    SetCameraMode(CMODE_NORMAL);
                if (Me_MOTION_C->attribute & ATTR_ALERT)
                {
                    SET_MOTION(MOT_ENGAGE_STANCE, 1);
                }
                else
                {
                    SET_MOTION(0, 1);
                }
                break;
            }
        }
        if ((GameClock & 3) != 0)
            return;
        spawn_smoke_burst_(dtL, 300, 10, 5);
        return;
    }

    case MOT_ENGAGE_SHEATHE:
        if (dtM->count != 0)
            return;
        if (dtM->loop == 0)
            return;
        SET_MOTION(MOT_STATE_SHEATHE, 1);
        return;

    /* A hole in the MOT_ enum, between MOT_ENGAGE_STANCE 0x501 and MOT_ENGAGE_SHEATHE 0x503. It is not in MOTcommon and

     * nothing in main.exe plays it, so it arrives from a character's own

     * mtbl and its meaning is not recoverable here -- hence the digit. */

    case 0x502:
        if (dtM->count != 0)
            return;
        if (dtM->loop == 0)
            return;
        SET_MOTION(MOT_ENGAGE_STANCE, 1);
        return;
    }

    if ((Me_MOTION_C->attribute & ATTR_ALERT) == 0)
    {
        SET_MOTION(0, 1);
        return;
    }
    else if (dtCMD != 0)
    {
        switch (dtCMD)
        {
        case CMD_DASH_FORWARD:
            SET_MOTION(MOT_CHASE_DASH_FWD, 1);
            return;
        case CMD_LUNGE:
            SET_MOTION(MOT_ATTACK_LUNGE, 1);
            return;
        case CMD_DASH_BACKWARD:
            SET_MOTION(MOT_CHASE_DASH_BACK, 1);
            return;
        case CMD_DASH_RIGHT:
            SET_MOTION(MOT_CHASE_DASH_RIGHT, 1);
            return;
        case CMD_DASH_LEFT:
            SET_MOTION(MOT_CHASE_DASH_LEFT, 1);
            return;
        default:
            return;
        }
    }
    else
    {
        trig = Me_MOTION_C->pad.trig;
        if (trig & PADRdown)
        {
            JumpControl();
            return;
        }
        if (trig & PADRup)
        {
            switch (SelectedItem)
            {
            case ITEM_SHURIKEN:
                SET_MOTION(MOT_SYURI, 1);
                return;
            case ITEM_KAGINAWA:
                SET_MOTION(MOT_KAGI, 1);
                return;
            case ITEM_MAKIBISHI:
                SET_MOTION(MOT_ITEM, 1);
                return;
            case ITEM_SMOKE:
                SET_MOTION(MOT_ITEM_THROW, 1);
                return;
            case ITEM_FIRE:
                SET_MOTION(MOT_ITEM_THROW, 1);
                return;
            case ITEM_JIRAI:
                SET_MOTION(MOT_ITEM_PLANT, 1);
                return;
            case ITEM_NONE:
            case ITEM_KAWARIMI:
                SoundEx(Me_MOTION_C->locate, SE_ITEM_UNAVAILABLE);
                return;
            default:
                ReqItemDefault(Me_MOTION_C, SelectedItem);
                return;
            }
        }
        else if (dtPAD & PADRright)
        {
            if (trig & PADRleft)
            {
                SET_MOTION(MOT_ATTACK_CROUCH, 1);
                return;
            }
            SET_MOTION(MOT_SQUAT, 1);
            return;
        }
        else
        {
            if (trig & PADRleft)
            {
                AttackControl();
                return;
            }
            if (dtPAD & PADLup)
            {
                SET_MOTION(MOT_CHASE, 1);
                return;
            }
            if ((dtPAD & PADLdown) == 0)
                return;
            SET_MOTION(MOT_CHASE_BACK, 1);
        }
    }
}
