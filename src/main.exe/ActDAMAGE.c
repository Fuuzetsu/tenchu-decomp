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

/*
 * ActDAMAGE (0x800262b0) — advances damage-reaction motions, emits impact
 * feedback, handles the fatal transition, and selects the recovery motion.
 *
 * Matching notes (1,548 bytes / 387 instructions):
 *  - The dispatch is a narrowed `(short)(dtM->mid - 0x1005)` jump table;
 *    its source case order is the same as the physical body order.
 *  - Cases 0 and 1 repeat the signed-short model-part loop and the SetBlood
 *    tail.  jump2 merges only the latter onto case 1, leaving the shared
 *    continuation physically between the later case bodies as in retail.
 *  - The deceleration `velocity`/`value` pair is the one working graph
 *    that must stay: spelling the fields directly costs 14 lines and
 *    dropping only `velocity` costs 29. Without `value` the two axes
 *    emit separate signed lh tests and unsigned lhu read-modify-writes,
 *    where the target shares one sign-extended load. The fatal path's
 *    former human/player/velocity aliases, `weapon_kind`, and the final
 *    human/attribute pair were NOT load-bearing and are gone.
 *  - `done` is a short, not enum bool.  Its HImode lifetime produces the
 *    target's v0/s0 join copies and prevents Sound's literal 1 from reusing
 *    s0.  The weapon-kind reject assigns it on both paths; jump/reorg then
 *    places the merged assignment in the comparison branch's delay slot.
 *  - The final attribute value is named before the flag store so its load
 *    overlaps the literal producer, avoiding a load-delay nop and giving the
 *    target's a0/v0/v1 allocation.
 */
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
            model->object[MODEL_PART_WAIST]->attribute &= ~MODEL_ATTR_HIDDEN;
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = -1;
        }
        else
        {
            dtV->vy = (dtM->count - dtM->motion->time) * 6;
        }
        if ((Me_MOTION_C->attribute & ATTR_NOFLOOR) ||
            Me_MOTION_C->map.height < 0)
        {
            SET_MOTION(MOT_DAMAGE_SLAM_BACK, 0);
        }
        if (dtM->count & 4)
            SetBlood(dtL, 1, 60);
        break;
    }

    case MOT_DAMAGE_LAUNCH_FORE:
    {
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
            model->object[MODEL_PART_WAIST]->attribute &= ~MODEL_ATTR_HIDDEN;
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = -1;
        }
        else
        {
            dtV->vy = (dtM->count - dtM->motion->time) * 6;
        }
        if ((Me_MOTION_C->attribute & ATTR_NOFLOOR) ||
            Me_MOTION_C->map.height < 0)
        {
            SET_MOTION(MOT_DAMAGE_SLAM_FORE, 0);
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
                PadShockAR(0, RUMBLE_POWER_MAX, RUMBLE_ATTACK_NONE, RUMBLE_RELEASE_LONG);
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            SET_MOTION(MOT_DAMAGE_DOWNED, 1);
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
            dtM->loop = -2;
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
            SET_MOTION(MOT_DAMAGE_GETUP, 1);
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
        register int value;

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
        if (Me_MOTION_C->attribute & ATTR_ALERT)
        {
            motID = MOT_ENGAGE_STANCE;
            motMODE = 1;
            Me_MOTION_C->attribute =
                (Me_MOTION_C->attribute & (u16)~ATTR_PHASE) | PHASE_ALERT;
        }
        else
        {
            SET_MOTION(MOT_STATE_DRAW, 1);
        }
    }
}
