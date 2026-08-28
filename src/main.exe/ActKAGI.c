#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActKAGI(void);
 *     MOTION.C:1091, 86 src lines, frame 88 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     stack sp+24     struct VECTOR v
 *     reg   $s0       long dist
 *     stack sp+40     struct PARAM_ITEM_LAUNCH item
 *     reg   $v1       short ry
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern struct VECTOR *dtL;
 *     extern struct TCameraStatus CamState;
 *     extern short motMODE;
 *     extern short motID;
 *     extern struct Humanoid *StagePlayer;
 *     extern struct SVECTOR *dtR;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct SVECTOR *dtV;
 * END PSX.SYM */

/*
 * ActKAGI (0x80020a40, 0x830 bytes) — the grappling-hook action states.
 * 0x400 launches the hook and aims it at the camera target, 0x401 waits for
 * SetFlyWire, and 0x402 pulls the character toward the target before
 * returning to the normal motion system.
 *
 * Matching notes:
 *  - PSX.SYM's VECTOR followed by PARAM_ITEM_LAUNCH is the exact retail
 *    stack layout: v at sp+0x10 and item at sp+0x20, giving a 0x50 frame.
 *  - `model->object[i++]` is material.  The post-increment creates the
 *    target's working copy of the narrow index before its scale and copies
 *    the increment back afterward; spelling the increment as a separate
 *    statement is one instruction short.
 *  - The camera-target block is a local-allocator tie.  `motID = 0x401`
 *    must precede the direct x/z subtraction expressions; preloading either
 *    operand into a temporary rotates v0/v1/a0/a1/a2 even when scheduling
 *    leaves the instruction order unchanged.
 *  - `quantized` must stay full-width through the 0xc00/0x200 rounding.
 *    Narrowing it to u16 creates an extra merge move.  Conversely, the CVA
 *    scan needs its own short counter (`scan_i`) instead of reusing the
 *    earlier model-part counter, which gives the target v1/a1/a0 coloring.
 *  - `__builtin_abs` is intentional: Build.hs passes -fno-builtin to cc1,
 *    so a normal abs() prototype would emit three calls.  The explicit
 *    builtin expands to the target branch/negu chains, and the short-circuit
 *    while duplicates those chains at the loop head and latch exactly.
 *  - The one-shot do around `human = Me_MOTION_C` is a scheduler/allocator
 *    fence: it keeps the human pointer live beside mmp without emitting code,
 *    producing a0/a1 and allowing the attribute load to fill its delay.
 */

extern Humanoid *Me_MOTION_C;

extern s32 spare_item_slot_(s32 mode, Humanoid *human);
extern int ReqItemUse(PARAM_ITEM_LAUNCH *p);

