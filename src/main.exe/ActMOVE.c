#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActMOVE(void);
 *     MOTION.C:994, 32 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern short SelectedItem;
 * END PSX.SYM */

/*
 * ActMOVE (0x80020108) — updates normal movement, turning, ledge checks,
 * jump handling, and selected-item actions.
 *
 * STATUS: MATCHED — exact 860 bytes / 215 instructions.
 */

extern Humanoid *Me_MOTION_C;
extern short HangCheck(void);
extern void JumpControl(void);

void ActMOVE(void)
{
    short mid;

    mid = dtM->mid;
    switch (mid)
    {
    case MOT_MOVE:
        if (dtM->count == 1 ||
            dtM->count == dtM->motion->time / 2)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }
        if ((dtPAD & PADLup) == 0)
        {
            motID = 0;
            motMODE = 1;
            break;
        }
        if (Me_MOTION_C->attribute & ATTR_WALL)
        {
            long y;
            long height;

            y = dtL->vy;
            height = Me_MOTION_C->map.height;
            dtL->vy -= LEDGE_PROBE_RISE;
            Me_MOTION_C->map.height = 1;
            if (HangCheck() == 0)
            {
                dtL->vy = y;
                Me_MOTION_C->map.height = height;
            }
            break;
        }
        if (dtPAD & (PADLleft | PADLright))
        {
            int current;
            int result;
            MotionDataType *motion;
            SVECTOR *rotation;

            rotation = dtR;
            current = rotation->vy;
            if (dtPAD & PADLright)
                result = current + Me_MOTION_C->turn;
            else
                result = current - Me_MOTION_C->turn;
            rotation->vy = result;
            motion = Me_MOTION_C->motion->motion;
            MoveHumanoid(Me_MOTION_C, motion->orderspd, motion->sidespd);
        }
        break;

    case MOT_MOVE_BACK:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }
        if ((dtPAD & PADLdown) == 0)
        {
            motID = 0;
            motMODE = 1;
        }
        {
            int current;
            int result;
            MotionDataType *motion;
            SVECTOR *rotation;

            rotation = dtR;
            current = rotation->vy;
            if (dtPAD & PADLright)
                result = current + Me_MOTION_C->turn;
            else
                result = current - Me_MOTION_C->turn;
            rotation->vy = result;
            motion = Me_MOTION_C->motion->motion;
            MoveHumanoid(Me_MOTION_C, motion->orderspd, motion->sidespd);
        }
        break;

    case MOT_MOVE_DASH_FWD:
    case MOT_MOVE_DASH_BACK:
    case MOT_MOVE_DASH_RIGHT:
    case MOT_MOVE_DASH_LEFT:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x13);
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            motID = 0;
            motMODE = 1;
        }
        break;

    default:
        break;
    }
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
            SoundEx(Me_MOTION_C->locate, SE_ITEM_UNAVAILABLE);
            return;

        item_default:
            ReqItemDefault(Me_MOTION_C, SelectedItem);
            return;
        }
        if (dtPAD & PADRright)
        {
            motID = MOT_SQUAT;
            motMODE = 1;
            return;
        }
        if (trig & PADRleft)
        {
            motID = MOT_STATE_DRAW;
            motMODE = 1;
        }
    }
}
