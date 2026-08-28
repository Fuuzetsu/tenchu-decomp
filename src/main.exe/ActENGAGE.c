#include "common.h"
#include "main.exe.h"
#include "padcmd.h"
#include "item.h"

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
    short one;
    short motion_id;
    short mask;
    int random;

    switch (dtM->mid)
    {
    case 0x501:
    {
        if (dtPAD & PADLright)
        {
            motID = 0x504;
            motMODE = 0;
            goto engage_case_post;
        }
        if (dtPAD & PADLleft)
        {
            motID = 0x505;
            motMODE = 0;
            goto engage_case_post;
        }
        if (dtCMD == CMD_LUNGE_BACK)
        {
            motID = 0x712;
            motMODE = 1;
            goto engage_case_post;
        }
        if (dtCMD == CMD_FLIP)
        {
            motID = 0x907;
            motMODE = 0;
            MoveHumanoid(Me_MOTION_C, 120, 0);
            goto engage_case_post;
        }
        if (dtM->count != 0)
            goto engage_case_post;
        random = rand();
        if (random != (random / 20) * 20)
            goto engage_case_post;
        motID = 0x713;
        motMODE = 1;
    engage_case_post:
        if (ActionHalt == -1 && dtM->count == 0)
        {
            motion_id = GetMotionID(dtM, 0x503);
            if (motion_id < 0)
            {
                motID = 0x80f;
                motMODE = 1;
            }
            else
            {
                motID = 0x503;
                motMODE = 1;
            }
        }
        break;
    }

    case 0x504:
        dtR->vy = dtR->vy + Me_MOTION_C->turn;
        one = 1;
        if (dtM->count == one)
            Sound(Me_MOTION_C, 0x10);
        if ((dtPAD & PADLright) == 0)
        {
            motID = 0x501;
            motMODE = one;
        }
        break;

    case 0x505:
        dtR->vy = dtR->vy - Me_MOTION_C->turn;
        one = 1;
        if (dtM->count == one)
            Sound(Me_MOTION_C, 0x10);
        if ((dtPAD & PADLleft) == 0)
        {
            motID = 0x501;
            motMODE = one;
        }
        break;

    case 0x500:
    {
        SVECTOR *velocity;
        MotionManager *motion;
        register int value;
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
        motion = dtM;
        count = motion->count - 1;
        motion->count = count;
        if (count < motion->loop)
        {
            switch (dtPAD & PADLdown)
            {
            default:
                motID = 0x602;
                motMODE = 1;
                break;
            case 0:
                if (Me_MOTION_C == StagePlayer)
                    SetCameraMode(CMODE_NORMAL);
                if (Me_MOTION_C->attribute & ATTR_ALERT)
                {
                    motID = 0x501;
                    motMODE = 1;
                }
                else
                {
                    motID = 0;
                    motMODE = 1;
                }
                break;
            }
        }
        if ((GameClock & 3) != 0)
            return;
        spawn_smoke_burst_(dtL, 300, 10, 5);
        return;
    }

    case 0x503:
        if (dtM->count != 0)
            return;
        if (dtM->loop == 0)
            return;
        motID = 0x80f;
        motMODE = 1;
        return;

    case 0x502:
        if (dtM->count != 0)
            return;
        if (dtM->loop == 0)
            return;
        motID = 0x501;
        motMODE = 1;
        return;
    }

    if ((Me_MOTION_C->attribute & ATTR_ALERT) == 0)
    {
        motID = 0;
        motMODE = 1;
        return;
    }
    else if (dtCMD != 0)
    {
        switch (dtCMD)
        {
        case 1:
            motID = 0x607;
            motMODE = 1;
            return;
        case 0x21:
            motID = 0x70d;
            motMODE = 1;
            return;
        case 2:
            motID = 0x604;
            motMODE = 1;
            return;
        case 4:
            motID = 0x605;
            motMODE = 1;
            return;
        case 3:
            motID = 0x606;
            motMODE = 1;
            return;
        default:
            return;
        }
    }
    else
    {
        mask = Me_MOTION_C->pad.trig;
        if (mask & PADRdown)
        {
            JumpControl();
            return;
        }
        if (mask & PADRup)
        {
            switch (SelectedItem)
            {
            case 1:
                motID = MOT_SYURI;
                motMODE = 1;
                return;
            case 0:
                motID = MOT_KAGI;
                motMODE = 1;
                return;
            case 2:
                motID = MOT_ITEM;
                motMODE = 1;
                return;
            case 5:
                motID = 0xf02;
                motMODE = 1;
                return;
            case 4:
                motID = 0xf02;
                motMODE = 1;
                return;
            case 6:
                motID = 0xf03;
                motMODE = 1;
                return;
            case -1:
            case 0xa:
                SoundEx(Me_MOTION_C->locate, 0xc);
                return;
            default:
                ReqItemDefault(Me_MOTION_C,
                               (short)SelectedItem);
                return;
            }
        }
        else
        {
            if (dtPAD & PADRright)
            {
                if (mask & PADRleft)
                {
                    motID = 0x70c;
                    motMODE = 1;
                    return;
                }
                motID = MOT_SQUAT;
                motMODE = 1;
                return;
            }
            else
            {
                if (mask & PADRleft)
                {
                    AttackControl();
                    return;
                }
                if (dtPAD & PADLup)
                {
                    motID = MOT_CHASE;
                    motMODE = 1;
                    return;
                }
                if ((dtPAD & PADLdown) == 0)
                    return;
                motID = 0x602;
                motMODE = 1;
            }
        }
    }
}

