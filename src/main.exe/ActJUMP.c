#include "common.h"
#include "main.exe.h"
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

typedef struct
{
    MapVector map;
    SVECTOR speed;
} ActJumpScratch;

extern Humanoid *Me_MOTION_C;
extern MotionManager *dtM;
extern VECTOR *dtL;
extern SVECTOR *dtV;
extern SVECTOR *dtR;
extern s16 dtPAD;
extern s16 motID;
extern s16 motMODE;
extern u16 RefrectVector[16];

extern short UpdateMotion(MotionManager *mmp, short mid);
extern short SetNowMotion(Humanoid *human, short mid, short move);
extern long GetAreaMapVector(unsigned long *area, MapVector *mvp, VECTOR *pos,
                             long wide, int mode);
extern long GetAreaMapLevel(unsigned long *area, long x, long y, long z,
                            long mode);
extern void MoveHumanoid(Humanoid *human, short front, short side);
extern void GetMoveSpeed(SVECTOR *speed, long ry, long front, long side);
extern short Sound(Humanoid *human, short seid);
extern void PadShockAR(short port, short power, short time, short mode);

/* Jump-state motion, collision response, air steering, and landing control. */
void ActJUMP(void)
{
    short old_mid;
    u16 pad;
    ActJumpScratch scratch;
    short reflected;
    short i;
    long level;
    long vertical;
    long scaled;
    SVECTOR *velocity;

    if ((Me_MOTION_C->pad.trig & 0x40) != 0 && motID != 0x901)
    {
        GetAreaMapVector(GlobalAreaMap, &scratch.map, dtL,
                         Me_MOTION_C->width + 300, 0);
        if (scratch.map.vector == 0)
        {
            return;
        }
        if (UpdateMotion(dtM, 0x901) == 0)
        {
            return;
        }
        reflected = RefrectVector[scratch.map.vector];
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
            if (old_mid == 0x906)
            {
                goto half_count;
            }
            if (old_mid != 0x907)
            {
                return;
            }
            dtR->vy += 0x800;
            (*Me_MOTION_C->model->object)->rotate.vy = 0;
        half_count:
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

        if (((s16)dtPAD & 0xf000) != 0 && motID != 0x906)
        {
            pad = (u16)dtPAD;
            if ((pad & 0x1000) != 0)
            {
                GetMoveSpeed(&scratch.speed, dtR->vy, 10, 0);
            }
            else if ((pad & 0x4000) != 0)
            {
                GetMoveSpeed(&scratch.speed, dtR->vy, -10, 0);
            }
            else if ((pad & 0x2000) != 0)
            {
                GetMoveSpeed(&scratch.speed, dtR->vy, 0, -10);
            }
            else
            {
                GetMoveSpeed(&scratch.speed, dtR->vy, 0, 10);
            }
            scratch.speed.vx = scratch.speed.vx + dtV->vx;
            scratch.speed.vz = scratch.speed.vz + dtV->vz;
            if (__builtin_abs(scratch.speed.vx) < 101)
            {
                if (__builtin_abs(scratch.speed.vz) < 101)
                {
                    dtV->vx = scratch.speed.vx;
                    dtV->vz = scratch.speed.vz;
                }
            }
        }

        if ((dtPAD & 0x80) == 0)
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
        if ((Me_MOTION_C->attribute & 0x40) == 0)
        {
            return;
        }
        motID = 0x70f;
        motMODE = 0;
    }
}

