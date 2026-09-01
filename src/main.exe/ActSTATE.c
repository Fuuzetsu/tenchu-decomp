#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "appear.h"
#include "item.h"
#include "afterimage.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSTATE(void);
 *     MOTION.C:1507, 79 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern struct Humanoid *StagePlayer;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct SVECTOR *dtV;
 *     extern short dtPAD;
 *     extern struct SVECTOR *dtR;
 *     extern struct VECTOR *dtL;
 *     extern struct TCameraStatus CamState;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 * END PSX.SYM */

/*
 * ActSTATE (0x8002375c) — handles the humanoid state-motion family rooted at
 * 0x800: weapon draw/sheath cleanup, falls and landing reactions, and the
 * return to the normal standing motion.
 *
 * Matching notes (2,680 bytes / 670 instructions):
 *  - One signed full-width temporary is reused by the chase-Z store and both
 *    terminal motion-selection paths. Its SImode motion producers let jump2
 *    fold only the duplicated final motMODE store.
 *  - The fall graph uses the global humanoid pointer directly; neither its
 *    outer tests nor the random-damage tail need a pointer alias.
 */

extern Humanoid *Me_MOTION_C;

extern void spawn_smoke_burst_(VECTOR *pos, u16 spread, s16 divisor, s16 count);
extern int ReqLifeBar(Humanoid *h);

