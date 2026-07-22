#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActSTICKON(void);
 *     MOTION.C:1799, 101 src lines, frame 40 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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

extern MotionManager *dtM;
extern Humanoid *Me_MOTION_C;
extern SVECTOR *dtR;
extern SVECTOR *dtV;
extern VECTOR *dtL;
extern s16 dtCMD;
extern s16 motID;
extern s16 motMODE;
extern s16 MotionUpdateMode;
extern s16 dtPAD;
extern s16 RefrectVector[16];
extern TCameraStatus CamState;
extern s16 SelectedItem;
extern s32 D_80097EF0;

extern MapVector *StickonCheck(void);
extern void GetMoveSpeed(SVECTOR *vect, s16 ry, s16 order, s16 side);
extern short SetNowMotion(Humanoid *human, short mid, short move);
extern short Sound(Humanoid *human, s16 seid);
extern int ReqItemMakibishi(PARAM_ITEM_LAUNCH *item);
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
    short drop_index;

    model = Me_MOTION_C->model;
    switch ((short)(dtM->mid - 0xC00))
    {
    case 0:
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
                motID = 0xB00;
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
            drop_index = wall_y;
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

            dtR->vy += (drop_index - dtR->vy) / -dtM->count;
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
            if (dtCMD == 0x12)
            {
                goto case0_command_12;
            }
            if (dtCMD < 0x13)
            {
                if (dtCMD == 0x11)
                {
                    goto case0_command_11;
                }
                goto case0_command_done;
            }
            if (dtCMD == 0x13)
            {
                goto case0_command_13;
            }
            if (dtCMD == 0x14)
            {
                goto case0_command_14;
            }
            goto case0_command_done;

case0_command_12:
            motID = 0xB06;
            goto case0_command_flag;

case0_command_13:
            motID = 0xB08;
            goto case0_command_flag;

case0_command_11:
            motID = 0xB05;
            goto case0_command_flag;

case0_command_14:
            motID = 0xB07;

case0_command_flag:
            motMODE = 1;

case0_command_done:
            if ((s8)((u16)motID >> 8) == 0xB)
            {
                dtM->mask = 0x7FFF;
                if (MotionUpdateMode != 0)
                {
                    for (i = 0; i < 5; i++)
                    {
                        if (CVAhuman[i].human == Me_MOTION_C)
                        {
                            goto case0_motion_done;
                        }
                    }
                }
                SetNowMotion(Me_MOTION_C, motID, motMODE);
                motMODE = -1;
case0_motion_done:
                dtM->count = -5;
                goto common_end;
            }
        }

        if (dtM->loop != -1)
        {
            goto common_end;
        }

        {
            s32 pad;
            s32 pad_rv;
            s32 loop_pad;
            MotionManager *update_motion;

            pad = (s16)(u16)dtPAD;
            if ((pad & 0xF000) != 0)
            {
                pd = 0;
                rv = (u16)model->object[0]->rotate.vy >> 10 & 3;
                if (((pad >> 12) & 1) == 0)
                {
                    loop_pad = pad;
                    do
                    {
                        pd++;
                    } while (((loop_pad >> (pd + 12)) & 1) == 0);
                }
                pad_rv = rv;
                if (pad_rv != ((pd + 2) & 3))
                {
                    update_motion = dtM;
                    y = 0xC02;
                    if (pad_rv == ((pd + 1) & 3))
                    {
                        y = 0xC01;
                    }
                    UpdateMotion(update_motion, y);
                    Me_MOTION_C->status = 0xC;
                    dtV->vz = 0;
                    dtV->vx = 0;
                    dtM->mask = -2;
                    model->object[0]->rotate.vx = -0x69;
                    UpdateCoordinate(model->object[0]);
                }
                goto common_end;
            }
        }

        if ((Me_MOTION_C->pad.trig & 0x10) != 0)
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
                    pd = 0xC03;
                }
                break;
            case CMODE_PEEP_L:
                if (camera_rv == 3)
                {
                    pd = 0xC03;
                }
                break;
            case CMODE_STICK_R:
                if (camera_rv == 2)
                {
                    pd = 0xC04;
                }
                break;
            case CMODE_PEEP_R:
                if (camera_rv == 1)
                {
                    pd = 0xC04;
                }
                break;
            }

            selected_item = SelectedItem;
            high_item = selected_item;
            D_80097EF0 = selected_item;
            if (selected_item < 6)
            {
                if (selected_item < 4 && selected_item != ITEM_MAKIBISHI)
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
        goto common_end;

    case 1:
    case 2:
    {
        u32 pad_bits;
        s32 pad;
        s32 loop_pad;

        if (dtCMD != 0)
        {
            if (dtCMD == 0x12)
            {
                goto case12_command_12;
            }
            if (dtCMD < 0x13)
            {
                if (dtCMD == 0x11)
                {
                    goto case12_command_11;
                }
                goto case12_command_done;
            }
            if (dtCMD == 0x13)
            {
                goto case12_command_13;
            }
            if (dtCMD == 0x14)
            {
                goto case12_command_14;
            }
            goto case12_command_done;

case12_command_12:
            motID = 0xB06;
            goto case12_command_flag;

case12_command_13:
            motID = 0xB08;
            goto case12_command_flag;

case12_command_11:
            motID = 0xB05;
            goto case12_command_flag;

case12_command_14:
            motID = 0xB07;

case12_command_flag:
            motMODE = 1;

case12_command_done:
            if ((s8)((u16)motID >> 8) == 0xB)
            {
                dtM->mask = 0x7FFF;
                if (MotionUpdateMode != 0)
                {
                    for (i = 0; i < 5; i++)
                    {
                        if (CVAhuman[i].human == Me_MOTION_C)
                        {
                            goto case12_motion_done;
                        }
                    }
                }
                SetNowMotion(Me_MOTION_C, motID, motMODE);
                motMODE = -1;
case12_motion_done:
                dtM->count = -5;
                goto common_end;
            }
        }

        if (dtM->count < 0)
        {
            goto common_end;
        }

        pad_bits = (u32)(u16)dtPAD << 16;
        pad = (s32)pad_bits >> 16;
        if ((pad & 0xF000) == 0)
        {
            goto case12_no_pad;
        }

        rv = (u16)model->object[0]->rotate.vy >> 10 & 3;
        pd = 0;
        if ((((s32)pad_bits >> 28) & 1) == 0)
        {
            loop_pad = (s16)(u16)dtPAD;
            do
            {
                pd++;
            } while (((loop_pad >> (pd + 12)) & 1) == 0);
        }
        if (rv == ((pd + 2) & 3))
        {
            goto common_end;
        }
        drop_index = 0xC02;
        if (rv == ((pd + 1) & 3))
        {
            drop_index = 0xC01;
        }
        if (motID != drop_index)
        {
            UpdateMotion(dtM, drop_index);
        }

        if (dtPAD & 0x1000)
        {
            MoveHumanoid(Me_MOTION_C, 0x1E, 0);
        }
        else if (dtPAD & 0x4000)
        {
            MoveHumanoid(Me_MOTION_C, -0x1E, 0);
        }
        else if (dtPAD & 0x8000)
        {
            MoveHumanoid(Me_MOTION_C, 0, 0x1E);
        }
        else if (dtPAD & 0x2000)
        {
            MoveHumanoid(Me_MOTION_C, 0, -0x1E);
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
                UpdateMotion(dtM, 0xC00);
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
        goto common_end;

case12_no_pad:
        motID = 0xC00;
        motMODE = 1;
        dtM->mask = 0x7FFF;
        goto common_end;
    }

    case 3:
    case 4:
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

        pd = motID != 0xC03;
        base_angle = model->object[0]->rotate.vy + dtR->vy;
        base_angle_value = base_angle;
        angle = (pd ? base_angle_value - 0x400
                    : base_angle_value + 0x400) & 0xF00;
        item.user = Me_MOTION_C;
        item.type = D_80097EF0;
        Me_MOTION_C->item[D_80097EF0]--;
        position = GetAbsolutePosition(Me_MOTION_C->model->object[pd + 0xD],
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
            for (drop_index = 0; drop_index < 5; drop_index++)
            {
                next_angle = angle - 10;
                do
                {
                    next_angle += rand() % 20;
                    angle ^= angle ^ next_angle;
                } while (0);
                y = next_angle;
                item.end.vx = (rsin(y) * (-30 - rand() % 200)) >> 12;
                item.end.vy = rand();
                item.end.vy = (item.end.vy / 30) * 30 - item.end.vy;
                item.end.vz = (rcos(y) * (-30 - rand() % 200)) >> 12;
                ReqItemMakibishi(&item);
            }
        }
        else
        {
            y = angle;
            item.end.vx = (rsin(y) * -120) >> 12;
            item.end.vy = 0;
            item.end.vz = (rcos(y) * -120) >> 12;
            if (item.type == ITEM_SMOKE)
            {
                goto item_smoke;
            }
            if ((u32)item.type >= 6)
            {
                goto item_high;
            }
            if (item.type == ITEM_FIRE)
            {
                goto item_fire;
            }
            goto item_done;

item_high:
            if (item.type == ITEM_DOKUDANGO)
            {
                goto item_dokudango;
            }
            goto item_done;

item_fire:
            ReqItemFire(&item);
            goto item_done;

item_smoke:
            ReqItemSmoke(&item);
            goto item_done;

item_dokudango:
            ReqItemDokudango(&item);

item_done:
        }
        motID = 0xC00;
        motMODE = 1;
        dtM->mask = 0x7FFF;
        return;
    }

    default:
        break;
    }

common_end:
    if ((dtPAD & 0x20) == 0)
    {
        dtM->mask = 0x7FFF;
        if (Me_MOTION_C == StagePlayer)
        {
            SetCameraMode(CMODE_NORMAL);
        }
        if (*(u16 *)&Me_MOTION_C->attribute & 0x40)
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
}
