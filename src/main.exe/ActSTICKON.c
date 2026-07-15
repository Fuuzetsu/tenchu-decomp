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
 *     extern struct NodeIndexType *FieldIndex;
 * END PSX.SYM */

/*
 * ActSTICKON (0x80025120) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=ActSTICKON
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSTICKON", ActSTICKON);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSTICKON", switchD_80025180__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSTICKON", switchD_80025180__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSTICKON", switchD_80025180__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSTICKON", switchD_80025180__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/ActSTICKON", switchD_80025180__caseD_5);

/* jump-table pool @ 0x800115c0 (5 words; tables at 0x800115c0) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 ActSTICKON_jtbl[5] = {
    0x80025188, 0x80025664, 0x80025664, 0x800259C0,
    0x800259C0,
};

#else /* NON_MATCHING */

typedef struct
{
    s32 level;
    s32 height;
    s16 attrib;
    s16 degree;
    u8 vector;
    u8 direct;
    u8 angleL;
    u8 angleH;
} StickonMapVector;

typedef struct
{
    Humanoid *human;
    s16 loop;
    s16 motid;
} HumanAnimType;

typedef struct
{
    VECTOR TargetVector;
    Humanoid *Owner;
    s32 Mode;
    s16 DirectionRX;
    s16 DirectionRY;
    s32 OldMode;
    u8 Valiation;
} TCameraStatus;

enum
{
    ITEM_MAKIBISHI = 2,
    ITEM_FIRE = 4,
    ITEM_SMOKE = 5,
    ITEM_DOKUDANGO = 7,
    CMODE_STICK_L = 6,
    CMODE_STICK_R = 7,
    CMODE_PEEP_L = 9,
    CMODE_PEEP_R = 10
};

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
extern HumanAnimType CVAhuman[5];
extern Humanoid *StagePlayer;
extern TCameraStatus CamState;
extern s16 SelectedItem;
extern s32 D_80097EF0;

extern StickonMapVector *StickonCheck(void);
extern void GetMoveSpeed(SVECTOR *vect, s16 ry, s16 order, s16 side);
extern s16 SetNowMotion(Humanoid *human, s16 mid, s16 move);
extern void SetCameraMode(s32 mode);
extern s16 Sound(Humanoid *human, s16 id);
extern s32 rsin(s32 angle);
extern s32 rcos(s32 angle);
extern int ReqItemMakibishi(PARAM_ITEM_USE *item);
extern int ReqItemFire(PARAM_ITEM_USE *item);
extern int ReqItemSmoke(PARAM_ITEM_USE *item);
extern int ReqItemDokudango(PARAM_ITEM_USE *item);

