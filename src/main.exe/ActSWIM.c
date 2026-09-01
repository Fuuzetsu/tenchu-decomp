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
 * STATUS: MATCHED — exact 1500 bytes / 375 instructions.
 */

extern Humanoid *Me_MOTION_C;
extern short SwimCheck(void);

void ActSWIM(void)
{
    enum
    {
        FIRST_SWIM_HIDDEN_PART = 7,
        LAST_SWIM_HIDDEN_PART = 12,
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
            SET_MOTION(MOT_SWIM_EXIT, 1);
            break;
        }
        if ((dtPAD & (PADLleft | PADLright)) != 0)
        {
            if (dtM->count == 1)
                Sound(Me_MOTION_C, SE_WATER_MOVE);
            {
                /* This reads as `dtR->vy += Me_MOTION_C->turn;` and is not:
                 * retail splits the load, add and store across a current yaw,
                 * turned yaw and rotation pointer, and all three are needed.
                 * Measured on the middle of the three copies -- the plain
                 * compound assignment costs 18 lines, dropping only the
                 * pointer 15, dropping only the yaw pair 13; collapsing all
                 * three copies at once costs 226. */
                int idle_yaw;
                int turned_yaw;
                SVECTOR *idle_rotation;

                idle_rotation = dtR;
                idle_yaw = idle_rotation->vy;
                if (dtPAD & PADLright)
                    turned_yaw = idle_yaw + Me_MOTION_C->turn;
                else
                    turned_yaw = idle_yaw - Me_MOTION_C->turn;
                idle_rotation->vy = turned_yaw;
            }
            break;
        }
        if ((dtPAD & (PADLdown | PADLup)) == 0)
            break;
        SET_MOTION(MOT_SWIM_STROKE, 0);
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
                SET_MOTION(MOT_SWIM_EXIT, 1);
                break;
            }
            if ((dtPAD & (PADLleft | PADLright)) != 0)
            {
                int stroke_yaw;
                int turned_yaw;
                SVECTOR *stroke_rotation;

                stroke_rotation = dtR;
                stroke_yaw = stroke_rotation->vy;
                if (dtPAD & PADLright)
                    turned_yaw = stroke_yaw + Me_MOTION_C->turn;
                else
                    turned_yaw = stroke_yaw - Me_MOTION_C->turn;
                stroke_rotation->vy = turned_yaw;
            }
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
            {
                int reverse_yaw;
                int turned_yaw;
                SVECTOR *reverse_rotation;

                reverse_rotation = dtR;
                reverse_yaw = reverse_rotation->vy;
                if (dtPAD & PADLright)
                    turned_yaw = reverse_yaw - Me_MOTION_C->turn;
                else
                    turned_yaw = reverse_yaw + Me_MOTION_C->turn;
                reverse_rotation->vy = turned_yaw;
            }
            movement_speed = -SWIM_SPEED;
        }
        else
        {
            goto set_swim_idle;
        }

        MoveHumanoid(Me_MOTION_C, movement_speed, 0);
        break;

    set_swim_idle:
        SET_MOTION(MOT_SWIM, 1);
        break;

    case MOT_SWIM_EXIT:
        if (dtM->count == 1)
        {
            ModelArchiveType *exit_model;
            s16 last_exit_part;
            s16 exit_part;

            exit_model = Me_MOTION_C->model;
            if (exit_model->n > LAST_SWIM_HIDDEN_PART)
                last_exit_part = LAST_SWIM_HIDDEN_PART;
            else
                last_exit_part = exit_model->n - 1;
            exit_part = FIRST_SWIM_HIDDEN_PART;
            while (exit_part <= last_exit_part)
            {
                u16 *part_attribute;
                int visible_attribute;

                part_attribute =
                    (u16 *)&exit_model->object[exit_part++]->attribute;
                visible_attribute = *part_attribute;
                visible_attribute = visible_attribute & ~MODEL_ATTR_HIDDEN;
                *part_attribute = visible_attribute;
            }
            *(u16 *)&exit_model->object[MODEL_PART_WAIST]->attribute &=
                ~MODEL_ATTR_HIDDEN;
            Sound(Me_MOTION_C, SE_WATER_MOVE);
            return;
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            if (Me_MOTION_C == StagePlayer)
                SetCameraMode(CMODE_NORMAL);
            if (Me_MOTION_C->attribute & ATTR_ALERT)
            {
                SET_MOTION(MOT_ENGAGE_STANCE, 1);
                return;
            }
            SET_MOTION(MOT_NORMAL, 1);
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

    {
        ModelArchiveType *item_model;
        s16 last_item_part;
        s16 item_part;

        item_model = item_user->model;
        if (item_model->n > LAST_SWIM_HIDDEN_PART)
            last_item_part = LAST_SWIM_HIDDEN_PART;
        else
            last_item_part = item_model->n - 1;
        item_part = FIRST_SWIM_HIDDEN_PART;
        while (item_part <= last_item_part)
        {
            u16 *part_attribute;
            int visible_attribute;

            part_attribute =
                (u16 *)&item_model->object[item_part++]->attribute;
            visible_attribute = *part_attribute;
            visible_attribute = visible_attribute & ~MODEL_ATTR_HIDDEN;
            *part_attribute = visible_attribute;
        }
        *(u16 *)&item_model->object[MODEL_PART_WAIST]->attribute &=
            ~MODEL_ATTR_HIDDEN;
    }

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
}
