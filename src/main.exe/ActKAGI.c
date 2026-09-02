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

extern Humanoid *Me_MOTION_C;

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
            item.user = Me_MOTION_C;
            item.start.vx = dtL->vx;
            item.start.vy = dtL->vy - Me_MOTION_C->height + 300;
            item.start.vz = dtL->vz;
            item.end = item.start;
            ReqItemUse(&item);
            Sound(Me_MOTION_C, SE_THROW_WEAPON);
        }
        else if (spare_item_slot_(SPARE_ITEM_SLOT_QUERY, Me_MOTION_C) == 0)
        {
            VECTOR *target;
            VECTOR *locate;
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
                SELECT_RETURN_MOTION();
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
            spare_item_slot_(SPARE_ITEM_SLOT_CLEAR, Me_MOTION_C);
            SELECT_RETURN_MOTION();
        }

        if ((Me_MOTION_C->map.attrib & MAP_WATER) &&
            (MOTION_STATUS(motID) != STAT_KAGI))
        {
            ModelArchiveType *model;

            model = Me_MOTION_C->model;
            if (model->n > MODEL_PART_BODY_LAST)
            {
                ry = MODEL_PART_BODY_LAST;
            }
            else
            {
                ry = model->n - 1;
            }
            HIDE_HUMANOID_BODY_PARTS(model, ry, i);
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
            /* Retail writes vy before immediately replacing it with the quantized value. */
            adjust_root = human->model->object[MODEL_PART_WAIST];
            motID = MOT_STATE_FALL;
            adjust_root->rotate.vy += old_ry - quantized;
            motMODE = MOTION_MOVE_NONE;
            dtM->mask = MOTION_MASK_ALL;
            SET_NOW_MOTION_UNLESS_CVA(goto motion_active);

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
