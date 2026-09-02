#include "common.h"
#include "main.exe.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActNORMAL(void);
 *     MOTION.C:916, 43 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern struct Humanoid *StagePlayer;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct SVECTOR *dtR;
 *     extern short dtCMD;
 *     extern short SelectedItem;
 * END PSX.SYM */

extern Humanoid *Me_MOTION_C;
extern int rand(void);
extern void JumpControl(void);

void ActNORMAL(void)
{
    motion_id mid;

    mid = dtM->mid;
    switch (mid)
    {
    case MOT_NORMAL:
        if ((dtPAD & PADL2) && StagePlayer != Me_MOTION_C)
        {
            if (dtPAD & PADLdown)
            {
                SET_MOTION(MOT_ACTION_GESTURE, MOTION_MOVE_APPLY);
                return;
            }
            if (dtPAD & PADLright)
            {
                SET_MOTION(MOT_ACTION_LOOP, MOTION_MOVE_APPLY);
                return;
            }
            if (dtPAD & PADLleft)
            {
                SET_MOTION(MOT_ACTION_NOTICE, MOTION_MOVE_APPLY);
                return;
            }
            if (dtPAD & PADLup)
            {
                SET_MOTION(MOT_ACTION, MOTION_MOVE_APPLY);
            }
            return;
        }
        if (dtPAD & PADLright)
        {
            SET_MOTION(MOT_NORMAL_TURN_R, MOTION_MOVE_NONE);
            break;
        }
        if (dtPAD & PADLleft)
        {
            SET_MOTION(MOT_NORMAL_TURN_L, MOTION_MOVE_NONE);
            break;
        }
        if (dtM->count == 0 && rand() % 100 == 0)
        {
            motMODE = MOTION_MOVE_APPLY;
            motID = (rand() & 1) ? MOT_ACTION_FIDGET_A : MOT_ACTION_FIDGET_B;
        }
        break;

    case MOT_NORMAL_TURN_R:
        if (dtM->count == 1)
            Sound(Me_MOTION_C, SE_TURN_STEP);
        if (dtPAD & PADLright)
        {
            dtR->vy += Me_MOTION_C->turn;
        }
        else
        {
            SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
        }
        break;

    case MOT_NORMAL_TURN_L:
        if (dtM->count == 1)
            Sound(Me_MOTION_C, SE_TURN_STEP);
        if ((dtPAD & PADLleft) == 0)
        {
            SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
        }
        else
        {
            dtR->vy -= Me_MOTION_C->turn;
        }
        break;

    default:
        break;
    }
    if (Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN)
    {
        SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
        return;
    }

    {
        pad_command command;

        command = dtCMD;
        if (command == CMD_NONE)
            goto command_0;
        if (command == CMD_DASH_BACKWARD)
            goto command_2;
        if (command < CMD_DASH_LEFT)
        {
            if (command == CMD_DASH_FORWARD)
                goto command_1;
            return;
        }
        if (command == CMD_DASH_LEFT)
            goto command_3;
        if (command == CMD_DASH_RIGHT)
            goto command_4;
        return;

    command_1:
        SET_MOTION(MOT_MOVE_DASH_FWD, MOTION_MOVE_APPLY);
        return;

    command_2:
        SET_MOTION(MOT_MOVE_DASH_BACK, MOTION_MOVE_APPLY);
        return;

    command_3:
        SET_MOTION(MOT_MOVE_DASH_LEFT, MOTION_MOVE_APPLY);
        return;

    command_0:
    {
        u16 trig;

        trig = Me_MOTION_C->pad.trig;
        if (trig & PADRdown)
        {
            JumpControl();
            return;
        }
        if (trig & PADRup)
        {
            SELECT_ITEM_USE_MOTION(item_sound, item_default);
            motMODE = MOTION_MOVE_APPLY;
            return;

        item_sound:
            SoundEx(Me_MOTION_C->locate, SE_ITEM_UNAVAILABLE);
            return;

        item_default:
            ReqItemDefault(Me_MOTION_C,
                           SelectedItem);
            return;
        }
        if (dtPAD & PADRright)
        {
            SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
            return;
        }
        if (dtPAD & PADLup)
        {
            SET_MOTION(MOT_MOVE, MOTION_MOVE_APPLY);
            return;
        }
        if (dtPAD & PADLdown)
        {
            SET_MOTION(MOT_MOVE_BACK, MOTION_MOVE_APPLY);
            return;
        }
        if (trig & PADRleft)
        {
            SET_MOTION(MOT_STATE_DRAW, MOTION_MOVE_APPLY);
        }
        return;
    }

    command_4:
        SET_MOTION(MOT_MOVE_DASH_RIGHT, MOTION_MOVE_APPLY);
        return;
    }
}
