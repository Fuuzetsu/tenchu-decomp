#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSWIM(void);
 *     MOTION.C:1030, 57 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
    case 0x300:
        if (SwimCheck() == 0)
        {
            motID = 0x301;
            motMODE = 1;
            goto common_action;
        }
        if ((dtPAD & 0xa000) != 0)
        {
            if (dtM->count == 1)
                Sound(Me_MOTION_C, 0x15);
            {
                int current;
                int result;
                SVECTOR *rotation;

                rotation = dtR;
                current = rotation->vy;
                if (MOTION_PAD_BITS & 0x2000)
                    result = current + Me_MOTION_C->turn;
                else
                    result = current - Me_MOTION_C->turn;
                rotation->vy = result;
            }
            goto common_action;
        }
        if ((MOTION_PAD_BITS & 0x5000) == 0)
            goto common_action;
        motID = 0x302;
        motMODE = 0;
        speed = 0x3c;
        if (MOTION_PAD_BITS & 0x1000)
        {
            MoveHumanoid(Me_MOTION_C, speed, 0);
            goto common_action;
        }
        {
            Humanoid *human;

            speed = -0x3c;
            human = Me_MOTION_C;
            if (human->map.angleH != 0)
                goto common_action;
            MoveHumanoid(human, speed, 0);
            goto common_action;
        }

    case 0x302:
        if (dtM->count == 1)
            Sound(Me_MOTION_C, 0x15);
        if (MOTION_PAD_BITS & 0x1000)
        {
            Humanoid *human;

            if (SwimCheck() == 0)
            {
                motID = 0x301;
                motMODE = 1;
                goto common_action;
            }
            if ((dtPAD & 0xa000) != 0)
            {
                int current;
                int result;
                SVECTOR *rotation;

                rotation = dtR;
                current = rotation->vy;
                if (MOTION_PAD_BITS & 0x2000)
                    result = current + Me_MOTION_C->turn;
                else
                    result = current - Me_MOTION_C->turn;
                rotation->vy = result;
            }
            speed = 0x3c;
            human = Me_MOTION_C;
            MoveHumanoid(human, speed, 0);
            goto common_action;
        }
        else
        {
            if (MOTION_PAD_BITS & 0x4000)
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
                    goto common_action;
                }
                if ((dtPAD & 0xa000) != 0)
                {
                    int current;
                    int result;
                    SVECTOR *rotation;

                    rotation = dtR;
                    current = rotation->vy;
                    if (MOTION_PAD_BITS & 0x2000)
                        result = current - Me_MOTION_C->turn;
                    else
                        result = current + Me_MOTION_C->turn;
                    rotation->vy = result;
                }
                speed = -0x3c;
            }
            else
            {
                goto set_swim_idle;
            }
        }

        MoveHumanoid(Me_MOTION_C, speed, 0);
        goto common_action;

set_swim_idle:
        motID = 0x300;
        motMODE = 1;
        goto common_action;

    case 0x301:
        if (dtM->count == 1)
        {
            ModelArchiveType *model;
            s16 last;
            s16 i;

            model = Me_MOTION_C->model;
            if (0xc < model->n)
                last = 0xc;
            else
                last = model->n - 1;
            i = 7;
            while (i <= last)
            {
                u16 *attribute;
                int attr;

                attribute = (u16 *)&model->object[i++]->attribute;
                attr = *attribute;
                attr = attr & ~1;
                *attribute = attr;
            }
            *(u16 *)&model->object[0]->attribute &= 0xfffe;
            Sound(Me_MOTION_C, 0x15);
            return;
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            if (Me_MOTION_C == StagePlayer)
                SetCameraMode(CMODE_NORMAL);
            if (*(u16 *)&Me_MOTION_C->attribute & 0x40)
            {
                motID = 0x501;
                motMODE = 1;
                return;
            }
            motID = 0;
            motMODE = 1;
            return;
        }
        if (dtM->count < 0x29)
            return;
        MoveHumanoid(Me_MOTION_C, 100, 0);
        return;

    default:
        goto common_action;
    }

common_action:
    {
        Humanoid *human;

        human = Me_MOTION_C;
        if ((human->pad.trig & 0x10) == 0)
            return;
        if (SelectedItem != 0)
            return;
        dtM->mask = -2;

        {
            ModelArchiveType *model;
            s16 last;
            s16 i;

            model = human->model;
            if (0xc < model->n)
                last = 0xc;
            else
                last = model->n - 1;
            i = 7;
            while (i <= last)
            {
                u16 *attribute;
                int attr;

                attribute = (u16 *)&model->object[i++]->attribute;
                attr = *attribute;
                attr = attr & ~1;
                *attribute = attr;
            }
            *(u16 *)&model->object[0]->attribute &= 0xfffe;
        }

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
        ReqItemDefault(Me_MOTION_C, SelectedItem);
        return;
    }
}
