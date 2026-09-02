#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSWIM(void);
 *     MOTION.C:1030, 57 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $a2       struct ModelArchiveType * mdl
 *     reg   $a1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short motID;
 *     extern short motMODE;
 *     extern short dtPAD;
 *     extern struct SVECTOR *dtR;
 *     extern struct SVECTOR *dtV;
 *     extern struct VECTOR *dtL;
 *     extern struct Humanoid *StagePlayer;
 *     extern short SelectedItem;
 * END PSX.SYM */

/*
 * ActSWIM (0x80020464) — updates swimming movement, state transitions, and
 * selected-item use.
 *
 * The two small helpers inline at every use. `turn_swimmer`'s constant
 * direction folds into the forward/reverse yaw updates, while
 * `ShowHumanoidBodyParts` keeps the model-bound calculation and visibility
 * walk as one source operation without introducing calls in retail.
 *
 * STATUS: MATCHED — exact 1500 bytes / 375 instructions.
 */

extern Humanoid *Me_MOTION_C;
extern short SwimCheck(void);

enum swim_steering_direction
{
    SWIM_STEER_FORWARD = 1,
    SWIM_STEER_REVERSE = -1
};

static inline void turn_swimmer(enum swim_steering_direction direction)
{
    s32 yaw;
    s32 turned_yaw;
    SVECTOR *rotation;

    rotation = dtR;
    yaw = rotation->vy;
    if ((dtPAD & PADLright) != 0)
        turned_yaw = yaw + direction * Me_MOTION_C->turn;
    else
        turned_yaw = yaw - direction * Me_MOTION_C->turn;
    rotation->vy = turned_yaw;
}

void ActSWIM(void)
{
    enum
    {
        SWIM_EXIT_MOVE_FRAME = 40,
        SWIM_EXIT_SPEED = 100
    };
    motion_id current_motion;
    int movement_speed;

    current_motion = dtM->mid;
    switch (current_motion)
    {
    case MOT_SWIM:
        if (SwimCheck() == 0)
        {
            SET_MOTION(MOT_SWIM_EXIT, MOTION_MOVE_APPLY);
            break;
        }
        if ((dtPAD & (PADLleft | PADLright)) != 0)
        {
            if (dtM->count == 1)
                Sound(Me_MOTION_C, SE_WATER_MOVE);
            turn_swimmer(SWIM_STEER_FORWARD);
            break;
        }
        if ((dtPAD & (PADLdown | PADLup)) == 0)
            break;
        SET_MOTION(MOT_SWIM_STROKE, MOTION_MOVE_NONE);
        movement_speed = SWIM_SPEED;
        if (dtPAD & PADLup)
        {
            MoveHumanoid(Me_MOTION_C, movement_speed, 0);
            break;
        }
        {
            Humanoid *backward_swimmer;

            movement_speed = -SWIM_SPEED;
            backward_swimmer = Me_MOTION_C;
            if (backward_swimmer->map.angleH != 0)
                break;
            MoveHumanoid(backward_swimmer, movement_speed, 0);
            break;
        }

    case MOT_SWIM_STROKE:
        if (dtM->count == 1)
            Sound(Me_MOTION_C, SE_WATER_MOVE);
        if (dtPAD & PADLup)
        {
            Humanoid *forward_swimmer;

            if (SwimCheck() == 0)
            {
                SET_MOTION(MOT_SWIM_EXIT, MOTION_MOVE_APPLY);
                break;
            }
            if ((dtPAD & (PADLleft | PADLright)) != 0)
                turn_swimmer(SWIM_STEER_FORWARD);
            movement_speed = SWIM_SPEED;
            forward_swimmer = Me_MOTION_C;
            MoveHumanoid(forward_swimmer, movement_speed, 0);
            break;
        }
        else if (dtPAD & PADLdown)
        {
            if (Me_MOTION_C->map.angleH != 0 || SwimCheck() == 0)
            {
                SVECTOR *blocked_velocity;
                VECTOR *position;

                blocked_velocity = dtV;
                position = dtL;
                position->vx -= blocked_velocity->vx;
                position->vz -= blocked_velocity->vz;
                blocked_velocity->vz = 0;
                blocked_velocity->vx = 0;
                break;
            }
            if ((dtPAD & (PADLleft | PADLright)) != 0)
                turn_swimmer(SWIM_STEER_REVERSE);
            movement_speed = -SWIM_SPEED;
        }
        else
        {
            goto set_swim_idle;
        }

        MoveHumanoid(Me_MOTION_C, movement_speed, 0);
        break;

    set_swim_idle:
        SET_MOTION(MOT_SWIM, MOTION_MOVE_APPLY);
        break;

    case MOT_SWIM_EXIT:
        if (dtM->count == 1)
        {
            ShowHumanoidBodyParts(Me_MOTION_C);
            Sound(Me_MOTION_C, SE_WATER_MOVE);
            return;
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            if (Me_MOTION_C == StagePlayer)
                SetCameraMode(CMODE_NORMAL);
            if (Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN)
            {
                SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
                return;
            }
            SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
            return;
        }
        if (dtM->count <= SWIM_EXIT_MOVE_FRAME)
            return;
        MoveHumanoid(Me_MOTION_C, SWIM_EXIT_SPEED, 0);
        return;

    default:
        break;
    }

    {
        Humanoid *item_user;

        item_user = Me_MOTION_C;
        if ((item_user->pad.trig & PADRup) == 0)
            return;
        /* Every arm of the switch below except ITEM_KAGINAWA is dead past this
         * guard — retail's own code, kept as-is. */
        if (SelectedItem != ITEM_KAGINAWA)
            return;
        dtM->mask = MOTION_MASK_NOROOT;

        ShowHumanoidBodyParts(item_user);

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
}
