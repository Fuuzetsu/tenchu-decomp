#include "common.h"
#include "main.exe.h"
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

extern MotionManager *dtM;
extern VECTOR *dtL;
extern SVECTOR *dtR;
extern SVECTOR *dtV;
extern Humanoid *Me_MOTION_C;
extern Humanoid *StagePlayer;
extern s16 motID;
extern s16 motMODE;
extern s16 MotionUpdateMode;
extern HumanAnimType CVAhuman[5];
extern TCameraStatus CamState;

extern s32 FUN_8004a368(s32 mode, Humanoid *human);
extern s16 GetDirection(s32 dx, s32 dz, s32 roty);
extern s32 SetFlyWire(VECTOR *start, VECTOR *end);
extern void SetWire(VECTOR *start, VECTOR *end, VECTOR *center, s32 length);
extern void ReqItemUse(PARAM_ITEM_USE *item);
extern void SetCameraMode(s32 mode);
extern s16 SetNowMotion(Humanoid *human, s16 mid, s16 move);
extern void Sound(Humanoid *human, s32 id);

void ActKAGI(void)
{
    VECTOR v;
    long dist;
    PARAM_ITEM_USE item;
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
            item.type = ITEM_KIND_2_KAGINAWA;
            item.user = Me_MOTION_C;
            item.start.vx = dtL->vx;
            item.start.vy = dtL->vy - Me_MOTION_C->height + 300;
            item.start.vz = dtL->vz;
            item.end = item.start;
            ReqItemUse(&item);
            Sound(Me_MOTION_C, 0x1e);
        }
        else if (FUN_8004a368(1, Me_MOTION_C) == 0)
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
            if (dx != 0 || dz != 0)
            {
                goto rope_direction;
            }
            if (Me_MOTION_C == StagePlayer)
            {
                SetCameraMode(0);
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
            goto rope_done;

rope_direction:
            ry = GetDirection(v.vx, v.vz, dtR->vy);
            dtR->vy += ry;
            Sound(Me_MOTION_C, 0x1f);
rope_done:;
        }
        else if (Me_MOTION_C->pad.trig & 0xe0)
        {
            FUN_8004a368(0, Me_MOTION_C);
            if (Me_MOTION_C == StagePlayer)
            {
                SetCameraMode(0);
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

        if ((*(u16 *)&Me_MOTION_C->map.attrib & 4) &&
            ((s8)((u16)motID >> 8) != 4))
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
                    *(u16 *)&model->object[i++]->attribute |= 1;
                } while (i <= ry);
            }
            *(u16 *)&model->object[0]->attribute |= 1;
            motID = 0x300;
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
        if (attrib & 4)
        {
            Sound(human, 0x15);
        }
        Sound(Me_MOTION_C, 0x18);
        break;
    }

    case 0x402:
        SetCameraMode(15);
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
                        *(u16 *)&Me_MOTION_C->attribute |= 0x400;
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
            if ((human->attribute & 0xce00) == 0)
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

