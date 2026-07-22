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
        if ((dtPAD & 1) && StagePlayer != Me_MOTION_C)
        {
            if (dtPAD & 0x4000)
            {
                motID = 0x102;
                motMODE = 1;
                return;
            }
            if (dtPAD & 0x2000)
            {
                motID = 0x101;
                motMODE = 1;
                return;
            }
            if (dtPAD & 0x8000)
            {
                motID = 0x106;
                motMODE = 1;
                return;
            }
            if (dtPAD & 0x1000)
            {
                motID = 0x100;
                motMODE = 1;
            }
            return;
        }
        if (dtPAD & 0x2000)
        {
            motID = 1;
            motMODE = 0;
            goto common_action;
        }
        if (dtPAD & 0x8000)
        {
            motID = 2;
            motMODE = 0;
            goto common_action;
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
        goto common_action;

    case 1:
        if (dtM->count == 1)
            Sound(Me_MOTION_C, 0x10);
        if (dtPAD & 0x2000)
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
        rotation_value = (s16)dtPAD & 0x8000U;
        if (rotation_value == 0)
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
        goto common_action;
    }

common_action:
    if (Me_MOTION_C->attribute & 0x40)
    {
        motID = 0x501;
        motMODE = 1;
        return;
    }

    {
        int command;

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
        motID = 0x202;
        motMODE = command;
        return;

command_2:
        motID = 0x203;
        motMODE = 1;
        return;

command_3:
        motID = 0x205;
        motMODE = 1;
        return;

command_0:
    {
        u16 trig;

        trig = Me_MOTION_C->pad.trig;
        if (trig & 0x40)
        {
            JumpControl();
            return;
        }
        if (trig & 0x10)
        {
            switch ((short)(SelectedItem + 1))
            {
            case 2:
                motID = 0xe00;
                break;
            case 1:
                motID = 0x400;
                break;
            case 3:
                motID = 0xf00;
                break;
            case 6:
                motID = 0xf02;
                break;
            case 5:
                motID = 0xf02;
                break;
            case 7:
                motID = 0xf03;
                break;
            case 0:
            case 11:
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
        if (dtPAD & 0x20)
        {
            motID = 0xb00;
            motMODE = 1;
            return;
        }
        if (dtPAD & 0x1000)
        {
            motID = 0x200;
            motMODE = 1;
            return;
        }
        if (dtPAD & 0x4000)
        {
            motID = 0x201;
            motMODE = 1;
            return;
        }
        if (trig & 0x80)
        {
            motID = 0x80e;
            motMODE = 1;
        }
        return;
    }

command_4:
        motID = 0x204;
        motMODE = 1;
        return;
    }
}
