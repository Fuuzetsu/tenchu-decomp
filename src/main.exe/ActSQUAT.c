#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"
#include "padcmd.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSQUAT(void);
 *     MOTION.C:1712, 83 src lines, frame 40 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $s0       short turn
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern struct SVECTOR *dtR;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct SVECTOR *dtV;
 *     extern struct TCameraStatus CamState;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct VECTOR *dtL;
 *     extern short dtCMD;
 *     extern short SelectedItem;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

extern Humanoid *Me_MOTION_C;
extern s32 PlayerSSR;

extern void AttackControl(void);
extern MapVector *StickonCheck(void);

void ActSQUAT(void)
{
    short turn;

    turn = Me_MOTION_C->turn / 2;
    switch (dtM->mid)
    {
    case MOT_SQUAT:
        if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = MOTION_LOOP_DISABLED;
        }
        if (dtPAD & PADLup)
        {
            SET_MOTION(MOT_SQUAT_WALK_F, MOTION_MOVE_APPLY);
            break;
        }
        if (dtPAD & PADLdown)
        {
            SET_MOTION(MOT_SQUAT_WALK_B, MOTION_MOVE_APPLY);
            break;
        }
        if (dtPAD & PADLright)
        {
            SET_MOTION(MOT_SQUAT_WALK_R, MOTION_MOVE_APPLY);
            break;
        }
        if (dtPAD & PADLleft)
        {
            SET_MOTION(MOT_SQUAT_WALK_L, MOTION_MOVE_APPLY);
            break;
        }
        if (Me_MOTION_C->pad.trig & PADRdown)
        {
            SET_MOTION(MOT_SQUAT_BACKFLIP, MOTION_MOVE_APPLY);
            dtR->vy += ANGLE_HALF;
        }
        break;

    case MOT_SQUAT_WALK_F:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }
        if ((dtPAD & PADLup) == 0)
        {
            SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
        }
        else if (Me_MOTION_C->pad.trig & PADRdown)
        {
            SET_MOTION(MOT_SQUAT_BACKFLIP, MOTION_MOVE_APPLY);
            dtR->vy += ANGLE_HALF;
        }
        break;

    case MOT_SQUAT_WALK_B:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }
        if ((dtPAD & PADLdown) == 0)
        {
            SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
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
            {
                result = current + turn;
            }
            else
            {
                result = current - turn;
            }
            rotation->vy = result;
            dtV->vz = 0;
            dtV->vx = 0;
            break;
        }
        goto move_if_stationary;

    case MOT_SQUAT_WALK_R:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }
        if ((dtPAD & PADLright) == 0)
        {
            SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
            break;
        }
        if (dtPAD & PADLdown)
        {
            dtR->vy += turn;
            dtV->vz = 0;
            dtV->vx = 0;
            break;
        }
        goto move_if_stationary;

    case MOT_SQUAT_WALK_L:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }
        if ((dtPAD & PADLleft) == 0)
        {
            SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
            break;
        }
        if (dtPAD & PADLdown)
        {
            dtR->vy -= turn;
            dtV->vz = 0;
            dtV->vx = 0;
            break;
        }
    move_if_stationary:
        if (dtV->vx == 0 && dtV->vz == 0)
        {
            MoveHumanoid(Me_MOTION_C,
                         Me_MOTION_C->motion->motion->orderspd,
                         Me_MOTION_C->motion->motion->sidespd);
        }
        break;

    case MOT_SQUAT_BACKFLIP:
        if (dtM->count == (dtM->motion->time >> 1))
        {
            Sound(Me_MOTION_C, SE_ACROBATIC_MOVE);
            CamState.snap_pending = 1;
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
        }
        break;

    default:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_ACROBATIC_MOVE);
            return;
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
            return;
        }
        if (dtV->vx == 0 && dtV->vz == 0)
        {
            return;
        }
        if (GetAreaMapLevel(GlobalAreaMap,
                            dtL->vx + dtV->vx * 4,
                            dtL->vy,
                            dtL->vz + dtV->vz * 4,
                            AREA_LEVEL_STEP_DOWN |
                                AREA_LEVEL_RETURN_DELTA) < STEP_DROP_LIMIT)
        {
            dtL->vx -= dtV->vx;
            dtL->vz -= dtV->vz;
            dtV->vz = 0;
            dtV->vx = 0;
        }
        return;
    }
    if (motID == MOT_SQUAT_BACKFLIP)
    {
        return;
    }
    if (motID != MOT_SQUAT && (dtV->vx != 0 || dtV->vz != 0))
    {
        if (__builtin_abs(GetAreaMapLevel(GlobalAreaMap,
                                          dtL->vx + dtV->vx * 16,
                                          dtL->vy,
                                          dtL->vz + dtV->vz * 16,
                                          AREA_LEVEL_STEP_DOWN |
                                              AREA_LEVEL_RETURN_DELTA)) >= 500)
        {
            dtV->vz = 0;
            dtV->vx = 0;
        }
    }

    if (dtCMD != CMD_NONE)
    {
        switch (dtCMD)
        {
        case CMD_ROLL_FORWARD:
            SET_MOTION(MOT_SQUAT_ROLL_F, MOTION_MOVE_APPLY);
            break;
        case CMD_ROLL_BACKWARD:
            SET_MOTION(MOT_SQUAT_ROLL_B, MOTION_MOVE_APPLY);
            break;
        case CMD_ROLL_LEFT:
            SET_MOTION(MOT_SQUAT_ROLL_L, MOTION_MOVE_APPLY);
            break;
        case CMD_ROLL_RIGHT:
            SET_MOTION(MOT_SQUAT_ROLL_R, MOTION_MOVE_APPLY);
            break;
        }
        return;
    }

    if (Me_MOTION_C->pad.trig & PADRleft)
    {
        AttackControl();
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

    if ((dtPAD & PADRright) == 0)
    {
        if (Me_MOTION_C == StagePlayer)
        {
            SetCameraMode(CMODE_NORMAL);
        }
        if (Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN)
        {
            SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
            return;
        }
        SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
        return;
    }
    if (PlayerSSR != 0)
    {
        StickonCheck();
    }
    return;
}
