#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActDAMAGE(void);
 *     MOTION.C:1989, 53 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *  - The fatal path's block-local human/player/velocity aliases preload the
 *    three pointers after PlayMotion without extending one across a call.
 *  - `done` is a short, not enum bool.  Its HImode lifetime produces the
 *    target's v0/s0 join copies and prevents Sound's literal 1 from reusing
 *    s0.  The weapon-kind reject assigns it on both paths; jump/reorg then
 *    places the merged assignment in the comparison branch's delay slot.
 *  - The final attribute value is named before the flag store so its load
 *    overlaps the literal producer, avoiding a load-delay nop and giving the
 *    target's a0/v0/v1 allocation.
 */
extern MotionManager *dtM;
extern SVECTOR *dtV;
extern VECTOR *dtL;
extern Humanoid *Me_MOTION_C;
extern s16 motID;
extern s16 motMODE;

extern s16 Sound(Humanoid *human, s16 id);
extern void FUN_80033bc0(VECTOR *pos, u16 spread, s16 divisor, s16 count);
extern void PadShockAR(s32 port, s32 power, s32 attack, s32 release);
extern s16 PlayMotion(MotionManager *motion, s16 mode);
extern void SetBlood(VECTOR *pos, s16 n, s16 time);
extern void TurnAroundAllItems(Humanoid *human);

void ActDAMAGE(void)
{
    short done;

    done = false;
    switch ((short)(dtM->mid - 0x1005))
    {
    case 0:
    {
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
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = -1;
        }
        else
        {
            dtV->vy = (dtM->count - dtM->motion->time) * 6;
        }
        if ((*(u16 *)&Me_MOTION_C->attribute & 0x800) ||
            Me_MOTION_C->map.height < 0)
        {
            motID = 0x1007;
            motMODE = 0;
        }
        if (dtM->count & 4)
            SetBlood(dtL, 1, 0x3c);
        goto check_done;
    }

    case 1:
    {
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
        }
        else if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = -1;
        }
        else
        {
            dtV->vy = (dtM->count - dtM->motion->time) * 6;
        }
        if ((*(u16 *)&Me_MOTION_C->attribute & 0x800) ||
            Me_MOTION_C->map.height < 0)
        {
            motID = 0x1008;
            motMODE = 0;
        }
        if (dtM->count & 4)
            SetBlood(dtL, 1, 0x3c);
        goto check_done;
    }

    case 2:
    case 3:
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x1d);
            FUN_80033bc0(dtL, 500, 0x1e, 0x1e);
            if (StagePlayer == Me_MOTION_C)
                PadShockAR(0, 0xff, 0, 0x1e);
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            motID = 0x1009;
            motMODE = 1;
            goto check_done;
        }
        dtV->vx = dtV->vx - (dtV->vx >> 2);
        dtV->vz = dtV->vz - (dtV->vz >> 2);
        goto check_done;

    case 4:
        if (Me_MOTION_C->life == 0)
        {
            Humanoid *human;
            Humanoid *player;
            SVECTOR *velocity;

            dtM->loop = 0;
            dtM->count = 0;
            PlayMotion(dtM, 1);
            human = Me_MOTION_C;
            player = StagePlayer;
            dtM->loop = -2;
            human->status = 0x11;
            velocity = dtV;
            *(u16 *)&human->attribute &= 0xffef;
            velocity->vz = 0;
            velocity->vy = 0;
            velocity->vx = 0;
            if (human == player)
                return;
            DeleteConflict(human->model->object[0]);
            TurnAroundAllItems(Me_MOTION_C);
            return;
        }
        dtM->loop--;
        if (Me_MOTION_C->life - (s16)Me_MOTION_C->lifemax >= dtM->loop)
        {
            motID = 0x100c;
            motMODE = 1;
        }
        goto check_done;

    case 5:
    case 6:
    case 7:
        if (dtM->count == 0 && dtM->loop != 0)
            done = true;
        goto check_done;

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
            short weapon_kind;

            weapon_kind = (s16)Me_MOTION_C->weapon_kind;
            if (weapon_kind != 0x2a)
            {
                done = true;
                goto check_done;
            }
            done = true;
            weapon = Me_MOTION_C->weapon;
            if (weapon[3] != NULL)
            {
                weapon[2] = weapon[0];
                weapon[0] = weapon[3];
                weapon[3] = NULL;
                Sound(Me_MOTION_C, 1);
            }
        }
        goto check_done;
    }
    }

check_done:
    if (done)
    {
        register Humanoid *human;

        human = Me_MOTION_C;
        if (*(u16 *)&human->attribute & 0x40)
        {
            u16 attribute;

            motID = 0x501;
            attribute = *(u16 *)&human->attribute;
            motMODE = 1;
            attribute = (attribute & 0xfffc) | 2;
            *(u16 *)&human->attribute = attribute;
        }
        else
        {
            motID = 0x80e;
            motMODE = 1;
        }
    }
}
