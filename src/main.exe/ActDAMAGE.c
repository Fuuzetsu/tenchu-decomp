#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActDAMAGE(void);
 *     MOTION.C:1989, 53 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a2       struct OrnamentType ** weapon
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern struct SVECTOR *dtV;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct VECTOR *dtL;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

extern Humanoid *Me_MOTION_C;

extern void spawn_smoke_burst_(VECTOR *pos, u16 spread, s16 divisor, s16 count);
extern s16 PlayMotion(MotionManager *motion, s16 mode);
extern void SetBlood(VECTOR *pos, s16 n, s16 time);
extern void TurnAroundAllItems(Humanoid *human);

void ActDAMAGE(void)
{
    short done;

    done = false;
    switch (dtM->mid)
    {
    case MOT_DAMAGE_LAUNCH_BACK:
    {
        if (dtM->count == 1)
        {
            ShowHumanoidBodyParts(Me_MOTION_C);
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = MOTION_LOOP_DISABLED;
        }
        else
        {
            dtV->vy = (dtM->count - dtM->motion->time) * 6;
        }
        if ((Me_MOTION_C->attribute & ATTR_NOFLOOR) ||
            Me_MOTION_C->map.height < 0)
        {
            SET_MOTION(MOT_DAMAGE_SLAM_BACK, MOTION_MOVE_NONE);
        }
        if (dtM->count & 4)
            SetBlood(dtL, 1, 60);
        break;
    }

    case MOT_DAMAGE_LAUNCH_FORE:
    {
        if (dtM->count == 1)
        {
            ShowHumanoidBodyParts(Me_MOTION_C);
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = MOTION_LOOP_DISABLED;
        }
        else
        {
            dtV->vy = (dtM->count - dtM->motion->time) * 6;
        }
        if ((Me_MOTION_C->attribute & ATTR_NOFLOOR) ||
            Me_MOTION_C->map.height < 0)
        {
            SET_MOTION(MOT_DAMAGE_SLAM_FORE, MOTION_MOVE_NONE);
        }
        if (dtM->count & 4)
            SetBlood(dtL, 1, 60);
        break;
    }

    case MOT_DAMAGE_SLAM_BACK:
    case MOT_DAMAGE_SLAM_FORE:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_BODY_SLAM);
            spawn_smoke_burst_(dtL, 500, 30, 30);
            if (StagePlayer == Me_MOTION_C)
                PadShockAR(PAD_PORT_1, RUMBLE_POWER_MAX, RUMBLE_ATTACK_NONE, RUMBLE_RELEASE_LONG);
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            SET_MOTION(MOT_DAMAGE_DOWNED, MOTION_MOVE_APPLY);
            break;
        }
        dtV->vx -= (dtV->vx >> 2);
        dtV->vz -= (dtV->vz >> 2);
        break;

    case MOT_DAMAGE_DOWNED:
        if (Me_MOTION_C->life == 0)
        {
            dtM->loop = 0;
            dtM->count = 0;
            PlayMotion(dtM, 1);
            dtM->loop = MOTION_LOOP_FROZEN;
            Me_MOTION_C->status = STAT_DEAD;
            Me_MOTION_C->attribute &= ~ATTR_SEARCH;
            dtV->vx = dtV->vy = dtV->vz = 0;
            if (Me_MOTION_C == StagePlayer)
                return;
            DeleteConflict(Me_MOTION_C->model->object[MODEL_PART_WAIST]);
            TurnAroundAllItems(Me_MOTION_C);
            return;
        }
        dtM->loop--;
        if (Me_MOTION_C->life - Me_MOTION_C->lifemax >= dtM->loop)
        {
            SET_MOTION(MOT_DAMAGE_GETUP, MOTION_MOVE_APPLY);
        }
        break;

    case MOT_DAMAGE_MAKIBISHI:
    case MOT_DAMAGE_CHOKE:
    case MOT_DAMAGE_GETUP:
        if (dtM->count == 0 && dtM->loop != 0)
            done = true;
        break;

    default:
    {
        SVECTOR *velocity;
        int value;

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
        if (dtM->count == 0 && dtM->loop != 0)
        {
            OrnamentType **weapon;
            if (Me_MOTION_C->wpatk != KATANAL)
            {
                done = true;
                break;
            }
            done = true;
            weapon = Me_MOTION_C->weapon;
            if (weapon[WEAPON_SLOT_INACTIVE_1] != NULL)
            {
                weapon[WEAPON_SLOT_INACTIVE_0] = weapon[WEAPON_SLOT_ACTIVE_0];
                weapon[WEAPON_SLOT_ACTIVE_0] = weapon[WEAPON_SLOT_INACTIVE_1];
                weapon[WEAPON_SLOT_INACTIVE_1] = NULL;
                Sound(Me_MOTION_C, CHAR_SE_WEAPON_CHANGE_B);
            }
        }
        break;
    }
    }
    if (done)
    {
        if (Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN)
        {
            motID = MOT_ENGAGE_STANCE;
            motMODE = MOTION_MOVE_APPLY;
            Me_MOTION_C->attribute =
                (Me_MOTION_C->attribute & (u16)~ATTR_PHASE) | PHASE_ALERT;
        }
        else
        {
            SET_MOTION(MOT_STATE_DRAW, MOTION_MOVE_APPLY);
        }
    }
}
