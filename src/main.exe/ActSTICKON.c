#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "padcmd.h"
#include "item.h"
#include "sound.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSTICKON(void);
 *     MOTION.C:1799, 101 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     reg   $v1       struct MapVector * map
 *     reg   $s0       struct ModelArchiveType * model
 *     reg   $a2       short y
 *     reg   $s2       short rv
 *     reg   $s1       short pd
 *     stack sp+16     struct SVECTOR vect
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern struct SVECTOR *dtR;
 *     extern short RefrectVector[16];
 *     extern short dtCMD;
 *     extern short motID;
 *     extern short motMODE;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern short dtPAD;
 *     extern struct SVECTOR *dtV;
 *     extern struct TCameraStatus CamState;
 *     extern short SelectedItem;
 *     extern struct VECTOR *dtL;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

extern Humanoid *Me_MOTION_C;
extern s32 StickonItem;

extern MapVector *StickonCheck(void);
extern int ReqItemMakibishi(PARAM_ITEM_DROP *item);
extern int ReqItemFire(PARAM_ITEM_LAUNCH *item);
extern int ReqItemSmoke(PARAM_ITEM_LAUNCH *item);
extern int ReqItemDokudango(PARAM_ITEM_LAUNCH *item);

void ActSTICKON(void)
{
    MapVector *map;
    ModelArchiveType *model;
    short y;
    short rv;
    short pd;
    short t;

    model = Me_MOTION_C->model;
    switch (dtM->mid)
    {
    case MOT_STICKON:
        if (dtM->count < 0)
        {
            MotionElementType *rotation;
            SVECTOR vect;
            u16 reflected_raw;
            s32 reflected;
            s32 wall_y;

            map = StickonCheck();
            if (map == 0)
            {
                SET_MOTION(MOT_SQUAT, MOTION_MOVE_APPLY);
                dtM->mask = MOTION_MASK_ALL;
                return;
            }

            wall_y = dtR->vy & ANGLE_QUADRANT_MASK;
            if (dtR->vy & ANGLE_HALF_QUADRANT)
            {
                wall_y += ANGLE_QUADRANT;
            }
            reflected_raw = RefrectVector[map->vector] - wall_y;
            t = wall_y;
            rv = reflected_raw;
            reflected = (s16)reflected_raw;
            if (reflected == 0)
            {
                dtR->vy += ANGLE_HALF;
            }
            if (__builtin_abs(reflected) > ANGLE_HALF)
            {
                if (reflected > 0)
                {
                    reflected_raw = reflected - ANGLE_FULL;
                }
                else
                {
                    reflected_raw = reflected + ANGLE_FULL;
                }
                rv = reflected_raw;
            }

            dtR->vy += (t - dtR->vy) / -dtM->count;
            dtM->motion->rotate[MODEL_PART_WAIST]->y = rv;
            rotation = dtM->motion->rotate[MODEL_PART_HEAD];
            if (rv & ANGLE_QUADRANT)
            {
                rotation->y = -rv;
            }
            else
            {
                rotation->y = 0;
            }
            GetMoveSpeed(&vect, rv, -300, 0);
            dtM->motion->locate->x = vect.vx;
            dtM->motion->locate->z = vect.vz;
        }
        else if (dtM->loop > 0)
        {
            dtM->loop = MOTION_LOOP_DISABLED;
        }

        if (dtCMD != CMD_NONE)
        {
            switch (dtCMD)
            {
            case CMD_ROLL_FORWARD:
                SET_MOTION(MOT_SQUAT_ROLL_F, MOTION_MOVE_APPLY);
                break;
            case CMD_ROLL_BACKWARD:
                SET_MOTION(MOT_SQUAT_ROLL_B, MOTION_MOVE_APPLY);
                break;
            case CMD_ROLL_LEFT:
                SET_MOTION(MOT_SQUAT_ROLL_L, MOTION_MOVE_APPLY);
                break;
            case CMD_ROLL_RIGHT:
                SET_MOTION(MOT_SQUAT_ROLL_R, MOTION_MOVE_APPLY);
                break;
            }

            if ((s8)MOTION_STATUS(motID) == STAT_SQUAT)
            {
                dtM->mask = MOTION_MASK_ALL;
                SET_NOW_MOTION_UNLESS_CVA(goto stickon_motion_done);
            stickon_motion_done:
                dtM->count = -5;
                break;
            }
        }

        if (dtM->loop != MOTION_LOOP_DISABLED)
        {
            break;
        }

        {
            if ((dtPAD & (PADLleft | PADLdown | PADLright | PADLup)) != 0)
            {
                pd = 0;
                rv = model->object[MODEL_PART_WAIST]->rotate.vy >> 10 & 3;
                if (((dtPAD >> 12) & 1) == 0)
                {
                    do
                    {
                        pd++;
                    } while (((dtPAD >> (pd + 12)) & 1) == 0);
                }
                if (rv != ((pd + 2) & 3))
                {
                    MotionManager *update_motion;

                    update_motion = dtM;
                    y = MOT_STICKON_SLIDE_R;
                    if (rv == ((pd + 1) & 3))
                    {
                        y = MOT_STICKON_SLIDE_L;
                    }
                    UpdateMotion(update_motion, y);
                    Me_MOTION_C->status = STAT_STICKON;
                    dtV->vz = 0;
                    dtV->vx = 0;
                    dtM->mask = MOTION_MASK_NOROOT;
                    model->object[MODEL_PART_WAIST]->rotate.vx = -0x69;
                    UpdateCoordinate(model->object[MODEL_PART_WAIST]);
                }
                break;
            }
        }

        if ((Me_MOTION_C->pad.trig & PADRup) != 0)
        {
            rv = model->object[MODEL_PART_WAIST]->rotate.vy >> 10 & 3;
            pd = 0;
            switch ((u32)CamState.Mode)
            {
            case CMODE_STICK_L:
                if (rv == 2)
                {
                    pd = MOT_STICKON_THROW_L;
                }
                break;
            case CMODE_PEEP_L:
                if (rv == 3)
                {
                    pd = MOT_STICKON_THROW_L;
                }
                break;
            case CMODE_STICK_R:
                if (rv == 2)
                {
                    pd = MOT_STICKON_THROW_R;
                }
                break;
            case CMODE_PEEP_R:
                if (rv == 1)
                {
                    pd = MOT_STICKON_THROW_R;
                }
                break;
            }

            {
                s32 selected_item;
                s32 high_item;

                selected_item = SelectedItem;
                high_item = selected_item;
                StickonItem = selected_item;
                if (selected_item <= ITEM_SMOKE)
                {
                    if (selected_item < ITEM_FIRE && selected_item != ITEM_MAKIBISHI)
                    {
                        pd = 0;
                    }
                }
                else if (high_item != ITEM_DOKUDANGO)
                {
                    pd = 0;
                }

                if (pd != 0)
                {
                    motMODE = MOTION_MOVE_APPLY;
                    motID = pd;
                    dtM->mask = MOTION_MASK_NOROOT;
                }
                else
                {
                    SoundEx(Me_MOTION_C->locate, SE_ITEM_UNAVAILABLE);
                }
            }
        }
        break;

    case MOT_STICKON_SLIDE_L:
    case MOT_STICKON_SLIDE_R:
    {
        if (dtCMD != CMD_NONE)
        {
            switch (dtCMD)
            {
            case CMD_ROLL_FORWARD:
                SET_MOTION(MOT_SQUAT_ROLL_F, MOTION_MOVE_APPLY);
                break;
            case CMD_ROLL_BACKWARD:
                SET_MOTION(MOT_SQUAT_ROLL_B, MOTION_MOVE_APPLY);
                break;
            case CMD_ROLL_LEFT:
                SET_MOTION(MOT_SQUAT_ROLL_L, MOTION_MOVE_APPLY);
                break;
            case CMD_ROLL_RIGHT:
                SET_MOTION(MOT_SQUAT_ROLL_R, MOTION_MOVE_APPLY);
                break;
            }

            if ((s8)MOTION_STATUS(motID) == STAT_SQUAT)
            {
                dtM->mask = MOTION_MASK_ALL;
                SET_NOW_MOTION_UNLESS_CVA(goto slide_motion_done);
            slide_motion_done:
                dtM->count = -5;
                break;
            }
        }

        if (dtM->count < 0)
        {
            break;
        }

        if ((dtPAD & (PADLleft | PADLdown | PADLright | PADLup)) == 0)
        {
            goto slide_no_pad;
        }

        rv = model->object[MODEL_PART_WAIST]->rotate.vy >> 10 & 3;
        pd = 0;
        if (((dtPAD >> 12) & 1) == 0)
        {
            do
            {
                pd++;
            } while (((dtPAD >> (pd + 12)) & 1) == 0);
        }
        if (rv == ((pd + 2) & 3))
        {
            break;
        }
        t = MOT_STICKON_SLIDE_R;
        if (rv == ((pd + 1) & 3))
        {
            t = MOT_STICKON_SLIDE_L;
        }
        if (motID != t)
        {
            UpdateMotion(dtM, t);
        }

        if (dtPAD & PADLup)
        {
            MoveHumanoid(Me_MOTION_C, 30, 0);
        }
        else if (dtPAD & PADLdown)
        {
            MoveHumanoid(Me_MOTION_C, -30, 0);
        }
        else if (dtPAD & PADLleft)
        {
            MoveHumanoid(Me_MOTION_C, 0, 30);
        }
        else if (dtPAD & PADLright)
        {
            MoveHumanoid(Me_MOTION_C, 0, -30);
        }

        y = model->object[MODEL_PART_WAIST]->rotate.vy + dtR->vy;
        y &= ANGLE_MASK;
        dtL->vx += dtV->vx;
        dtL->vz += dtV->vz;
        map = StickonCheck();
        if (y != RefrectVector[map->vector])
        {
            if (rv == pd)
            {
                dtPAD = 0;
            }
            else
            {
                dtL->vx -= dtV->vx;
                dtL->vz -= dtV->vz;
                UpdateMotion(dtM, MOT_STICKON);
                dtM->loop = MOTION_LOOP_DISABLED;
                dtM->mask = MOTION_MASK_ALL;
            }
        }
        dtV->vz = 0;
        dtV->vx = 0;
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, SE_FOOTSTEP);
        }
        break;

    slide_no_pad:
        SET_MOTION(MOT_STICKON, MOTION_MOVE_APPLY);
        dtM->mask = MOTION_MASK_ALL;
        break;
    }

    case MOT_STICKON_THROW_L:
    case MOT_STICKON_THROW_R:
    {
        VECTOR *position;
        PARAM_ITEM_LAUNCH item;
        s32 angle;

        if (dtM->count != 0 || dtM->loop == 0)
        {
            return;
        }

        pd = motID != MOT_STICKON_THROW_L;
        {
            s32 base_angle_value;

            /* Narrow only after selecting the base angle. */
            base_angle_value =
                (s16)(model->object[MODEL_PART_WAIST]->rotate.vy + dtR->vy);
            angle = (pd ? base_angle_value - ANGLE_QUADRANT
                        : base_angle_value + ANGLE_QUADRANT) &
                    0xF00;
        }
        item.user = Me_MOTION_C;
        item.type = StickonItem;
        Me_MOTION_C->item[StickonItem]--;
        position = GetAbsolutePosition(
            Me_MOTION_C->model->object[MODEL_PART_WEAPON_HAND_0 + pd],
            0, 0, 0);
        angle = (s16)angle;
        position->vx -= (rsin(angle) * 500) >> FIXED_SHIFT;
        position->vz -= (rcos(angle) * 500) >> FIXED_SHIFT;
        item.start.vx = position->vx;
        item.start.vy = position->vy;
        item.start.vz = position->vz;

        if (pd != 0)
        {
            angle -= ANGLE_HALF_QUADRANT;
        }
        else
        {
            angle += ANGLE_HALF_QUADRANT;
        }

        if (item.type == ITEM_MAKIBISHI)
        {
            s32 next_angle;

            for (t = 0; t < 5; t++)
            {
                next_angle = angle - 10;
                next_angle += rand() % 20;
                angle += next_angle - angle;
                /* Empty loop retained for code layout; its original source construct is unknown. */
                do
                {
                } while (0);
                y = next_angle;
                item.end.vx =
                    (rsin(y) * (-30 - rand() % 200)) >> FIXED_SHIFT;
                item.end.vy = rand();
                item.end.vy = -(item.end.vy % 30);
                item.end.vz =
                    (rcos(y) * (-30 - rand() % 200)) >> FIXED_SHIFT;
                ReqItemMakibishi((PARAM_ITEM_DROP *)&item);
            }
        }
        else
        {
            y = angle;
            item.end.vx = (rsin(y) * -120) >> FIXED_SHIFT;
            item.end.vy = 0;
            item.end.vz = (rcos(y) * -120) >> FIXED_SHIFT;
            switch (item.type)
            {
            case ITEM_FIRE:
                ReqItemFire(&item);
                break;
            case ITEM_SMOKE:
                ReqItemSmoke(&item);
                break;
            case ITEM_DOKUDANGO:
                ReqItemDokudango(&item);
                break;
            }
        }
        SET_MOTION(MOT_STICKON, MOTION_MOVE_APPLY);
        dtM->mask = MOTION_MASK_ALL;
        return;
    }

    default:
        break;
    }
    if ((dtPAD & PADRright) == 0)
    {
        dtM->mask = MOTION_MASK_ALL;
        SELECT_RETURN_MOTION();
    }
}
