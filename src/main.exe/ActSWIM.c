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
    short mid;
    int speed;

    mid = dtM->mid;
    switch (mid)
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
                int current;
                int result;
                SVECTOR *rotation;

                rotation = dtR;
                current = rotation->vy;
                if (MOTION_PAD_BITS & PADLright)
                    result = current + Me_MOTION_C->turn;
                else
                    result = current - Me_MOTION_C->turn;
                rotation->vy = result;
            }
            break;
        }
        if ((MOTION_PAD_BITS & (PADLdown | PADLup)) == 0)
            break;
        SET_MOTION(MOT_SWIM_STROKE, 0);
        speed = SWIM_SPEED;
        if (MOTION_PAD_BITS & PADLup)
        {
            MoveHumanoid(Me_MOTION_C, speed, 0);
            break;
        }
        {
            Humanoid *human;

            speed = -SWIM_SPEED;
            human = Me_MOTION_C;
            if (human->map.angleH != 0)
                break;
            MoveHumanoid(human, speed, 0);
            break;
        }

    case MOT_SWIM_STROKE:
        if (dtM->count == 1)
            Sound(Me_MOTION_C, SE_WATER_MOVE);
        if (MOTION_PAD_BITS & PADLup)
        {
            Humanoid *human;

            if (SwimCheck() == 0)
            {
                SET_MOTION(MOT_SWIM_EXIT, 1);
                break;
            }
            if ((dtPAD & (PADLleft | PADLright)) != 0)
            {
                int current;
                int result;
                SVECTOR *rotation;

                rotation = dtR;
                current = rotation->vy;
                if (MOTION_PAD_BITS & PADLright)
                    result = current + Me_MOTION_C->turn;
                else
                    result = current - Me_MOTION_C->turn;
                rotation->vy = result;
            }
            speed = SWIM_SPEED;
            human = Me_MOTION_C;
            MoveHumanoid(human, speed, 0);
            break;
        }
        else if (MOTION_PAD_BITS & PADLdown)
        {
            if (Me_MOTION_C->map.angleH != 0 || SwimCheck() == 0)
            {
                SVECTOR *velocity;
                VECTOR *locate;

                velocity = dtV;
                locate = dtL;
                locate->vx -= velocity->vx;
                locate->vz -= velocity->vz;
                velocity->vz = 0;
                velocity->vx = 0;
                break;
            }
            if ((dtPAD & (PADLleft | PADLright)) != 0)
            {
                int current;
                int result;
                SVECTOR *rotation;

                rotation = dtR;
                current = rotation->vy;
                if (MOTION_PAD_BITS & PADLright)
                    result = current - Me_MOTION_C->turn;
                else
                    result = current + Me_MOTION_C->turn;
                rotation->vy = result;
            }
            speed = -SWIM_SPEED;
        }
        else
        {
            goto set_swim_idle;
        }

        MoveHumanoid(Me_MOTION_C, speed, 0);
        break;

    set_swim_idle:
        SET_MOTION(MOT_SWIM, 1);
        break;

    case MOT_SWIM_EXIT:
        if (dtM->count == 1)
        {
            ModelArchiveType *model;
            s16 last;
            s16 i;

            model = Me_MOTION_C->model;
            if (model->n > 12)
                last = 12;
            else
                last = model->n - 1;
            i = 7;
            while (i <= last)
            {
                u16 *attribute;
                int attr;

                attribute = (u16 *)&model->object[i++]->attribute;
                attr = *attribute;
                attr = attr & ~MODEL_ATTR_HIDDEN;
                *attribute = attr;
            }
            *(u16 *)&model->object[MODEL_PART_WAIST]->attribute &= ~MODEL_ATTR_HIDDEN;
            Sound(Me_MOTION_C, SE_WATER_MOVE);
            return;
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            if (Me_MOTION_C == StagePlayer)
                SetCameraMode(CMODE_NORMAL);
            if (*(u16 *)&Me_MOTION_C->attribute & ATTR_ALERT)
            {
                SET_MOTION(MOT_ENGAGE_STANCE, 1);
                return;
            }
            SET_MOTION(0, 1);
            return;
        }
        if (dtM->count <= 40)
            return;
        MoveHumanoid(Me_MOTION_C, 100, 0);
        return;

    default:
        break;
    }

{
    Humanoid *human;

    human = Me_MOTION_C;
    if ((human->pad.trig & PADRup) == 0)
        return;
    /* SelectedItem is 0 past this guard, so every arm of the switch below
     * except ITEM_KAGINAWA (= 0) is dead — retail's own code, kept as-is. */
    if (SelectedItem != 0)
        return;
    dtM->mask = -2;

    {
        ModelArchiveType *model;
        s16 last;
        s16 i;

        model = human->model;
        if (model->n > 12)
            last = 12;
        else
            last = model->n - 1;
        i = 7;
        while (i <= last)
        {
            u16 *attribute;
            int attr;

            attribute = (u16 *)&model->object[i++]->attribute;
            attr = *attribute;
            attr = attr & ~MODEL_ATTR_HIDDEN;
            *attribute = attr;
        }
        *(u16 *)&model->object[MODEL_PART_WAIST]->attribute &= ~MODEL_ATTR_HIDDEN;
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
