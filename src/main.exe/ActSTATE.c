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

extern Humanoid *Me_MOTION_C;

extern void spawn_smoke_burst_(VECTOR *pos, u16 spread, s16 divisor, s16 count);
extern int ReqLifeBar(Humanoid *h);

void ActSTATE(void)
{
    short cleanup_guard;
    long t;
    Humanoid *human;

    switch (dtM->mid)
    {
    case MOT_STATE_DRAW:
        if (dtM->count == 1)
        {
            CLEAR_WEAPON_ATTACK_EFFECTS(Me_MOTION_C, Me_MOTION_C->wpatk,
                                        cleanup_guard);
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
                        if ((Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN) != 0)
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
            if ((Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN) == 0)
            {
                return;
            }
            motID = MOT_ENGAGE_STANCE;
            motMODE = MOTION_MOVE_APPLY;
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
        SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
        return;

    case MOT_STATE_SHEATHE: /* stand down: sheathe (hitboxes and afterimage off),
                 * then back to idle unless still combat-ready */
        if (dtM->count == 1)
        {
            CLEAR_WEAPON_ATTACK_EFFECTS(Me_MOTION_C, Me_MOTION_C->wpatk,
                                        cleanup_guard);
            dtM->mask = MOTION_MASK_ALL;
            if ((Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN) != 0)
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
        SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
        return;

    case MOT_STATE_FALL:
        if (dtM->count < -0x36 && dtV->vy > 200)
        {
            dtM->count = -0x1e;
        }
        if (dtV->vy > 0 && (Me_MOTION_C->pad.trig & PADRleft) != 0)
        {
            SET_MOTION(MOT_ATTACK_DIVE, MOTION_MOVE_NONE);
        }
        {
            if ((Me_MOTION_C->attribute & ATTR_NOFLOOR) != 0 || Me_MOTION_C->map.height <= 0)
            {
                if (dtM->count < -0x28)
                {
                    SELECT_RETURN_MOTION();
                    Sound(Me_MOTION_C, SE_LAND_LIGHT);
                    return;
                }
                if (dtM->count > -0x15)
                {
                    if ((Me_MOTION_C->type & PAGE_MASK) != PAGE_GUARD)
                    {
                        SET_MOTION(MOT_STATE_LAND_HEAVY, MOTION_MOVE_NONE);
                        return;
                    }
                }
                else
                {
                    SET_MOTION(MOT_STATE_LAND, MOTION_MOVE_NONE);
                    return;
                }

                {
                    motMODE = MOTION_MOVE_NONE;
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
            PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX, RUMBLE_ATTACK_NORMAL, RUMBLE_RELEASE_NONE);
            SetCameraMode(CMODE_NORMAL);
        }
        if (dtM->count < 5 && (dtPAD & PADRright) != 0 &&
            (Me_MOTION_C->pad.trig & PADRdown) != 0)
        {
            SET_MOTION(MOT_SQUAT_BACKFLIP, MOTION_MOVE_APPLY);
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
                    PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX,
                               RUMBLE_ATTACK_NONE, RUMBLE_RELEASE_LONG);
                }
                else
                {
                    PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX,
                               RUMBLE_ATTACK_FAST, RUMBLE_RELEASE_NONE);
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
                if ((Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN) != 0)
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
        SET_MOTION(MOT_CHASE, MOTION_MOVE_APPLY);
        SET_NOW_MOTION_UNLESS_CVA(goto motion_ready);
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
        if ((Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN) != 0)
        {
            motID = MOT_ENGAGE_STANCE;
            break;
        }
    zero_motion:
        SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
        return;

    default:
    case MOT_STATE_VARIANT_2:
        return;
    }
    motMODE = MOTION_MOVE_APPLY;
    return;
positive_motion:
    motMODE = MOTION_MOVE_APPLY;
}
