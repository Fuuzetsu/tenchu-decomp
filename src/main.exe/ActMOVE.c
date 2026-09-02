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

extern Humanoid *Me_MOTION_C;
extern short HangCheck(void);
extern void JumpControl(void);

void ActMOVE(void)
{
    switch (dtM->mid)
    {
    case MOT_MOVE:
        if (dtM->count == 1 ||
            dtM->count == dtM->motion->time / 2)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }
        if ((dtPAD & PADLup) == 0)
        {
            SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
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
            SVECTOR *rotation;

            rotation = dtR;
            current = rotation->vy;
            if (dtPAD & PADLright)
                result = current + Me_MOTION_C->turn;
            else
                result = current - Me_MOTION_C->turn;
            rotation->vy = result;
            MoveHumanoid(Me_MOTION_C,
                         Me_MOTION_C->motion->motion->orderspd,
                         Me_MOTION_C->motion->motion->sidespd);
        }
        break;

    case MOT_MOVE_BACK:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }
        if ((dtPAD & PADLdown) == 0)
        {
            SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
        }
        {
            int current;
            int result;
            SVECTOR *rotation;

            rotation = dtR;
            current = rotation->vy;
            if (dtPAD & PADLright)
                result = current + Me_MOTION_C->turn;
            else
                result = current - Me_MOTION_C->turn;
            rotation->vy = result;
            MoveHumanoid(Me_MOTION_C,
                         Me_MOTION_C->motion->motion->orderspd,
                         Me_MOTION_C->motion->motion->sidespd);
        }
        break;

    case MOT_MOVE_DASH_FWD:
    case MOT_MOVE_DASH_BACK:
    case MOT_MOVE_DASH_RIGHT:
    case MOT_MOVE_DASH_LEFT:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_ACROBATIC_MOVE);
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
        }
        break;

    default:
        break;
    }
    {
        if (Me_MOTION_C->pad.trig & PADRdown)
        {
            JumpControl();
            return;
        }
        if (Me_MOTION_C->pad.trig & PADRup)
        {
            SELECT_ITEM_USE_MOTION(item_sound, item_default);
            motMODE = MOTION_MOVE_APPLY;
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
            SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
            return;
        }
        if (Me_MOTION_C->pad.trig & PADRleft)
        {
            SET_MOTION(MOT_STATE_DRAW, MOTION_MOVE_APPLY);
        }
    }
}