// triage: HARD — 524 insns, mul/div, 5 loop, 12 callees, ~0.06 to ProcItemGoshikimai
// likely-relevant cookbook sections:
//   - Loops: 5 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void ActKAGI(void)
//
// {
//   short *psVar1;
//   bool bVar2;
//   Humanoid *pHVar3;
//   SVECTOR *pSVar4;
//   SVECTOR *pSVar5;
//   MotionManager *pMVar6;
//   short sVar7;
//   ushort uVar8;
//   int iVar9;
//   long lVar10;
//   VECTOR *pVVar11;
//   ModelType *pMVar12;
//   ushort uVar13;
//   int iVar14;
//   ModelArchiveType *pMVar15;
//   int local_40;
//   int local_3c;
//   int local_38;
//   PARAM_ITEM_USE local_30;
//
//   sVar7 = dtM->mid;
//   if (sVar7 == 0x401) {
//     if ((dtM->count == 0) && (dtM->loop == 1)) {
//       pVVar11 = GetAbsolutePosition(Me_MOTION_C->model->object[0xe],0,0,0);
//       dtM->loop = -1;
//       iVar14 = SetFlyWire(pVVar11,&CamState.TargetVector);
//       dtM->count = (short)iVar14;
//     }
//     pMVar6 = dtM;
//     if (dtM->loop != -1) {
//       return;
//     }
//     uVar8 = dtM->count - 1;
//     dtM->count = uVar8;
//     pHVar3 = Me_MOTION_C;
//     if (-1 < (int)((uint)uVar8 << 0x10)) {
//       return;
//     }
//     motID = 0x402;
//     pMVar6->mask = -2;
//     DAT_80097f0e = 1;
//     if (((pHVar3->map).attrib & 4U) != 0) {
//       Sound(pHVar3,0x15);
//     }
//     Sound(Me_MOTION_C,0x18);
//     return;
//   }
//   if (0x401 < sVar7) {
//     if (sVar7 != 0x402) {
//       return;
//     }
//     SetCameraMode(CMODE_FALL|CMODE_DIRECTION);
//     local_40 = CamState.TargetVector.vx - dtL->vx;
//     local_3c = CamState.TargetVector.vy - dtL->vy;
//     local_38 = CamState.TargetVector.vz - dtL->vz;
//     lVar10 = SquareRoot0(local_40 * local_40 + local_38 * local_38);
//     sVar7 = GetDirection(local_40,local_38,dtR->vy);
//     ((*Me_MOTION_C->model->object)->rotate).vy = sVar7;
//     lVar10 = ratan2(lVar10,-local_3c);
//     pHVar3 = Me_MOTION_C;
//     ((*Me_MOTION_C->model->object)->rotate).vx = (short)lVar10;
//     UpdateCoordinate(*pHVar3->model->object);
//     iVar14 = local_40;
//     if (local_40 < 0) {
//       iVar14 = -local_40;
//     }
//     if (iVar14 < 400) {
//       iVar14 = local_3c;
//       if (local_3c < 0) {
//         iVar14 = -local_3c;
//       }
//       if (iVar14 < 400) {
//         iVar14 = local_38;
//         if (local_38 < 0) {
//           iVar14 = -local_38;
//         }
//         if (iVar14 < 400) {
//           Me_MOTION_C->attribute = Me_MOTION_C->attribute | 0x400;
//         }
//       }
//     }
//     pSVar5 = dtV;
//     pSVar4 = dtR;
//     pHVar3 = Me_MOTION_C;
//     if (((int)Me_MOTION_C->attribute & 0xce00U) == 0) {
//       iVar14 = local_40;
//       if (local_40 < 0) {
//         iVar14 = -local_40;
//       }
//       if (400 < iVar14) goto LAB_8002119c;
//       iVar14 = local_3c;
//       if (local_3c < 0) {
//         iVar14 = -local_3c;
//       }
//       if (400 < iVar14) goto LAB_8002119c;
//       iVar14 = local_38;
//       if (local_38 < 0) {
//         iVar14 = -local_38;
//       }
//       while (400 < iVar14) {
// LAB_8002119c:
//         do {
//           do {
//             local_40 = local_40 >> 1;
//             local_3c = local_3c >> 1;
//             local_38 = local_38 >> 1;
//             iVar14 = local_40;
//             if (local_40 < 0) {
//               iVar14 = -local_40;
//             }
//           } while (400 < iVar14);
//           iVar14 = local_3c;
//           if (local_3c < 0) {
//             iVar14 = -local_3c;
//           }
//         } while (400 < iVar14);
//         iVar14 = local_38;
//         if (local_38 < 0) {
//           iVar14 = -local_38;
//         }
//       }
//       dtV->vx = (short)local_40;
//       pSVar5->vy = (short)local_3c;
//       pSVar5->vz = (short)local_38;
//       pVVar11 = GetAbsolutePosition(pHVar3->model->object[0xe],0,0,0);
//       SetWire(pVVar11,&CamState.TargetVector,(VECTOR *)0x0,0x1000);
//       return;
//     }
//     sVar7 = dtR->vy;
//     uVar8 = sVar7 + ((*Me_MOTION_C->model->object)->rotate).vy;
//     uVar13 = uVar8 & 0xc00;
//     dtR->vy = uVar8;
//     if ((uVar8 & 0x200) != 0) {
//       uVar13 = uVar13 + 0x400;
//     }
//     pSVar4->vy = uVar13;
//     pMVar6 = dtM;
//     pMVar12 = *pHVar3->model->object;
//     motID = 0x803;
//     DAT_80097f0e = 0;
//     (pMVar12->rotate).vy = (pMVar12->rotate).vy + (sVar7 - uVar13);
//     bVar2 = MotionUpdateMode != 0;
//     pMVar6->mask = 0x7fff;
//     if (bVar2) {
//       iVar9 = 0;
//       iVar14 = 0;
//       do {
//         iVar9 = iVar9 + 1;
//         if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar14 >> 0xd)) == pHVar3) goto LAB_800210c8;
//         iVar14 = iVar9 * 0x10000;
//       } while (iVar9 * 0x10000 >> 0x10 < 5);
//     }
//     SetNowMotion(Me_MOTION_C,motID,DAT_80097f0e);
//     DAT_80097f0e = -1;
// LAB_800210c8:
//     pHVar3 = Me_MOTION_C;
//     dtM->count = dtM->count >> 1;
//     pSVar4 = dtV;
//     if ((pHVar3->map).vector == '\x0f') {
//       psVar1 = &dtV->vz;
//       dtV->vx = dtV->vx >> 1;
//       pSVar4->vz = *psVar1 >> 1;
//     }
//     else {
//       dtV->vz = 0;
//       pSVar4->vx = 0;
//     }
//     dtV->vy = 0;
//     return;
//   }
//   if (sVar7 != 0x400) {
//     return;
//   }
//   if ((dtM->count == 0) && (dtM->loop != 0)) {
//     dtM->loop = -1;
//   }
//   if (dtM->count == 1) {
//     local_30.type = ITEM_KAGINAWA;
//     local_30.user = Me_MOTION_C;
//     local_30.start.vx = dtL->vx;
//     local_30.start.vy = (dtL->vy - (int)Me_MOTION_C->height) + 300;
//     local_30.start.vz = dtL->vz;
//     local_30.end.vx = local_30.start.vx;
//     local_30.end.vy = local_30.start.vy;
//     local_30.end.vz = local_30.start.vz;
//     ReqItemUse(&local_30);
//     Sound(Me_MOTION_C,0x1e);
//     goto LAB_80020c90;
//   }
//   iVar14 = FUN_8004a368(1,Me_MOTION_C);
//   if (iVar14 == 0) {
//     DAT_80097f0e = 1;
//     iVar14 = CamState.TargetVector.vx - dtL->vx;
//     motID = 0x401;
//     iVar9 = CamState.TargetVector.vz - dtL->vz;
//     if ((iVar14 != 0) || (iVar9 != 0)) {
//       sVar7 = GetDirection(iVar14,iVar9,dtR->vy);
//       pHVar3 = Me_MOTION_C;
//       dtR->vy = dtR->vy + sVar7;
//       Sound(pHVar3,0x1f);
//       goto LAB_80020c90;
//     }
//     if (Me_MOTION_C == StagePlayer) {
//       SetCameraMode(CMODE_NORMAL);
//     }
//     if ((Me_MOTION_C->attribute & 0x40U) == 0) {
// LAB_80020c88:
//       motID = 0;
//     }
//     else {
//       motID = 0x501;
//     }
//   }
//   else {
//     if (((Me_MOTION_C->pad).trig & 0xe0) == 0) goto LAB_80020c90;
//     FUN_8004a368(0);
//     if (Me_MOTION_C == StagePlayer) {
//       SetCameraMode(CMODE_NORMAL);
//     }
//     if ((Me_MOTION_C->attribute & 0x40U) == 0) goto LAB_80020c88;
//     motID = 0x501;
//   }
//   DAT_80097f0e = 1;
// LAB_80020c90:
//   if ((((Me_MOTION_C->map).attrib & 4U) != 0) && ((int)((uint)(ushort)motID << 0x10) >> 0x18 != 4))
//   {
//     pMVar15 = Me_MOTION_C->model;
//     sVar7 = pMVar15->n + -1;
//     if (0xc < pMVar15->n) {
//       sVar7 = 0xc;
//     }
//     iVar14 = 7;
//     if (6 < sVar7) {
//       do {
//         iVar9 = iVar14 << 0x10;
//         iVar14 = iVar14 + 1;
//         iVar9 = *(int *)((iVar9 >> 0xe) + (int)pMVar15->object);
//         *(ushort *)(iVar9 + 0x5a) = *(ushort *)(iVar9 + 0x5a) | 1;
//       } while (iVar14 * 0x10000 >> 0x10 <= (int)sVar7);
//     }
//     (*pMVar15->object)->attribute = (*pMVar15->object)->attribute | 1;
//     motID = 0x300;
//     DAT_80097f0e = 1;
//     dtM->mask = 0x7fff;
//   }
//   return;
// }
