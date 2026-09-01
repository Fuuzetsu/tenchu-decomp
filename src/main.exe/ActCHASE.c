#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "padcmd.h"
#include "humanoid.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActCHASE(void);
 *     MOTION.C:1241, 61 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s1       short turn
 *     reg   $v1       short i
 *     reg   $s0       long y
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern short MotionUpdateMode;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern short dtCMD;
 *     extern short SelectedItem;
 * END PSX.SYM */

/*
 * ActCHASE (0x800217dc) — updates chase movement and dispatches chase-state
 * jump, attack, and selected-item actions.
 *
 * STATUS: MATCHED — exact 1416 bytes / 354 instructions.
 */

extern Humanoid *Me_MOTION_C;
extern short HangCheck(void);
extern void JumpControl(void);
extern void AttackControl(void);
extern void spawn_smoke_burst_(VECTOR *pos, u16 spread, s16 divisor, s16 count);

void ActCHASE(void)
{
    short turn;

    turn = Me_MOTION_C->turn / 2;
    switch (dtM->mid)
    {
    case MOT_CHASE:
    {
        if (dtM->count == 0 || dtM->count ==
                dtM->motion->time / 2 /* /2 (not >>1): the signed-division
                                         correction is in the bytes here,
                                         unlike ActSQUAT's site; measured */)
        {
            Sound(Me_MOTION_C,
                  (Me_MOTION_C->map.attrib & MAP_WOOD)
                      ? SE_RUN_STEP_WOOD
                      : SE_RUN_STEP);
        }

        if (dtPAD & PADLup)
        {
            if (Me_MOTION_C->attribute & ATTR_LEDGE)
            {
                short i;

                SET_MOTION(MOT_STATE_CLIMB, 0);
                if (MotionUpdateMode != 0)
                {
                    i = 0;
                    do
                    {
                        if (CVAhuman[i].human == Me_MOTION_C)
                        {
                            goto motion_ready;
                        }
                        i++;
                    } while (i < N_CVA_HUMANS);
                }
                SetNowMotion(Me_MOTION_C, motID, motMODE);
                motMODE = MOTION_MOVE_UNSET;
            motion_ready:
                MoveHumanoid(Me_MOTION_C, 35, 0);
                if (dtM->mode & 1)
                {
                    dtM->mode &= ~1;
                    dtM->count = 13;
                }
                else
                {
                    dtM->mode |= 1;
                }
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

                /* Staged read-modify-write (rotation/current/result):
                 * byte-required, same measured lever as ActSQUAT's
                 * identical block. */
                rotation = dtR;
                current = rotation->vy;
                if (dtPAD & PADLright)
                {
                    result = current + turn;
                }
                else
                {
                    result = current - turn;
                }
                rotation->vy = result;
                MoveHumanoid(Me_MOTION_C,
                             Me_MOTION_C->motion->motion->orderspd,
                             Me_MOTION_C->motion->motion->sidespd);
                break;
            }

            if ((dtPAD & PADRright) == 0)
            {
                break;
            }
            motID = MOT_SQUAT;
        }
        else
        {
            motID = MOT_ENGAGE_STANCE;
        }
        motMODE = 1;
        break;
    }

    case MOT_CHASE_BACK:
    {
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }

        if ((dtPAD & PADLdown) == 0)
        {
            SET_MOTION(MOT_ENGAGE_STANCE, 1);
        }
        else if (dtCMD == CMD_LUNGE_BACK)
        {
            SET_MOTION(MOT_ATTACK_LUNGE_BACK, 1);
        }
        else if (dtPAD & (PADLleft | PADLright))
        {
            int current;
            int result;
            SVECTOR *rotation;

            rotation = dtR;
            current = rotation->vy;
            if (dtPAD & PADLright)
            {
                result = current + turn * 4;
            }
            else
            {
                result = current - turn * 4;
            }
            rotation->vy = result;
            MoveHumanoid(Me_MOTION_C,
                         Me_MOTION_C->motion->motion->orderspd,
                         Me_MOTION_C->motion->motion->sidespd);
        }

        if ((dtPAD & PADRright) == 0)
        {
            break;
        }
        if (Me_MOTION_C->pad.trig & PADRleft)
        {
            SET_MOTION(MOT_ATTACK_CROUCH, 1);
            return;
        }
        SET_MOTION(MOT_SQUAT, 1);
        break;
    }

    case MOT_CHASE_DASH_FWD:
        if (Me_MOTION_C->pad.trig & PADRleft)
        {
            AttackControl();
        }
        else if (Me_MOTION_C->pad.trig & PADRdown)
        {
            JumpControl();
        }
        /* fall through */
    case MOT_CHASE_DASH_BACK:
    case MOT_CHASE_DASH_RIGHT:
    case MOT_CHASE_DASH_LEFT:
    {
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_ACROBATIC_MOVE);
        }
        if (dtM->count < 7)
        {
            spawn_smoke_burst_(dtL, 150, SMOKE_DRIFT_DIVISOR_DEFAULT, 1);
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            SET_MOTION(MOT_ENGAGE_STANCE, 1);
        }
        return;
    }

    default:
        break;
    }

    if (dtCMD == CMD_FLIP)
    {
        SET_MOTION(MOT_JUMP_FLIP, 0);
        MoveHumanoid(Me_MOTION_C, CHASE_WALK_SPEED, 0);
        return;
    }

    if (Me_MOTION_C->pad.trig & PADRdown)
    {
        JumpControl();
        return;
    }
    if (Me_MOTION_C->pad.trig & PADRup)
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
    if (Me_MOTION_C->pad.trig & PADRleft)
    {
        AttackControl();
    }
}