// triage: HARD — 349 insns, 2 loop, 8 callees, ~0.05 to MotionAndMove
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ActJUMP(void)
//
// {
//   short *psVar1;
//   bool bVar2;
//   Humanoid *pHVar3;
//   ushort uVar4;
//   SVECTOR *pSVar5;
//   short sVar6;
//   long lVar7;
//   int iVar8;
//   uint uVar9;
//   int iVar10;
//   short ry;
//   short side;
//   VECTOR VStack_28;
//   SVECTOR local_10;
//
//   sVar6 = motID;
//   uVar4 = dtPAD;
//   if ((((Me_MOTION_C->pad).trig & 0x40) != 0) && (motID != 0x901)) {
//     GetAreaMapVector((MapVector *)GlobalAreaMap,&VStack_28,(long)dtL);
//     if ((byte)VStack_28.pad == 0) {
//       return;
//     }
//     sVar6 = UpdateMotion(dtM,0x901);
//     if (sVar6 == 0) {
//       return;
//     }
//     sVar6 = RefrectVector[(byte)VStack_28.pad];
//     dtL->vy = dtL->vy + -500;
//     pSVar5 = dtV;
//     pHVar3 = Me_MOTION_C;
//     if (sVar6 == -1) {
//       psVar1 = &dtV->vz;
//       dtV->vx = -dtV->vx;
//       pSVar5->vz = -*psVar1;
//     }
//     else {
//       dtR->vy = sVar6 + 0x800;
//       MoveHumanoid(pHVar3,-100,0);
//     }
//     Sound(Me_MOTION_C,0x48);
//     return;
//   }
//   if (((Me_MOTION_C->attribute & 0xa00U) == 0) || (dtM->count < 2)) {
//     if ((dtM->count == 0) && (dtM->loop != 0)) {
//       motID = 0x803;
//       DAT_80097f0e = 0;
//       iVar10 = 0;
//       if (MotionUpdateMode != 0) {
//         iVar8 = 0;
//         do {
//           iVar10 = iVar10 + 1;
//           if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar8 >> 0xd)) == Me_MOTION_C)
//           goto LAB_80024510;
//           iVar8 = iVar10 * 0x10000;
//         } while (iVar10 * 0x10000 >> 0x10 < 5);
//       }
//       SetNowMotion(Me_MOTION_C,0x803,0);
//       DAT_80097f0e = 0xffff;
// LAB_80024510:
//       pHVar3 = Me_MOTION_C;
//       if (sVar6 != 0x906) {
//         if (sVar6 != 0x907) {
//           return;
//         }
//         dtR->vy = dtR->vy + 0x800;
//         ((*pHVar3->model->object)->rotate).vy = 0;
//       }
//       dtM->count = dtM->count >> 1;
//       return;
//     }
//     sVar6 = dtM->count - (dtM->motion->time >> 1);
//     if (motID == 0x906) {
//       sVar6 = sVar6 * 10;
//     }
//     else {
//       sVar6 = sVar6 * 0x14;
//     }
//     uVar9 = (uint)(short)dtPAD;
//     dtV->vy = sVar6;
//     if (((uVar9 & 0xf000) != 0) && (motID != 0x906)) {
//       sVar6 = 10;
//       if ((uVar4 & 0x1000) == 0) {
//         sVar6 = -10;
//         if ((uVar4 & 0x4000) == 0) {
//           if ((uVar4 & 0x2000) == 0) {
//             sVar6 = 0;
//             ry = dtR->vy;
//             side = 10;
//           }
//           else {
//             sVar6 = 0;
//             ry = dtR->vy;
//             side = -10;
//           }
//         }
//         else {
//           ry = dtR->vy;
//           side = 0;
//         }
//       }
//       else {
//         ry = dtR->vy;
//         side = 0;
//       }
//       GetMoveSpeed(&local_10,ry,sVar6,side);
//       pSVar5 = dtV;
//       local_10.vx = local_10.vx + dtV->vx;
//       local_10.vz = local_10.vz + dtV->vz;
//       iVar10 = (int)local_10.vx;
//       if (iVar10 < 0) {
//         iVar10 = -iVar10;
//       }
//       if (iVar10 < 0x65) {
//         iVar10 = (int)local_10.vz;
//         if (iVar10 < 0) {
//           iVar10 = -iVar10;
//         }
//         if (iVar10 < 0x65) {
//           dtV->vx = local_10.vx;
//           pSVar5->vz = local_10.vz;
//         }
//       }
//     }
//     if ((dtPAD & 0x80) == 0) {
//       return;
//     }
//     if (motID == 0x907) {
//       return;
//     }
//     if (dtV->vy < 1) {
//       return;
//     }
//     sVar6 = 0x70f;
//     if ((Me_MOTION_C->attribute & 0x40U) == 0) {
//       return;
//     }
//   }
//   else {
//     lVar7 = GetAreaMapLevel(GlobalAreaMap,dtL->vx,dtL->vy);
//     pHVar3 = Me_MOTION_C;
//     if (dtL->vy < lVar7) {
//       motID = 0x803;
//       DAT_80097f0e = 0;
//       iVar10 = 0;
//       if (MotionUpdateMode != 0) {
//         iVar8 = 0;
//         do {
//           iVar10 = iVar10 + 1;
//           if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar8 >> 0xd)) == Me_MOTION_C)
//           goto LAB_800243bc;
//           iVar8 = iVar10 * 0x10000;
//         } while (iVar10 * 0x10000 >> 0x10 < 5);
//       }
//       SetNowMotion(Me_MOTION_C,0x803,0);
//       DAT_80097f0e = 0xffff;
// LAB_800243bc:
//       Sound(Me_MOTION_C,6);
//       bVar2 = Me_MOTION_C != StagePlayer;
//       dtM->count = dtM->count >> 2;
//       if (bVar2) {
//         return;
//       }
//       PadShockAR(0,0xff,10,0);
//       return;
//     }
//     sVar6 = 0x804;
//     if (motID == 0x907) {
//       dtR->vy = dtR->vy + ((*Me_MOTION_C->model->object)->rotate).vy;
//       motID = 0x806;
//       DAT_80097f0e = 1;
//       ((*pHVar3->model->object)->rotate).vy = 0;
//       return;
//     }
//   }
//   DAT_80097f0e = 0;
//   motID = sVar6;
//   return;
// }