void ActKAGI(void)
{
    VECTOR v;
    long dist;
    PARAM_ITEM_LAUNCH item;
    short ry;
    short i;

    switch (dtM->mid)
    {
    case 0x400:
        if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = -1;
        }

        if (dtM->count == 1)
        {
            item.type = ITEM_KAGINAWA;
            item.user = Me_MOTION_C;
            item.start.vx = dtL->vx;
            item.start.vy = dtL->vy - Me_MOTION_C->height + 300;
            item.start.vz = dtL->vz;
            item.end = item.start;
            ReqItemUse(&item);
            Sound(Me_MOTION_C, 0x1e);
        }
        else if (spare_item_slot_(1, Me_MOTION_C) == 0)
        {
            register VECTOR *target;
            register VECTOR *locate;
            u32 dx;
            u32 dz;

            motID = 0x401;
            locate = dtL;
            target = &CamState.TargetVector;
            dx = target->vx - locate->vx;
            v.vx = dx;
            motMODE = 1;
            dz = target->vz - locate->vz;
            v.vz = dz;
            if (dx == 0 && dz == 0)
            {
                if (Me_MOTION_C == StagePlayer)
                {
                    SetCameraMode(CMODE_NORMAL);
                }
                if (*(u16 *)&Me_MOTION_C->attribute & ATTR_ALERT)
                {
                    motID = 0x501;
                    motMODE = 1;
                }
                else
                {
                    motID = 0;
                    motMODE = 1;
                }
            }
            else
            {
                ry = GetDirection(v.vx, v.vz, dtR->vy);
                dtR->vy += ry;
                Sound(Me_MOTION_C, 0x1f);
            }
        }
        else if (Me_MOTION_C->pad.trig & (PADRleft | PADRdown | PADRright))
        {
            spare_item_slot_(0, Me_MOTION_C);
            if (Me_MOTION_C == StagePlayer)
            {
                SetCameraMode(CMODE_NORMAL);
            }
            if (*(u16 *)&Me_MOTION_C->attribute & ATTR_ALERT)
            {
                motID = 0x501;
                motMODE = 1;
            }
            else
            {
                motID = 0;
                motMODE = 1;
            }
        }

        if ((*(u16 *)&Me_MOTION_C->map.attrib & MAP_WATER) &&
            ((s8)((u16)motID >> 8) != STAT_KAGI))
        {
            ModelArchiveType *model;

            model = Me_MOTION_C->model;
            if (model->n > 12)
            {
                ry = 12;
            }
            else
            {
                ry = model->n - 1;
            }
            i = 7;
            if (i <= ry)
            {
                do
                {
                    *(u16 *)&model->object[i++]->attribute |= MODEL_ATTR_HIDDEN;
                } while (i <= ry);
            }
            *(u16 *)&model->object[0]->attribute |= MODEL_ATTR_HIDDEN;
            motID = MOT_SWIM;
            motMODE = 1;
            dtM->mask = 0x7fff;
        }
        break;

    case 0x401:
    {
        MotionManager *mmp;
        Humanoid *human;
        u16 count;
        u16 attrib;

        if (dtM->count == 0 && dtM->loop == 1)
        {
            VECTOR *p;

            p = GetAbsolutePosition(Me_MOTION_C->model->object[14], 0, 0, 0);
            dtM->loop = -1;
            dtM->count = SetFlyWire(p, &CamState.TargetVector);
        }
        mmp = dtM;
        if (mmp->loop != -1)
        {
            return;
        }
        count = mmp->count - 1;
        mmp->count = count;
        if ((s16)count >= 0)
        {
            return;
        }
        human = Me_MOTION_C;
        motID = 0x402;
        mmp->mask = -2;
        attrib = *(u16 *)&human->map.attrib;
        motMODE = 1;
        if (attrib & MAP_WATER)
        {
            Sound(human, 0x15);
        }
        Sound(Me_MOTION_C, 0x18);
        break;
    }

    case 0x402:
        SetCameraMode(CMODE_AIM);
        v.vx = CamState.TargetVector.vx - dtL->vx;
        v.vy = CamState.TargetVector.vy - dtL->vy;
        v.vz = CamState.TargetVector.vz - dtL->vz;
        dist = SquareRoot0(v.vx * v.vx + v.vz * v.vz);
        ry = GetDirection(v.vx, v.vz, dtR->vy);
        Me_MOTION_C->model->object[0]->rotate.vy = ry;
        Me_MOTION_C->model->object[0]->rotate.vx = ratan2(dist, -v.vy);
        UpdateCoordinate(Me_MOTION_C->model->object[0]);

        {
            long abs_x;

            abs_x = v.vx;
            if (abs_x < 0)
            {
                abs_x = -abs_x;
            }
            if (abs_x < 400)
            {
                long abs_y;

                abs_y = v.vy;
                if (abs_y < 0)
                {
                    abs_y = -abs_y;
                }
                if (abs_y < 400)
                {
                    long abs_z;

                    abs_z = v.vz;
                    if (abs_z < 0)
                    {
                        abs_z = -abs_z;
                    }
                    if (abs_z < 400)
                    {
                        *(u16 *)&Me_MOTION_C->attribute |= ATTR_WALL;
                    }
                }
            }
        }

        {
            Humanoid *human;
            SVECTOR *rotation;
            ModelType *root;
            ModelType *adjust_root;
            short old_ry;
            short scan_i;
            u16 sum;
            u32 quantized;

            human = Me_MOTION_C;
            if ((human->attribute &
                 (ATTR_PUSH | ATTR_HIT | ATTR_NOFLOOR | ATTR_WALL | 0x200)) == 0)
            {
                goto make_wire;
            }
            root = human->model->object[0];
            rotation = dtR;
            old_ry = rotation->vy;
            sum = old_ry + root->rotate.vy;
            quantized = sum & 0xc00;
            rotation->vy = sum;
            if (sum & 0x200)
            {
                quantized += 0x400;
            }
            rotation->vy = quantized;
            adjust_root = human->model->object[0];
            motID = 0x803;
            adjust_root->rotate.vy += old_ry - quantized;
            motMODE = 0;
            dtM->mask = 0x7fff;
            if (MotionUpdateMode != 0)
            {
                for (scan_i = 0; scan_i < 5; scan_i++)
                {
                    if (CVAhuman[scan_i].human == Me_MOTION_C)
                    {
                        goto motion_active;
                    }
                }
            }
            SetNowMotion(Me_MOTION_C, motID, motMODE);
            motMODE = -1;

        motion_active:
            dtM->count >>= 1;
            if (Me_MOTION_C->map.vector != 15)
            {
                dtV->vz = 0;
                dtV->vx = 0;
            }
            else
            {
                dtV->vx >>= 1;
                dtV->vz >>= 1;
            }
            dtV->vy = 0;
            return;
        }

    make_wire:
        while (__builtin_abs(v.vx) > 400 || __builtin_abs(v.vy) > 400 ||
               __builtin_abs(v.vz) > 400)
        {
            v.vx >>= 1;
            v.vy >>= 1;
            v.vz >>= 1;
        }
        dtV->vx = v.vx;
        dtV->vy = v.vy;
        dtV->vz = v.vz;
        SetWire(GetAbsolutePosition(Me_MOTION_C->model->object[14], 0, 0, 0),
                &CamState.TargetVector, 0, 0x1000);
        break;
    }
}
