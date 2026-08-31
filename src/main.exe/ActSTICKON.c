#include "common.h"
#include "main.exe.h"
#include "humanoid.h"
#include "item.h"

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

/*
 * ActSTICKON (0x80025120, 0xcb0 bytes) — controls wall-clinging movement,
 * transitions, camera/item commands, and item throws.
 *
 * STATUS: MATCHED — 3248/3248 bytes, 812/812 instructions.
 *
 * Two notes for future readers, since both residuals here were long recorded
 * as un-reachable register ties and neither actually was:
 *
 * 1. The camera dispatch is a plain switch on an UNSIGNED mode, with the
 *    compare literals inline in each case body. There is deliberately no
 *    `camera_direction` variable: the shared `li v0,K / bne a0,v0` tail in the
 *    target is a jump2 CROSS-JUMPING artifact. Cross-jumping runs AFTER
 *    register allocation, so it merged two case bodies whose only difference
 *    was the `li v0,K`. Modelling that merged tail as a source-level variable
 *    gives the constant a live range spanning the tree's own `li v0,6` compare
 *    scratch, which exiles it from v0 and cascades rv from a0 into a1. Inline
 *    literals are per-case short-lived pseudos that reuse v0, so rv keeps a0.
 *    Case order 6, 9, 7, 10 pairs the two 0xC03 arms and the two 0xC04 arms
 *    and reproduces the target's fall-through block layout.
 *
 * 2. In the makibishi loop, `angle ^= angle ^ next_angle` (the defined
 *    self-XOR update) and the do{}while(0) around it are BOTH load-bearing and
 *    must stay. The XOR keeps the copy from being coalesced away (a plain
 *    `angle = next_angle` loses the `move s2,s0` and the function goes 4 bytes
 *    short). The fence must ENCLOSE the copy: its NOTE_INSN_LOOP_END is what
 *    stops local-alloc's optimize_reg_copy_1 (local-alloc.c:753 in this cc1 —
 *    there is no regmove.c in 2.8.1) from rewriting the later `y = next_angle`
 *    read from next_angle (s0) to angle (s2). That scan breaks on a CODE_LABEL,
 *    a JUMP_INSN, or a loop note; with the copy outside the fence, nothing
 *    breaks it and the sign-extension reads s2.
 */

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
    SVECTOR vect;
    PARAM_ITEM_LAUNCH item;
    short i;
    short t;

    model = Me_MOTION_C->model;
    switch (dtM->mid)
    {
    case MOT_STICKON:
        if (dtM->count < 0)
        {
            MotionElementType *rotation;
            u16 reflected_raw;
            s32 reflected;
            u32 raw_y;
            s32 wall_y;

            map = StickonCheck();
            if (map == 0)
            {
                motID = MOT_SQUAT;
                motMODE = 1;
                dtM->mask = 0x7FFF;
                return;
            }

            raw_y = (u16)dtR->vy;
            wall_y = raw_y & 0xC00;
            if (raw_y & 0x200)
            {
                wall_y += 0x400;
            }
            reflected_raw = (u16)RefrectVector[map->vector] - wall_y;
            /* t re-registers wall_y for the divide below:
             * byte-required (direct wall_y use mismatches; measured). */
            t = wall_y;
            rv = reflected_raw;
            reflected = (s16)reflected_raw;
            if (reflected == 0)
            {
                dtR->vy += 0x800;
            }
            if (__builtin_abs(reflected) > 0x800)
            {
                if (reflected > 0)
                {
                    reflected_raw = reflected - 0x1000;
                }
                else
                {
                    reflected_raw = reflected + 0x1000;
                }
                rv = reflected_raw;
            }

            dtR->vy += (t - dtR->vy) / -dtM->count;
            dtM->motion->rotate[0]->y = rv;
            rotation = dtM->motion->rotate[2];
            if (rv & 0x400)
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
            dtM->loop = -1;
        }

        if (dtCMD != 0)
        {
            switch (dtCMD)
            {
            case 0x11:
                motID = MOT_SQUAT_ROLL_F;
                motMODE = 1;
                break;
            case 0x12:
                motID = MOT_SQUAT_ROLL_B;
                motMODE = 1;
                break;
            case 0x13:
                motID = MOT_SQUAT_ROLL_L;
                motMODE = 1;
                break;
            case 0x14:
                motID = MOT_SQUAT_ROLL_R;
                motMODE = 1;
                break;
            }

            if ((s8)((u16)motID >> 8) == STAT_SQUAT)
            {
                dtM->mask = 0x7FFF;
                if (MotionUpdateMode != 0)
                {
                    for (i = 0; i < 5; i++)
                    {
                        if (CVAhuman[i].human == Me_MOTION_C)
                        {
                            goto stickon_motion_done;
                        }
                    }
                }
                SetNowMotion(Me_MOTION_C, motID, motMODE);
                motMODE = -1;
            stickon_motion_done:
                dtM->count = -5;
                break;
            }
        }

        if (dtM->loop != -1)
        {
            break;
        }

        {
            s32 pad;
            s32 pad_rv;
            s32 loop_pad;
            MotionManager *update_motion;

            pad = (s16)(u16)dtPAD;
            if ((pad & (PADLleft | PADLdown | PADLright | PADLup)) != 0)
            {
                pd = 0;
                rv = (u16)model->object[0]->rotate.vy >> 10 & 3;
                if (((pad >> 12) & 1) == 0)
                {
                    /* loop_pad re-registers pad for the scan:
                     * byte-required (direct pad use mismatches; measured). */
                    loop_pad = pad;
                    do
                    {
                        pd++;
                    } while (((loop_pad >> (pd + 12)) & 1) == 0);
                }
                if (rv != ((pd + 2) & 3))
                {
                    /* Staged dtM load: byte-required (calling with dtM
                     * directly reorders the li/lw pair; measured). */
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
                    dtM->mask = -2;
                    model->object[0]->rotate.vx = -0x69;
                    UpdateCoordinate(model->object[0]);
                }
                break;
            }
        }

        if ((Me_MOTION_C->pad.trig & PADRup) != 0)
        {
            s32 selected_item;
            s32 high_item;
            u32 camera_rv;

            camera_rv = (u16)model->object[0]->rotate.vy >> 10 & 3;
            pd = 0;
            switch ((u32)CamState.Mode)
            {
            case CMODE_STICK_L:
                if (camera_rv == 2)
                {
                    pd = MOT_STICKON_THROW_L;
                }
                break;
            case CMODE_PEEP_L:
                if (camera_rv == 3)
                {
                    pd = MOT_STICKON_THROW_L;
                }
                break;
            case CMODE_STICK_R:
                if (camera_rv == 2)
                {
                    pd = MOT_STICKON_THROW_R;
                }
                break;
            case CMODE_PEEP_R:
                if (camera_rv == 1)
                {
                    pd = MOT_STICKON_THROW_R;
                }
                break;
            }

            selected_item = SelectedItem;
            /* Second name for selected_item: byte-required (removal
             * re-colors the pair; measured). */
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
                motMODE = 1;
                motID = pd;
                dtM->mask = -2;
            }
            else
            {
                SoundEx(Me_MOTION_C->locate, 0xC);
            }
        }
        break;

    case MOT_STICKON_SLIDE_L:
    case MOT_STICKON_SLIDE_R:
    {
        u32 pad_bits;
        s32 pad;
        s32 loop_pad;

        if (dtCMD != 0)
        {
            switch (dtCMD)
            {
            case 0x11:
                motID = MOT_SQUAT_ROLL_F;
                motMODE = 1;
                break;
            case 0x12:
                motID = MOT_SQUAT_ROLL_B;
                motMODE = 1;
                break;
            case 0x13:
                motID = MOT_SQUAT_ROLL_L;
                motMODE = 1;
                break;
            case 0x14:
                motID = MOT_SQUAT_ROLL_R;
                motMODE = 1;
                break;
            }

            if ((s8)((u16)motID >> 8) == STAT_SQUAT)
            {
                dtM->mask = 0x7FFF;
                if (MotionUpdateMode != 0)
                {
                    for (i = 0; i < 5; i++)
                    {
                        if (CVAhuman[i].human == Me_MOTION_C)
                        {
                            goto slide_motion_done;
                        }
                    }
                }
                SetNowMotion(Me_MOTION_C, motID, motMODE);
                motMODE = -1;
            slide_motion_done:
                dtM->count = -5;
                break;
            }
        }

        if (dtM->count < 0)
        {
            break;
        }

        /* dtPAD parked in a register's high half so BOTH extractions below
         * share the one sll: >>16 recovers the whole pad word, >>28 & 1
         * isolates PADLup (bit 12). Masking `pad` directly does not match
         * (andi vs the retail sra pair). */
        pad_bits = (u32)(u16)dtPAD << 16;
        pad = (s32)pad_bits >> 16;
        if ((pad & (PADLleft | PADLdown | PADLright | PADLup)) == 0)
        {
            goto slide_no_pad;
        }

        rv = (u16)model->object[0]->rotate.vy >> 10 & 3;
        pd = 0;
        if ((((s32)pad_bits >> 28) & 1) == 0)
        {
            /* A separate dtPAD reload — the extra lhu is in the bytes. */
            loop_pad = (s16)(u16)dtPAD;
            do
            {
                pd++;
            } while (((loop_pad >> (pd + 12)) & 1) == 0);
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

        y = model->object[0]->rotate.vy + dtR->vy;
        y &= 0xFFF;
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
                dtM->loop = -1;
                dtM->mask = 0x7FFF;
            }
        }
        dtV->vz = 0;
        dtV->vx = 0;
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x11);
        }
        break;

    slide_no_pad:
        motID = MOT_STICKON;
        motMODE = 1;
        dtM->mask = 0x7FFF;
        break;
    }

    case MOT_STICKON_THROW_L:
    case MOT_STICKON_THROW_R:
    {
        VECTOR *position;
        short base_angle;
        s32 base_angle_value;
        s32 angle;
        s32 next_angle;

        if (dtM->count != 0 || dtM->loop == 0)
        {
            return;
        }

        pd = motID != MOT_STICKON_THROW_L;
        base_angle = model->object[0]->rotate.vy + dtR->vy;
        base_angle_value = base_angle;
        angle = (pd ? base_angle_value - 0x400
                    : base_angle_value + 0x400) &
                0xF00;
        item.user = Me_MOTION_C;
        item.type = StickonItem;
        Me_MOTION_C->item[StickonItem]--;
        position = GetAbsolutePosition(Me_MOTION_C->model->object[pd + 0xD] /* 13/14: L/R hand */,
                                       0, 0, 0);
        angle = (s16)angle;
        position->vx -= (rsin(angle) * 500) >> 12;
        position->vz -= (rcos(angle) * 500) >> 12;
        item.start.vx = position->vx;
        item.start.vy = position->vy;
        item.start.vz = position->vz;

        if (pd != 0)
        {
            angle -= 0x200;
        }
        else
        {
            angle += 0x200;
        }

        if (item.type == ITEM_MAKIBISHI)
        {
            for (t = 0; t < 5; t++)
            {
                next_angle = angle - 10;
                next_angle += rand() % 20;
                angle ^= angle ^ next_angle;
                /* empty one-shot: a sched1 region fence (an emptied debug print reads the same way). */
                do
                {
                } while (0);
                y = next_angle;
                item.end.vx = (rsin(y) * (-30 - rand() % 200)) >> 12;
                item.end.vy = rand();
                item.end.vy = -(item.end.vy % 30);
                item.end.vz = (rcos(y) * (-30 - rand() % 200)) >> 12;
                ReqItemMakibishi((PARAM_ITEM_DROP *)&item);
            }
        }
        else
        {
            y = angle;
            item.end.vx = (rsin(y) * -120) >> 12;
            item.end.vy = 0;
            item.end.vz = (rcos(y) * -120) >> 12;
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
        motID = MOT_STICKON;
        motMODE = 1;
        dtM->mask = 0x7FFF;
        return;
    }

    default:
        break;
    }
    if ((dtPAD & PADRright) == 0)
    {
        dtM->mask = 0x7FFF;
        if (Me_MOTION_C == StagePlayer)
        {
            SetCameraMode(CMODE_NORMAL);
        }
        if (*(u16 *)&Me_MOTION_C->attribute & ATTR_ALERT)
        {
            motID = MOT_ENGAGE_STANCE;
            motMODE = 1;
        }
        else
        {
            motID = 0;
            motMODE = 1;
        }
    }
}
