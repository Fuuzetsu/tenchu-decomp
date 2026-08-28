#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActJUMP(void);
 *     MOTION.C:1590, 69 src lines, frame 56 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     stack sp+24     struct MapVector map
 *     reg   $a3       short ry
 *     reg   $v1       short i
 *     reg   $s0       short mid
 *     reg   $v1       short i
 *     stack sp+40     struct SVECTOR spd
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct VECTOR *dtL;
 *     extern struct MotionManager *dtM;
 *     extern short RefrectVector[16];
 *     extern struct SVECTOR *dtV;
 *     extern struct SVECTOR *dtR;
 *     extern short MotionUpdateMode;
 *     extern short motMODE;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct Humanoid *StagePlayer;
 *     extern short dtPAD;
 * END PSX.SYM */

extern Humanoid *Me_MOTION_C;

extern short UpdateMotion(MotionManager *mmp, short mid);

/* Jump-state motion, collision response, air steering, and landing control. */
void ActJUMP(void)
{
    short old_mid;
    u16 pad;
    MapVector map;
    SVECTOR spd;
    short reflected;
    short i;
    long level;
    long vertical;
    long scaled;
    SVECTOR *velocity;

    if ((Me_MOTION_C->pad.trig & PADRdown) != 0 && motID != 0x901)
    {
        GetAreaMapVector(GlobalAreaMap, &map, dtL,
                         Me_MOTION_C->width + 300, 0);
        if (map.vector == 0)
        {
            return;
        }
        if (UpdateMotion(dtM, 0x901) == 0)
        {
            return;
        }
        reflected = RefrectVector[map.vector];
        dtL->vy -= 500;
        if (reflected == -1)
        {
            dtV->vx = -dtV->vx;
            dtV->vz = -dtV->vz;
        }
        else
        {
            dtR->vy = reflected + 0x800;
            MoveHumanoid(Me_MOTION_C, -100, 0);
        }
        Sound(Me_MOTION_C, 0x48);
        return;
    }

    if ((Me_MOTION_C->attribute & 0xa00) != 0 && dtM->count >= 2)
    {
        level = GetAreaMapLevel(GlobalAreaMap, dtL->vx, dtL->vy, dtL->vz, 0);
        if (dtL->vy < level)
        {
            motID = 0x803;
            motMODE = 0;
            if (MotionUpdateMode != 0)
            {
                i = 0;
                do
                {
                    if (CVAhuman[i].human == Me_MOTION_C)
                    {
                        goto landed_motion_done;
                    }
                    i++;
                } while (i < 5);
            }
            SetNowMotion(Me_MOTION_C, motID, motMODE);
            motMODE = -1;
        landed_motion_done:
            Sound(Me_MOTION_C, 6);
            dtM->count >>= 2;
            if (Me_MOTION_C == StagePlayer)
            {
                PadShockAR(0, 0xff, 10, 0);
            }
            return;
        }
        if (motID == 0x907)
        {
            ModelType *object;

            dtR->vy += (*Me_MOTION_C->model->object)->rotate.vy;
            object = *Me_MOTION_C->model->object;
            motID = 0x806;
            motMODE = 1;
            object->rotate.vy = 0;
            return;
        }
        motID = 0x804;
        motMODE = 0;
        return;
    }
    else
    {
        if (dtM->count == 0 && dtM->loop != 0)
        {
            old_mid = (u16)motID;
            motID = 0x803;
            motMODE = 0;
            if (MotionUpdateMode != 0)
            {
                i = 0;
                do
                {
                    if (CVAhuman[i].human == Me_MOTION_C)
                    {
                        goto fall_motion_done;
                    }
                    i++;
                } while (i < 5);
            }
            SetNowMotion(Me_MOTION_C, motID, motMODE);
            motMODE = -1;
        fall_motion_done:
            if (old_mid != 0x906)
            {
                if (old_mid != 0x907)
                {
                    return;
                }
                dtR->vy += 0x800;
                (*Me_MOTION_C->model->object)->rotate.vy = 0;
            }
            dtM->count >>= 1;
            return;
        }

        velocity = dtV;
        vertical = dtM->count - (dtM->motion->time >> 1);
        if (motID == 0x906)
        {
            scaled = vertical * 10;
        }
        else
        {
            scaled = vertical * 20;
        }
        velocity->vy = scaled;

        if ((dtPAD & (PADLleft | PADLdown | PADLright | PADLup)) != 0 && motID != 0x906)
        {
            pad = (u16)dtPAD;
            if ((pad & PADLup) != 0)
            {
                GetMoveSpeed(&spd, dtR->vy, 10, 0);
            }
            else if ((pad & PADLdown) != 0)
            {
                GetMoveSpeed(&spd, dtR->vy, -10, 0);
            }
            else if ((pad & PADLright) != 0)
            {
                GetMoveSpeed(&spd, dtR->vy, 0, -10);
            }
            else
            {
                GetMoveSpeed(&spd, dtR->vy, 0, 10);
            }
            spd.vx = spd.vx + dtV->vx;
            spd.vz = spd.vz + dtV->vz;
            if (__builtin_abs(spd.vx) < 101)
            {
                if (__builtin_abs(spd.vz) < 101)
                {
                    dtV->vx = spd.vx;
                    dtV->vz = spd.vz;
                }
            }
        }

        if ((dtPAD & PADRleft) == 0)
        {
            return;
        }
        if (motID == 0x907)
        {
            return;
        }
        if (dtV->vy <= 0)
        {
            return;
        }
        if ((Me_MOTION_C->attribute & ATTR_ALERT) == 0)
        {
            return;
        }
        motID = 0x70f;
        motMODE = 0;
    }
}
