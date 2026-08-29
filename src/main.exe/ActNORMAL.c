#include "common.h"
#include "main.exe.h"
#include "item.h"

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

/*
 * ActNORMAL (0x8001f7e4) — updates idle/turning motion and dispatches command,
 * jump, movement, and selected-item actions.
 *
 * STATUS: MATCHING
 */

extern Humanoid *Me_MOTION_C;
extern int rand(void);
extern void JumpControl(void);

void ActNORMAL(void)
{
    int rotation_value;
    short mid;

    mid = dtM->mid;
    switch (mid)
    {
    case 0:
        if ((dtPAD & PADL2) && StagePlayer != Me_MOTION_C)
        {
            if (dtPAD & PADLdown)
            {
                motID = MOT_ACTION_GESTURE;
                motMODE = 1;
                return;
            }
            if (dtPAD & PADLright)
            {
                motID = MOT_ACTION_LOOP;
                motMODE = 1;
                return;
            }
            if (dtPAD & PADLleft)
            {
                motID = MOT_ACTION_NOTICE;
                motMODE = 1;
                return;
            }
            if (dtPAD & PADLup)
            {
                motID = MOT_ACTION;
                motMODE = 1;
            }
            return;
        }
        if (dtPAD & PADLright)
        {
            motID = 1;
            motMODE = 0;
            break;
        }
        if (dtPAD & PADLleft)
        {
            motID = 2;
            motMODE = 0;
            break;
        }
        if (dtM->count == 0 && rand() % 100 == 0)
        {
            int random;
            short random_motion;

            motMODE = 1;
            random = rand();
            random_motion = 0x105;
            if (random & 1)
                random_motion = 0x104;
            motID = random_motion;
        }
        break;

    case 1:
        if (dtM->count == 1)
            Sound(Me_MOTION_C, 0x10);
        if (dtPAD & PADLright)
        {
            dtR->vy += (u16)Me_MOTION_C->turn;
        }
        else
        {
            motID = 0;
            motMODE = 1;
        }
        break;

    case 2:
        if (dtM->count == 1)
            Sound(Me_MOTION_C, 0x10);
        if ((dtPAD & PADLleft) == 0)
        {
            motID = 0;
            motMODE = 1;
        }
        else
        {
            dtR->vy -= (u16)Me_MOTION_C->turn;
        }
        break;

    default:
        break;
    }
    if (Me_MOTION_C->attribute & ATTR_ALERT)
    {
        motID = MOT_ENGAGE_STANCE;
        motMODE = 1;
        return;
    }

    {
        int command;

        /* A hand goto ladder, provably not a peeled-zero switch: the
         * bodies lay out 1,2,3,0,4 — command 0's (non-case) body sits
         * BETWEEN case bodies, which no switch emission can produce. */
        command = dtCMD;
        if (command == 0)
            goto command_0;
        if (command == 2)
            goto command_2;
        if (command < 3)
        {
            if (command == 1)
                goto command_1;
            return;
        }
        if (command == 3)
            goto command_3;
        if (command == 4)
            goto command_4;
        return;

    command_1:
        motID = MOT_MOVE_DASH_FWD;
        motMODE = 1;
        return;

    command_2:
        motID = MOT_MOVE_DASH_BACK;
        motMODE = 1;
        return;

    command_3:
        motID = MOT_MOVE_DASH_LEFT;
        motMODE = 1;
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
            switch (SelectedItem)
            {
            case ITEM_SHURIKEN:
                motID = MOT_SYURI;
                break;
            case ITEM_KAGINAWA:
                motID = MOT_KAGI;
                break;
            case ITEM_MAKIBISHI:
                motID = MOT_ITEM;
                break;
            case ITEM_SMOKE:
                motID = MOT_ITEM_THROW;
                break;
            case ITEM_FIRE:
                motID = MOT_ITEM_THROW;
                break;
            case ITEM_JIRAI:
                motID = MOT_ITEM_PLANT;
                break;
            case ITEM_NONE:
            case ITEM_KAWARIMI:
                goto item_sound;
            default:
                goto item_default;
            }
            motMODE = 1;
            return;

        item_sound:
            SoundEx(Me_MOTION_C->locate, 0xc);
            return;

        item_default:
            ReqItemDefault(Me_MOTION_C,
                           SelectedItem);
            return;
        }
        if (dtPAD & PADRright)
        {
            motID = MOT_SQUAT;
            motMODE = 1;
            return;
        }
        if (dtPAD & PADLup)
        {
            motID = MOT_MOVE;
            motMODE = 1;
            return;
        }
        if (dtPAD & PADLdown)
        {
            motID = MOT_MOVE_BACK;
            motMODE = 1;
            return;
        }
        if (trig & PADRleft)
        {
            motID = MOT_STATE_DRAW;
            motMODE = 1;
        }
        return;
    }

    command_4:
        motID = MOT_MOVE_DASH_RIGHT;
        motMODE = 1;
        return;
    }
}
