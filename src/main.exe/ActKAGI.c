#include "common.h"
#include "tuning.h"
#include "sound.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

/* Per-axis limit on the grapple step: the approach is finished once every
 * axis is inside it, and the wire vector is halved until it fits. */
#define KAGI_STEP_MAX 400

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActKAGI(void);
 *     MOTION.C:1091, 86 src lines, frame 88 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
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
 * MOT_KAGI launches the hook and aims it at the camera target, MOT_KAGI_FLY
 * waits for SetFlyWire, and MOT_KAGI_PULL pulls the character toward the target before
 * returning to the normal motion system.
 *
 * Matching notes:
 *  - PSX.SYM's VECTOR followed by PARAM_ITEM_LAUNCH is the exact retail
 *    stack layout: v at sp+0x10 and item at sp+0x20, giving a 0x50 frame.
 *  - `model->object[i++]` is material.  The post-increment creates the
 *    target's working copy of the narrow index before its scale and copies
 *    the increment back afterward; spelling the increment as a separate
 *    statement is one instruction short.
 *  - The camera-target block is a local-allocator tie.  `motID =
 *    MOT_KAGI_FLY` must precede the x/z subtraction expressions, which go
 *    through the register-pinned locate/target pointer pair — reordering
 *    either rotates v0/v1/a0/a1/a2 even when scheduling leaves the
 *    instruction order unchanged.
 *  - `quantized` must stay full-width through the 0xc00/0x200 rounding.
 *    Narrowing it to u16 creates an extra merge move.  Conversely, the CVA
 *    scan needs its own short counter (a nested `i` shadowing the
 *    outer one, which is how PSX.SYM records it) instead of reusing the
 *    earlier model-part counter, which gives the target v1/a1/a0 coloring.
 *  - `__builtin_abs` is intentional: Build.hs passes -fno-builtin to cc1,
 *    so a normal abs() prototype would emit three calls.  The explicit
 *    builtin expands to the target branch/negu chains, and the short-circuit
 *    while duplicates those chains at the loop head and latch exactly.
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
    case MOT_KAGI:
        if (dtM->count == 0 && dtM->loop != 0)
        {
            dtM->loop = MOTION_LOOP_DISABLED;
        }

        if (dtM->count == 1)
        {
            item.type = ITEM_KAGINAWA;
            item.user.human = Me_MOTION_C;
            item.start.vx = dtL->vx;
            item.start.vy = dtL->vy - Me_MOTION_C->height + 300;
            item.start.vz = dtL->vz;
            item.end = item.start;
            ReqItemUse(&item);
            Sound(Me_MOTION_C, SE_THROW_WEAPON);
        }
        else if (spare_item_slot_(1, Me_MOTION_C) == 0)
        {
            register VECTOR *target;
            register VECTOR *locate;
            s32 dx;
            s32 dz;

            motID = MOT_KAGI_FLY;
            locate = dtL;
            target = &CamState.TargetVector;
            dx = target->vx - locate->vx;
            v.vx = dx;
            motMODE = MOTION_MOVE_APPLY;
            dz = target->vz - locate->vz;
            v.vz = dz;
            if (dx == 0 && dz == 0)
            {
                if (Me_MOTION_C == StagePlayer)
                {
                    SetCameraMode(CMODE_NORMAL);
                }
                if (Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN)
                {
                    SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
                }
                else
                {
                    SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
                }
            }
            else
            {
                ry = GetDirection(v.vx, v.vz, dtR->vy);
                dtR->vy += ry;
                Sound(Me_MOTION_C, SE_WEAPON_RECOVER);
            }
        }
        else if (Me_MOTION_C->pad.trig & (PADRleft | PADRdown | PADRright))
        {
            spare_item_slot_(0, Me_MOTION_C);
            if (Me_MOTION_C == StagePlayer)
            {
                SetCameraMode(CMODE_NORMAL);
            }
            if (Me_MOTION_C->attribute & ATTR_WEAPON_DRAWN)
            {
                SET_MOTION(MOT_ENGAGE_STANCE, MOTION_MOVE_APPLY);
            }
            else
            {
                SET_MOTION(MOT_NORMAL, MOTION_MOVE_APPLY);
            }
        }

        if ((Me_MOTION_C->map.attrib & MAP_WATER) &&
            ((motID >> 8) != STAT_KAGI))
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
            while (i <= ry)
            {
                model->object[i++]->attribute |= MODEL_ATTR_HIDDEN;
            }
            *(u16 *)&model->object[MODEL_PART_WAIST]->attribute |= MODEL_ATTR_HIDDEN;
            SET_MOTION(MOT_SWIM, MOTION_MOVE_APPLY);
            dtM->mask = MOTION_MASK_ALL;
        }
        break;

    case MOT_KAGI_FLY:
    {
        MotionManager *mmp;
        Humanoid *human;
        u16 attrib;

        if (dtM->count == 0 && dtM->loop == 1)
        {
            VECTOR *p;

            p = GetAbsolutePosition(
                Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_1], 0, 0, 0);
            dtM->loop = MOTION_LOOP_DISABLED;
            dtM->count = SetFlyWire(p, &CamState.TargetVector);
        }
        mmp = dtM;
        if (mmp->loop != MOTION_LOOP_DISABLED)
        {
            return;
        }
        if (--mmp->count >= 0)
        {
            return;
        }
        human = Me_MOTION_C;
        motID = MOT_KAGI_PULL;
        mmp->mask = MOTION_MASK_NOROOT;
        attrib = human->map.attrib;
        motMODE = MOTION_MOVE_APPLY;
        if (attrib & MAP_WATER)
        {
            Sound(human, SE_WATER_MOVE);
        }
        Sound(Me_MOTION_C, SE_GRAPPLE_PULL);
        break;
    }

    case MOT_KAGI_PULL:
        SetCameraMode(CMODE_AIM);
        v.vx = CamState.TargetVector.vx - dtL->vx;
        v.vy = CamState.TargetVector.vy - dtL->vy;
        v.vz = CamState.TargetVector.vz - dtL->vz;
        dist = SquareRoot0(v.vx * v.vx + v.vz * v.vz);
        ry = GetDirection(v.vx, v.vz, dtR->vy);
        Me_MOTION_C->model->object[MODEL_PART_WAIST]->rotate.vy = ry;
        Me_MOTION_C->model->object[MODEL_PART_WAIST]->rotate.vx = ratan2(dist, -v.vy);
        UpdateCoordinate(Me_MOTION_C->model->object[MODEL_PART_WAIST]);

        if (__builtin_abs(v.vx) < KAGI_STEP_MAX && __builtin_abs(v.vy) < KAGI_STEP_MAX &&
            __builtin_abs(v.vz) < KAGI_STEP_MAX)
        {
            Me_MOTION_C->attribute |= ATTR_WALL;
        }

        {
            Humanoid *human;
            SVECTOR *rotation;
            ModelType *root;
            ModelType *adjust_root;
            short old_ry;
            short i;
            u16 sum;
            u32 quantized;

            human = Me_MOTION_C;
            if ((human->attribute &
                 (ATTR_PUSH | ATTR_HIT | ATTR_NOFLOOR | ATTR_WALL | ATTR_BUOYANT)) == 0)
            {
                goto make_wire;
            }
            root = human->model->object[MODEL_PART_WAIST];
            rotation = dtR;
            old_ry = rotation->vy;
            sum = old_ry + root->rotate.vy;
            quantized = sum & ANGLE_QUADRANT_MASK;
            rotation->vy = sum;
            if (sum & ANGLE_HALF_QUADRANT)
            {
                quantized += ANGLE_QUADRANT;
            }
            rotation->vy = quantized;
            /* The vy = sum store above is dead (quantized overwrites it)
             * but both sh are in the bytes; adjust_root is the asm's own
             * reload of object[0] beside root. */
            adjust_root = human->model->object[MODEL_PART_WAIST];
            motID = MOT_STATE_FALL;
            adjust_root->rotate.vy += old_ry - quantized;
            motMODE = MOTION_MOVE_NONE;
            dtM->mask = MOTION_MASK_ALL;
            if (MotionUpdateMode != 0)
            {
                for (i = 0; i < N_CVA_HUMANS; i++)
                {
                    if (CVAhuman[i].human == Me_MOTION_C)
                    {
                        goto motion_active;
                    }
                }
            }
            SetNowMotion(Me_MOTION_C, motID, motMODE);
            motMODE = MOTION_MOVE_UNSET;

        motion_active:
            dtM->count >>= 1;
            if (Me_MOTION_C->map.vector != MAP_PROBE_ALL)
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
        while (__builtin_abs(v.vx) > KAGI_STEP_MAX || __builtin_abs(v.vy) > KAGI_STEP_MAX ||
               __builtin_abs(v.vz) > KAGI_STEP_MAX)
        {
            v.vx >>= 1;
            v.vy >>= 1;
            v.vz >>= 1;
        }
        setVector(dtV, v.vx, v.vy, v.vz);
        SetWire(GetAbsolutePosition(
                    Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_1], 0, 0, 0),
                &CamState.TargetVector, 0, FIXED_ONE);
        break;
    }
}