void ActSTICKON(void)
{
    StickonMapVector *map;
    ModelArchiveType *model;
    short y;
    short rv;
    short pd;
    SVECTOR vect;
    PARAM_ITEM_USE item;
    short i;
    short drop_index;

    model = Me_MOTION_C->model;
    switch ((short)(dtM->mid - 0xC00))
    {
    case 0:
        if (dtM->count < 0)
        {
            MotionElementType *rotation;
            u32 reflected_raw;
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

            do
            {
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
            } while (0);
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
            s32 loop_pad;

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
                if (rv != ((pd + 2) & 3))
                {
                    y = 0xC02;
                    if (rv == ((pd + 1) & 3))
                    {
                        y = 0xC01;
                    }
                    UpdateMotion(dtM, y);
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
            s32 camera_direction;
            u32 camera_rv;

            camera_rv = (u16)model->object[0]->rotate.vy >> 10 & 3;
            pd = 0;
            if (CamState.Mode == CMODE_STICK_R)
            {
                goto camera_stick_right;
            }
            if ((u32)CamState.Mode < 8)
            {
                camera_direction = 2;
                if (CamState.Mode == CMODE_STICK_L)
                {
                    goto camera_left;
                }
            }
            else
            {
                if (CamState.Mode == CMODE_PEEP_L)
                {
                    camera_direction = 3;
                    goto camera_left;
                }
                camera_direction = 1;
                if (CamState.Mode == CMODE_PEEP_R)
                {
                    goto camera_right;
                }
            }
            goto camera_done;

camera_left:
            if (camera_rv == camera_direction)
            {
                pd = 0xC03;
            }
            goto camera_done;

camera_stick_right:
            camera_direction = 2;

camera_right:
            if (camera_rv == camera_direction)
            {
                pd = 0xC04;
            }

camera_done:

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

            if (pd == 0)
            {
                SoundEx(Me_MOTION_C->locate, 0xC);
            }
            else
            {
                motMODE = 1;
                motID = pd;
                dtM->mask = -2;
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

        if (pd == 0)
        {
            angle += 0x200;
        }
        else
        {
            angle -= 0x200;
        }

        if (item.type == ITEM_MAKIBISHI)
        {
            for (drop_index = 0; drop_index < 5; drop_index++)
            {
                next_angle = angle - 10 + rand() % 20;
                angle = next_angle;
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
            SetCameraMode(0);
        }
        if (*(u16 *)&Me_MOTION_C->attribute & 0x40)
        {
            motID = 0x501;
        }
        else
        {
            motID = 0;
        }
        motMODE = 1;
    }
}

/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// void ActSTICKON(void)
// 
// {
//   bool bVar1;
//   ushort uVar2;
//   ushort uVar3;
//   SVECTOR *pSVar4;
//   MotionManager *pMVar5;
//   ushort uVar6;
//   ushort uVar7;
//   uint uVar8;
//   MapVector *pMVar9;
//   VECTOR *pVVar10;
//   int iVar11;
//   int iVar12;
//   uint uVar13;
//   short sVar14;
//   ModelArchiveType *pMVar15;
//   int a;
//   int iVar16;
//   short sVar17;
//   int iVar18;
//   SVECTOR local_50;
//   PARAM_ITEM_USE local_48;
//   
//   pMVar15 = Me_MOTION_C->model;
//   switch((int)(((ushort)dtM->mid - 0xc00) * 0x10000) >> 0x10) {
//   case 0:
//     if (dtM->count < 0) {
//       pMVar9 = StickonCheck();
//       sVar17 = 0xb00;
//       if (pMVar9 == (MapVector *)0x0) goto LAB_80025d28;
//       uVar7 = dtR->vy;
//       uVar13 = uVar7 & 0xc00;
//       if ((uVar7 & 0x200) != 0) {
//         uVar13 = uVar13 + 0x400;
//       }
//       uVar8 = (ushort)RefrectVector[pMVar9->vector] - uVar13;
//       iVar11 = (int)(uVar8 * 0x10000) >> 0x10;
//       if (iVar11 == 0) {
//         dtR->vy = uVar7 + 0x800;
//       }
//       pMVar5 = dtM;
//       iVar18 = iVar11;
//       if (iVar11 < 0) {
//         iVar18 = -iVar11;
//       }
//       if ((0x800 < iVar18) && (uVar8 = iVar11 - 0x1000, iVar11 < 1)) {
//         uVar8 = iVar11 + 0x1000;
//       }
//       iVar18 = (int)(short)uVar13 - (int)dtR->vy;
//       iVar11 = -(int)dtM->count;
//       if (iVar11 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar11 == -1) && (iVar18 == -0x80000000)) {
//         trap(0x1800);
//       }
//       dtR->vy = dtR->vy + (short)(iVar18 / iVar11);
//       sVar17 = (short)uVar8;
//       pMVar5->motion->rotate[0]->y = sVar17;
//       iVar11 = *(int *)&pMVar5->motion[1].time;
//       if ((uVar8 & 0x400) == 0) {
//         *(undefined2 *)(iVar11 + 2) = 0;
//       }
//       else {
//         *(short *)(iVar11 + 2) = -sVar17;
//       }
//       GetMoveSpeed(&local_50,sVar17,-300,0);
//       pMVar5 = dtM;
//       dtM->motion->locate->x = local_50.vx;
//       pMVar5->motion->locate->z = local_50.vz;
//     }
//     else if (0 < dtM->loop) {
//       dtM->loop = -1;
//     }
//     if (dtCMD != 0) {
//       if (dtCMD == 0x12) {
//         sVar17 = 0xb06;
// LAB_80025390:
//         DAT_80097f0e = 1;
//         motID = sVar17;
//       }
//       else {
//         if (0x12 < dtCMD) {
//           if (dtCMD == 0x13) {
//             sVar17 = 0xb08;
//           }
//           else {
//             sVar17 = 0xb07;
//             if (dtCMD != 0x14) goto LAB_8002539c;
//           }
//           goto LAB_80025390;
//         }
//         sVar17 = 0xb05;
//         if (dtCMD == 0x11) goto LAB_80025390;
//       }
// LAB_8002539c:
//       if ((int)((uint)(ushort)motID << 0x10) >> 0x18 == 0xb) {
//         bVar1 = MotionUpdateMode != 0;
//         dtM->mask = 0x7fff;
//         if (bVar1) {
//           iVar18 = 0;
//           iVar11 = 0;
//           do {
//             iVar18 = iVar18 + 1;
//             if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar11 >> 0xd)) == Me_MOTION_C)
//             goto LAB_80025754;
//             iVar11 = iVar18 * 0x10000;
//           } while (iVar18 * 0x10000 >> 0x10 < 5);
//         }
// LAB_80025738:
//         SetNowMotion(Me_MOTION_C,motID,DAT_80097f0e);
//         DAT_80097f0e = -1;
// LAB_80025754:
//         dtM->count = -5;
//         goto switchD_80025180_caseD_5;
//       }
//     }
//     if (dtM->loop != -1) goto switchD_80025180_caseD_5;
//     uVar13 = (int)((uint)dtPAD << 0x10) >> 0x10;
//     if ((uVar13 & 0xf000) != 0) {
//       sVar17 = 0;
//       if (((int)((uint)dtPAD << 0x10) >> 0x1c & 1U) == 0) {
//         sVar14 = 1;
//         do {
//           sVar17 = sVar14;
//           sVar14 = sVar17 + 1;
//         } while (((int)uVar13 >> ((int)sVar17 + 0xcU & 0x1f) & 1U) == 0);
//       }
//       uVar13 = (uint)(short)((ushort)((*pMVar15->object)->rotate).vy >> 10 & 3);
//       if (uVar13 != ((int)sVar17 + 2U & 3)) {
//         sVar14 = 0xc02;
//         if (uVar13 == ((int)sVar17 + 1U & 3)) {
//           sVar14 = 0xc01;
//         }
//         UpdateMotion(dtM,sVar14);
//         pSVar4 = dtV;
//         Me_MOTION_C->status = 0xc;
//         pSVar4->vz = 0;
//         pSVar4->vx = 0;
//         dtM->mask = -2;
//         ((*pMVar15->object)->rotate).vx = -0x69;
//         UpdateCoordinate(*pMVar15->object);
//       }
//       goto switchD_80025180_caseD_5;
//     }
//     if (((Me_MOTION_C->pad).trig & 0x10) == 0) goto switchD_80025180_caseD_5;
//     uVar7 = (ushort)((*pMVar15->object)->rotate).vy >> 10 & 3;
//     sVar17 = 0;
//     if (CamState.Mode == CMODE_STICK_R) {
//       uVar6 = 2;
// LAB_800255d0:
//       if (uVar7 == uVar6) {
//         sVar17 = 0xc04;
//       }
//     }
//     else if (CamState.Mode < CMODE_SWIM) {
//       uVar6 = 2;
//       if (CamState.Mode == CMODE_STICK_L) {
// LAB_800255bc:
//         if (uVar7 == uVar6) {
//           sVar17 = 0xc03;
//         }
//       }
//     }
//     else {
//       if (CamState.Mode == CMODE_PEEP_L) {
//         uVar6 = 3;
//         goto LAB_800255bc;
//       }
//       uVar6 = 1;
//       if (CamState.Mode == CMODE_PEEP_R) goto LAB_800255d0;
//     }
//     DAT_80097ef0 = (TItemType)DAT_80097b1e;
//     if ((int)DAT_80097ef0 < 6) {
//       if (((int)DAT_80097ef0 < 4) && (DAT_80097ef0 != ITEM_MAKIBISHI)) {
//         sVar17 = 0;
//       }
//     }
//     else if (DAT_80097ef0 != ITEM_DOKUDANGO) {
//       sVar17 = 0;
//     }
//     if (sVar17 == 0) {
//       SoundEx(Me_MOTION_C->locate,0xc);
//     }
//     else {
//       DAT_80097f0e = 1;
//       motID = sVar17;
//       dtM->mask = -2;
//     }
//     goto switchD_80025180_caseD_5;
//   case 1:
//   case 2:
//     break;
//   case 3:
//   case 4:
//     if (dtM->count != 0) {
//       return;
//     }
//     if (dtM->loop == 0) {
//       return;
//     }
//     bVar1 = motID != 0xc03;
//     sVar17 = ((*pMVar15->object)->rotate).vy + dtR->vy;
//     if (bVar1) {
//       uVar7 = sVar17 - 0x400;
//     }
//     else {
//       uVar7 = sVar17 + 0x400;
//     }
//     local_48.user = Me_MOTION_C;
//     local_48.type = DAT_80097ef0;
//     Me_MOTION_C->item[DAT_80097ef0] = Me_MOTION_C->item[DAT_80097ef0] + 0xff;
//     iVar16 = (int)(short)(ushort)bVar1;
//     pVVar10 = GetAbsolutePosition(Me_MOTION_C->model->object[iVar16 + 0xd],0,0,0);
//     iVar18 = (int)(short)(uVar7 & 0xf00);
//     iVar11 = rsin(iVar18);
//     pVVar10->vx = pVVar10->vx - (iVar11 * 500 >> 0xc);
//     iVar11 = rcos(iVar18);
//     local_48.start.vx = pVVar10->vx;
//     pVVar10->vz = pVVar10->vz - (iVar11 * 500 >> 0xc);
//     local_48.start.vy = pVVar10->vy;
//     local_48.start.vz = pVVar10->vz;
//     if (iVar16 == 0) {
//       iVar18 = iVar18 + 0x200;
//     }
//     else {
//       iVar18 = iVar18 + -0x200;
//     }
//     iVar11 = 0;
//     if (local_48.type == ITEM_MAKIBISHI) {
//       do {
//         iVar16 = rand();
//         iVar18 = iVar18 + -10 + iVar16 % 0x14;
//         a = iVar18 * 0x10000 >> 0x10;
//         iVar16 = rsin(a);
//         iVar12 = rand();
//         local_48.end.vx = iVar16 * (-0x1e - iVar12 % 200) >> 0xc;
//         local_48.end.vy = rand();
//         local_48.end.vy = (local_48.end.vy / 0x1e) * 0x1e - local_48.end.vy;
//         iVar16 = rcos(a);
//         iVar12 = rand();
//         local_48.end.vz = iVar16 * (-0x1e - iVar12 % 200) >> 0xc;
//         ReqItemMakibishi((PARAM_ITEM_DROP *)&local_48);
//         iVar11 = iVar11 + 1;
//       } while (iVar11 * 0x10000 >> 0x10 < 5);
//     }
//     else {
//       iVar11 = rsin((int)(short)iVar18);
//       local_48.end.vx = iVar11 * -0x78 >> 0xc;
//       local_48.end.vy = 0;
//       iVar11 = rcos((int)(short)iVar18);
//       local_48.end.vz = iVar11 * -0x78 >> 0xc;
//       if (local_48.type == ITEM_SMOKE) {
//         ReqItemSmoke(&local_48);
//       }
//       else if (local_48.type < ITEM_JIRAI) {
//         if (local_48.type == ITEM_FIRE) {
//           ReqItemFire(&local_48);
//         }
//       }
//       else if (local_48.type == ITEM_DOKUDANGO) {
//         ReqItemDokudango(&local_48);
//       }
//     }
//     sVar17 = 0xc00;
// LAB_80025d28:
//     DAT_80097f0e = 1;
//     motID = sVar17;
//     dtM->mask = 0x7fff;
//     return;
//   default:
//     goto switchD_80025180_caseD_5;
//   }
//   if (dtCMD != 0) {
//     if (dtCMD == 0x12) {
//       sVar17 = 0xb06;
// LAB_800256bc:
//       DAT_80097f0e = 1;
//       motID = sVar17;
//     }
//     else {
//       if (0x12 < dtCMD) {
//         if (dtCMD == 0x13) {
//           sVar17 = 0xb08;
//         }
//         else {
//           sVar17 = 0xb07;
//           if (dtCMD != 0x14) goto LAB_800256c8;
//         }
//         goto LAB_800256bc;
//       }
//       sVar17 = 0xb05;
//       if (dtCMD == 0x11) goto LAB_800256bc;
//     }
// LAB_800256c8:
//     if ((int)((uint)(ushort)motID << 0x10) >> 0x18 == 0xb) {
//       bVar1 = MotionUpdateMode != 0;
//       dtM->mask = 0x7fff;
//       if (bVar1) {
//         iVar18 = 0;
//         iVar11 = 0;
//         do {
//           iVar18 = iVar18 + 1;
//           if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar11 >> 0xd)) == Me_MOTION_C)
//           goto LAB_80025754;
//           iVar11 = iVar18 * 0x10000;
//         } while (iVar18 * 0x10000 >> 0x10 < 5);
//       }
//       goto LAB_80025738;
//     }
//   }
//   if (dtM->count < 0) goto switchD_80025180_caseD_5;
//   uVar13 = (int)((uint)dtPAD << 0x10) >> 0x10;
//   if ((uVar13 & 0xf000) == 0) {
//     motID = 0xc00;
//     DAT_80097f0e = 1;
//     dtM->mask = 0x7fff;
//     goto switchD_80025180_caseD_5;
//   }
//   uVar6 = (ushort)((*pMVar15->object)->rotate).vy >> 10 & 3;
//   uVar7 = 0;
//   if (((int)((uint)dtPAD << 0x10) >> 0x1c & 1U) == 0) {
//     uVar2 = 1;
//     do {
//       uVar7 = uVar2;
//       uVar2 = uVar7 + 1;
//     } while (((int)uVar13 >> ((int)(short)uVar7 + 0xcU & 0x1f) & 1U) == 0);
//   }
//   if ((int)(short)uVar6 == ((int)(short)uVar7 + 2U & 3)) goto switchD_80025180_caseD_5;
//   sVar17 = 0xc02;
//   if ((int)(short)uVar6 == ((int)(short)uVar7 + 1U & 3)) {
//     sVar17 = 0xc01;
//   }
//   if (motID != sVar17) {
//     UpdateMotion(dtM,sVar17);
//   }
//   sVar17 = 0x1e;
//   if ((dtPAD & 0x1000) == 0) {
//     sVar17 = -0x1e;
//     if ((dtPAD & 0x4000) != 0) {
//       sVar14 = 0;
//       goto LAB_800258a0;
//     }
//     sVar17 = 0;
//     if ((dtPAD & 0x8000) != 0) {
//       sVar14 = 0x1e;
//       goto LAB_800258a0;
//     }
//     sVar14 = -0x1e;
//     if ((dtPAD & 0x2000) != 0) goto LAB_800258a0;
//   }
//   else {
//     sVar14 = 0;
// LAB_800258a0:
//     MoveHumanoid(Me_MOTION_C,sVar17,sVar14);
//   }
//   pSVar4 = dtV;
//   pVVar10 = dtL;
//   uVar2 = ((*pMVar15->object)->rotate).vy;
//   uVar3 = dtR->vy;
//   dtL->vx = dtL->vx + (int)dtV->vx;
//   pVVar10->vz = pVVar10->vz + (int)pSVar4->vz;
//   pMVar9 = StickonCheck();
//   pSVar4 = dtV;
//   pVVar10 = dtL;
//   if (((uint)uVar2 + (uint)uVar3 & 0xfff) != (int)RefrectVector[pMVar9->vector]) {
//     if (uVar6 == uVar7) {
//       dtPAD = 0;
//     }
//     else {
//       dtL->vx = dtL->vx - (int)dtV->vx;
//       pMVar5 = dtM;
//       pVVar10->vz = pVVar10->vz - (int)pSVar4->vz;
//       UpdateMotion(pMVar5,0xc00);
//       pMVar5 = dtM;
//       dtM->loop = -1;
//       pMVar5->mask = 0x7fff;
//     }
//   }
//   pMVar5 = dtM;
//   pSVar4 = dtV;
//   dtV->vz = 0;
//   pSVar4->vx = 0;
//   if (pMVar5->count == 1) {
//     Sound(Me_MOTION_C,0x11);
//   }
// switchD_80025180_caseD_5:
//   if ((dtPAD & 0x20) == 0) {
//     bVar1 = Me_MOTION_C == StagePlayer;
//     dtM->mask = 0x7fff;
//     if (bVar1) {
//       SetCameraMode(CMODE_NORMAL);
//     }
//     if ((Me_MOTION_C->attribute & 0x40U) == 0) {
//       motID = 0;
//     }
//     else {
//       motID = 0x501;
//     }
//     DAT_80097f0e = 1;
//   }
//   return;
// }

#endif /* NON_MATCHING */
