#include "common.h"
#include "tuning.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActJUMP(void);
 *     MOTION.C:1590, 69 src lines, frame 56 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
    short mid; /* the motion id saved before SET_MOTION overwrites motID */
    u16 pad;
    MapVector map;
    SVECTOR spd;
    short ry;
    short i;
    long level;
    long apex_offset;
    long scaled;
    SVECTOR *velocity;

    if ((Me_MOTION_C->pad.trig & PADRdown) != 0 && motID != MOT_JUMP_WALLKICK)
    {
        GetAreaMapVector(GlobalAreaMap, &map, dtL,
                         Me_MOTION_C->width + 300, 0);
        if (map.vector == 0)
        {
            return;
        }
        if (UpdateMotion(dtM, MOT_JUMP_WALLKICK) == 0)
        {
            return;
        }
        ry = RefrectVector[map.vector];
        dtL->vy -= 500;
        if (ry == -1)
        {
            dtV->vx = -dtV->vx;
            dtV->vz = -dtV->vz;
        }
        else
        {
            dtR->vy = ry + ANGLE_HALF;
            MoveHumanoid(Me_MOTION_C, -100, 0);
        }
        Sound(Me_MOTION_C, SE_JUMP_IMPACT);
        return;
    }

    if ((Me_MOTION_C->attribute & (ATTR_NOFLOOR | ATTR_BUOYANT)) != 0 && dtM->count >= 2)
    {
        level = GetAreaMapLevel(GlobalAreaMap, dtL->vx, dtL->vy, dtL->vz, 0);
        if (dtL->vy < level)
        {
            SET_MOTION(MOT_STATE_FALL, 0);
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
            Sound(Me_MOTION_C, CHAR_VOICE_HURT);
            dtM->count >>= 2;
            if (Me_MOTION_C == StagePlayer)
            {
                PadShockAR(0, RUMBLE_POWER_MAX, RUMBLE_ATTACK_NORMAL, RUMBLE_RELEASE_NONE);
            }
            return;
        }
        if (motID == MOT_JUMP_FLIP)
        {
            ModelType *object;

            dtR->vy += (*Me_MOTION_C->model->object)->rotate.vy;
            object = *Me_MOTION_C->model->object;
            SET_MOTION(MOT_STATE_LAND_FLIP, 1);
            object->rotate.vy = 0;
            return;
        }
        SET_MOTION(MOT_STATE_LAND, 0);
        return;
    }
    else
    {
        if (dtM->count == 0 && dtM->loop != 0)
        {
            mid = (u16)motID;
            SET_MOTION(MOT_STATE_FALL, 0);
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
            if (mid != MOT_JUMP_RUN)
            {
                if (mid != MOT_JUMP_FLIP)
                {
                    return;
                }
                dtR->vy += ANGLE_HALF;
                (*Me_MOTION_C->model->object)->rotate.vy = 0;
            }
            dtM->count >>= 1;
            return;
        }

        /* Staged velocity/scaled pair: byte-required (folding the two
         * multiplies into per-arm dtV->vy stores mismatches; measured). */
        velocity = dtV;
        apex_offset = dtM->count - (dtM->motion->time >> 1);
        if (motID == MOT_JUMP_RUN)
        {
            scaled = apex_offset * 10;
        }
        else
        {
            scaled = apex_offset * 20;
        }
        velocity->vy = scaled;

        if ((dtPAD & (PADLleft | PADLdown | PADLright | PADLup)) != 0 && motID != MOT_JUMP_RUN)
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
            spd.vx += dtV->vx;
            spd.vz += dtV->vz;
            if (__builtin_abs(spd.vx) <= 100)
            {
                if (__builtin_abs(spd.vz) <= 100)
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
        if (motID == MOT_JUMP_FLIP)
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
        SET_MOTION(MOT_ATTACK_DIVE, 0);
    }
}
