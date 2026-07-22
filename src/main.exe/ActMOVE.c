#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

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
extern s16 dtPAD;
extern s16 motID;
extern s16 motMODE;
extern short HangCheck(void);
extern void JumpControl(void);

void ActMOVE(void)
{
    short mid;

    mid = dtM->mid;
    switch (mid)
    {
    case 0x200:
        if (dtM->count == 1 ||
            dtM->count == dtM->motion->time / 2)
        {
            Sound(Me_MOTION_C, 0x11);
        }
        if ((dtPAD & 0x1000) == 0)
        {
            motID = 0;
            motMODE = 1;
            goto common_action;
        }
        if (Me_MOTION_C->attribute & 0x400)
        {
            long y;
            long height;

            y = dtL->vy;
            height = Me_MOTION_C->map.height;
            dtL->vy = y - 400;
            Me_MOTION_C->map.height = 1;
            if (HangCheck() == 0)
            {
                dtL->vy = y;
                Me_MOTION_C->map.height = height;
            }
            goto common_action;
        }
        if (dtPAD & 0xa000)
        {
            int current;
            int result;
            MotionDataType *motion;
            SVECTOR *rotation;

            rotation = dtR;
            current = rotation->vy;
            if (dtPAD & 0x2000)
                result = current + Me_MOTION_C->turn;
            else
                result = current - Me_MOTION_C->turn;
            rotation->vy = result;
            motion = Me_MOTION_C->motion->motion;
            MoveHumanoid(Me_MOTION_C, motion->orderspd, motion->sidespd);
        }
        goto common_action;

    case 0x201:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x11);
        }
        if ((dtPAD & 0x4000) == 0)
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
            if (dtPAD & 0x2000)
                result = current + Me_MOTION_C->turn;
            else
                result = current - Me_MOTION_C->turn;
            rotation->vy = result;
            motion = Me_MOTION_C->motion->motion;
            MoveHumanoid(Me_MOTION_C, motion->orderspd, motion->sidespd);
        }
        goto common_action;

    case 0x202:
    case 0x203:
    case 0x204:
    case 0x205:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x13);
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            motID = 0;
            motMODE = 1;
        }
        goto common_action;

    default:
        goto common_action;
    }

common_action:
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
        if (trig & 0x80)
        {
            motID = 0x80e;
            motMODE = 1;
        }
    }
}