void ActSTATE(void)
{
    short i, cleanup_guard;
    long t;
    Humanoid *human;

    switch (dtM->mid)
    {
    case MOT_STATE_DRAW:
        if (dtM->count == 1)
        {
            {
                switch (Me_MOTION_C->wpatk)
                {
                case FIST:
                    DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_0]);
                    DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_1]);
                    cleanup_guard = ATTACK_CANCEL_ALL;
                    break;
                case JAW:
                    DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_BEAST_HAND_0]);
                    cleanup_guard = ATTACK_CANCEL_ALL;
                    break;
                case NO_WEAPON:
                    cleanup_guard = ATTACK_CANCEL_ALL;
                    break;
                default:
                    DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0]);
                    DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_1]);
                    cleanup_guard = ATTACK_CANCEL_ALL;
                    break;
                }
                if ((cleanup_guard & ATTACK_CANCEL_AFTERIMAGES) != 0)
                {
                    if (Me_MOTION_C->illusion[WEAPON_HAND_0] != 0)
                    {
                        DisposeAfterimage(
                            Me_MOTION_C->illusion[WEAPON_HAND_0]);
                        Me_MOTION_C->illusion[WEAPON_HAND_0] = 0;
                    }
                    if (Me_MOTION_C->illusion[WEAPON_HAND_1] != 0)
                    {
                        DisposeAfterimage(
                            Me_MOTION_C->illusion[WEAPON_HAND_1]);
                        Me_MOTION_C->illusion[WEAPON_HAND_1] = 0;
                    }
                }
            }
            dtM->mask = MOTION_MASK_ALL;
            if (Me_MOTION_C->type < KERAI_KATANA)
            {
                if (Me_MOTION_C->type > AYAME_1)
                {
                    if (Me_MOTION_C == StagePlayer)
                    {
                        SetCameraMode(CMODE_NORMAL);
                    }
                    {
                        if ((Me_MOTION_C->attribute & ATTR_ALERT) != 0)
                        {
                            t = MOT_ENGAGE_STANCE;
                        }
                        else
                        {
                            goto zero_motion;
                        }
                        motID = t;
                    }
                    break;
                }
            }
            if ((Me_MOTION_C->attribute & ATTR_ALERT) == 0)
            {
                return;
            }
            motID = MOT_ENGAGE_STANCE;
            motMODE = 1;
            return;

        }
        if (dtM->count == dtM->motion->time / 2)
        {
            Sound(Me_MOTION_C, CHAR_SE_WEAPON_CHANGE_A);
            EquipWeapon(Me_MOTION_C, WEAPON_DRAWN);
            return;
        }
        if (dtM->count != 0)
        {
            return;
        }
        if (dtM->loop == 0)
        {
            return;
        }
        {
            human = Me_MOTION_C;
            if ((human->attribute & ATTR_PHASE) == 0)
            {
                human->attribute |= ATTR_SEARCH | PHASE_ALERT;
                human->chase[HUMANOID_CHASE_X] = StagePlayer->locate->vx;
                t = StagePlayer->locate->vz;
                human->actscnt = 1;
                human->chase[HUMANOID_CHASE_Z] = t;
            }
        }
        SET_MOTION(MOT_ENGAGE_STANCE, 1);
        return;

    case MOT_STATE_SHEATHE: /* stand down: sheathe (hitboxes and afterimage off),
                 * then back to idle unless still combat-ready */
        if (dtM->count == 1)
        {
            {
                switch (Me_MOTION_C->wpatk)
                {
                case FIST:
                    DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_0]);
                    DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_ONININ_HAND_1]);
                    cleanup_guard = ATTACK_CANCEL_ALL;
                    break;
                case JAW:
                    DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_BEAST_HAND_0]);
                    cleanup_guard = ATTACK_CANCEL_ALL;
                    break;
                case NO_WEAPON:
                    cleanup_guard = ATTACK_CANCEL_ALL;
                    break;
                default:
                    DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0]);
                    DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_1]);
                    cleanup_guard = ATTACK_CANCEL_ALL;
                    break;
                }
                if ((cleanup_guard & ATTACK_CANCEL_AFTERIMAGES) != 0)
                {
                    if (Me_MOTION_C->illusion[WEAPON_HAND_0] != 0)
                    {
                        DisposeAfterimage(
                            Me_MOTION_C->illusion[WEAPON_HAND_0]);
                        Me_MOTION_C->illusion[WEAPON_HAND_0] = 0;
                    }
                    if (Me_MOTION_C->illusion[WEAPON_HAND_1] != 0)
                    {
                        DisposeAfterimage(
                            Me_MOTION_C->illusion[WEAPON_HAND_1]);
                        Me_MOTION_C->illusion[WEAPON_HAND_1] = 0;
                    }
                }
            }
            dtM->mask = MOTION_MASK_ALL;
            if ((Me_MOTION_C->attribute & ATTR_ALERT) != 0)
            {
                return;
            }
            goto zero_motion;

        }
        if (dtM->count == dtM->motion->time / 2)
        {
            Sound(Me_MOTION_C, CHAR_SE_WEAPON_CHANGE_B);
            EquipWeapon(Me_MOTION_C, WEAPON_SHEATHED);
            return;
        }
        if (dtM->count != 0)
        {
            return;
        }
        if (dtM->loop == 0)
        {
            return;
        }
        SET_MOTION(0, 1);
        return;

    case MOT_STATE_FALL:
        if (dtM->count < -0x36 && dtV->vy > 200)
        {
            dtM->count = -0x1e;
        }
        if (dtV->vy > 0 && (Me_MOTION_C->pad.trig & PADRleft) != 0)
        {
            SET_MOTION(MOT_ATTACK_DIVE, 0);
        }
        {
            if ((Me_MOTION_C->attribute & ATTR_NOFLOOR) != 0 || Me_MOTION_C->map.height <= 0)
            {
                if (dtM->count < -0x28)
                {
                    if (Me_MOTION_C == StagePlayer)
                    {
                        SetCameraMode(CMODE_NORMAL);
                    }
                    if ((Me_MOTION_C->attribute & ATTR_ALERT) != 0)
                    {
                        SET_MOTION(MOT_ENGAGE_STANCE, 1);
                    }
                    else
                    {
                        SET_MOTION(0, 1);
                    }
                    Sound(Me_MOTION_C, SE_LAND_LIGHT);
                    return;
                }
                if (dtM->count > -0x15)
                {
                    if ((Me_MOTION_C->type & PAGE_MASK) != PAGE_GUARD)
                    {
                        SET_MOTION(MOT_STATE_LAND_HEAVY, 0);
                        return;
                    }
                }
                else
                {
                    SET_MOTION(MOT_STATE_LAND, 0);
                    return;
                }

                {
                    motMODE = 0;
                    motID = (rand() & 1) ? MOT_DAMAGE_SLAM_BACK : MOT_DAMAGE_SLAM_FORE;
                    Me_MOTION_C->life -= 10;
                    if (Me_MOTION_C->life < 0)
                    {
                        Me_MOTION_C->life = 0;
                    }
                    Sound(Me_MOTION_C, CHAR_VOICE_HURT_HEAVY);
                    ReqLifeBar(Me_MOTION_C);
                }
                return;
            }
        }

        if (dtM->count > -0x2e)
        {
            return;
        }
        dtV->vx >>= 1;
        dtV->vz >>= 1;
        return;

    case MOT_STATE_LAND:
    case MOT_STATE_LAND_HEAVY:
        if (dtM->count == 1 && Me_MOTION_C == StagePlayer)
        {
            PadShockAR(0, RUMBLE_POWER_MAX, RUMBLE_ATTACK_NORMAL, RUMBLE_RELEASE_NONE);
            SetCameraMode(CMODE_NORMAL);
        }
        if (dtM->count < 5 && (dtPAD & PADRright) != 0 &&
            (Me_MOTION_C->pad.trig & PADRdown) != 0)
        {
            SET_MOTION(MOT_SQUAT_BACKFLIP, 1);
            dtR->vy += ANGLE_HALF;
        }
        /* fall through */
    case MOT_STATE_LAND_FLIP:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C,
                  motID == MOT_STATE_LAND ? SE_LAND_LIGHT : SE_LAND_HEAVY);
            spawn_smoke_burst_(dtL, 300, SMOKE_DRIFT_DIVISOR_DEFAULT, 10);
            if (StagePlayer == Me_MOTION_C)
            {
                if (motID == MOT_STATE_LAND_HEAVY)
                {
                    PadShockAR(0, RUMBLE_POWER_MAX, RUMBLE_ATTACK_NONE, RUMBLE_RELEASE_LONG);
                }
                else
                {
                    PadShockAR(0, RUMBLE_POWER_MAX, RUMBLE_ATTACK_FAST, RUMBLE_RELEASE_NONE);
                }
            }
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            if (motID == MOT_STATE_LAND_FLIP)
            {
                CamState.snap_pending = 1;
            }
            if (Me_MOTION_C == StagePlayer)
            {
                SetCameraMode(CMODE_NORMAL);
            }
            {
                if ((Me_MOTION_C->attribute & ATTR_ALERT) != 0)
                {
                    t = MOT_ENGAGE_STANCE;
                }
                else
                {
                    goto zero_motion;
                }
                motID = t;
            }
            goto positive_motion;
        }
        dtV->vx -= dtV->vx >> 2;
        dtV->vz -= dtV->vz >> 2;
        return;

    case MOT_STATE_CLIMB:
        if (dtM->count != dtM->motion->time / 2)
        {
            if (dtM->count != 0)
            {
                return;
            }
            if (dtM->loop == 0)
            {
                return;
            }
        }
        SET_MOTION(MOT_CHASE, 1);
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
        motMODE = -1;
    motion_ready:
        Sound(Me_MOTION_C, SE_ACROBATIC_MOVE);
        return;

    case MOT_STATE_PICKUP:
        if (dtM->count != 0)
        {
            return;
        }
        if (dtM->loop == 0)
        {
            return;
        }
        if (Me_MOTION_C == StagePlayer)
        {
            SetCameraMode(CMODE_NORMAL);
        }
        if ((Me_MOTION_C->attribute & ATTR_ALERT) != 0)
        {
            motID = MOT_ENGAGE_STANCE;
            break;
        }
    zero_motion:
        SET_MOTION(0, 1);
        return;

    default:
    /* A hole in the MOT_ enum, between MOT_STATE_CLIMB 0x801 and MOT_STATE_FALL 0x803. It is not in MOTcommon and
     * nothing in main.exe plays it, so it arrives from a character's own
     * mtbl and its meaning is not recoverable here -- hence the digit. */
    case 0x802:
        return;
    }
    motMODE = 1;
    return;
positive_motion:
    motMODE = 1;
}
