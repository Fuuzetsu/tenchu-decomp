#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SwimCheck(void);
 *     MOTION.C:219, 33 src lines, frame 40 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
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
 *     stack sp+16     struct VECTOR vect
 *     reg   $a2       struct ModelArchiveType * mdl
 *     reg   $a1       short i
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short motID;
 *     extern struct MotionManager *dtM;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct VECTOR *dtL;
 *     extern struct Humanoid *StagePlayer;
 *     extern short ActionHalt;
 *     extern short motMODE;
 *     extern short MotionUpdateMode;
 *     extern struct HumanAnimType CVAhuman[5];
 * END PSX.SYM */

extern Humanoid *Me_MOTION_C;
extern s16 motID;
extern s16 motMODE;

extern void AttackCancelControl(short mode);
extern void PadShockAR(short port, short power, short time, short mode);
extern void FUN_800270f8(Humanoid *human, short hide);
extern int ReqLifeBar(Humanoid *h);
extern void reset_alert_duration(void);

short SwimCheck(void)
{
    short status;
    short i;
    short j;
    VECTOR splash;
    VECTOR *locate;
    int object_id;
    int r;
    u16 motion;
    long width;

    if (Me_MOTION_C->map.height <= 0)
    {
        if ((Me_MOTION_C->map.attrib & 4) == 0)
        {
            return 0;
        }
        status = Me_MOTION_C->status;
        if (status == 4)
        {
            return 0;
        }
        if (status == 3)
        {
            goto return_one;
        }
        if (status == 0x11)
        {
            if (motID == 0x1108)
            {
                goto return_one;
            }
            if (dtM->loop == -1)
            {
                return 0;
            }
        }

        if (Me_MOTION_C->status != 0xb)
        {
            object_id = (*Me_MOTION_C->model->object)->id;
            if (object_id >= 0)
            {
                locate = dtL;
                dtL->vx = ConflictObject[object_id].position.vx;
                locate->vz = ConflictObject[object_id].position.vz;
            }
        }

        splash.vy = Me_MOTION_C->map.level;
        i = 0;
        do
        {
            r = rand();
            width = Me_MOTION_C->width;
            splash.vx = dtL->vx + (r % width) * 2 - width;
            r = rand();
            width = Me_MOTION_C->width;
            splash.vz = dtL->vz + (r % width) * 2 - width;
            SetSplash(&splash, (rand() & 7) << 12,
                      (rand() & 7) << 12, 6);
            i++;
        } while (i < 20);

        AttackCancelControl(3);
        if (Me_MOTION_C == StagePlayer)
        {
            SetCameraMode(CMODE_SWIM);
            PadShockAR(0, 0xff, 10, 0);
        }
        ActionHalt = 0;
        FUN_800270f8(Me_MOTION_C, 1);
        motion = GetMotionID(dtM, 0x300);
        if ((s16)motion < 0 || Me_MOTION_C->life == 0)
        {
            motID = 0x1108;
            motMODE = 1;
            Sound(Me_MOTION_C, 8);
            Me_MOTION_C->life = 0;
            ReqLifeBar(Me_MOTION_C);
        }
        else
        {
            motID = 0x300;
            motMODE = 1;
        }

        if (MotionUpdateMode != 0)
        {
            j = 0;
            do
            {
                if (CVAhuman[j].human == Me_MOTION_C)
                {
                    goto motion_done;
                }
                j++;
            } while (j < 5);
        }
        SetNowMotion(Me_MOTION_C, motID, motMODE);
        motMODE = -1;
    motion_done:
        Sound(Me_MOTION_C, 0x16);
        reset_alert_duration();
        goto return_one;
    }
    return 0;
return_one:
    return 1;
}

/* Matching notes:
 * - Keep the two color rand() calls inside SetSplash's arguments.  cc1 then
 *   materializes the first shifted color across the second call, exactly as
 *   the retail scheduler does.
 * - The two loop counters are distinct locals: sharing one makes the later
 *   CVAhuman scan inherit the splash loop's callee-saved register.
 * - Assigning the dtL alias only in the successful conflict-object arm keeps
 *   its gp load at the target's first actual use.
 */

// Ghidra decompilation (reference):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// short SwimCheck(void)
//
// {
//   Humanoid *h;
//   VECTOR *pVVar1;
//   short sVar2;
//   ushort uVar3;
//   int iVar4;
//   uint uVar5;
//   uint uVar6;
//   int iVar7;
//   VECTOR local_20;
//
//   pVVar1 = dtL;
//   sVar2 = 0;
//   if ((Me_MOTION_C->map).height < 1) {
//     if ((((Me_MOTION_C->map).attrib & 4U) == 0) || (sVar2 = Me_MOTION_C->status, sVar2 == 4)) {
//       sVar2 = 0;
//     }
//     else {
//       if (sVar2 != 3) {
//         if (sVar2 == 0x11) {
//           if (motID == 0x1108) {
//             return 1;
//           }
//           if (dtM->loop == -1) {
//             return 0;
//           }
//         }
//         if ((Me_MOTION_C->status != 0xb) &&
//            (iVar7 = (int)(*Me_MOTION_C->model->object)->id, -1 < iVar7)) {
//           dtL->vx = ConflictObject[iVar7].position.vx;
//           pVVar1->vz = ConflictObject[iVar7].position.vz;
//         }
//         local_20.vy = (Me_MOTION_C->map).level;
//         iVar7 = 0;
//         do {
//           iVar4 = rand();
//           local_20.vx = (long)Me_MOTION_C->width;
//           if (local_20.vx == 0) {
//             trap(0x1c00);
//           }
//           if ((local_20.vx == -1) && (iVar4 == -0x80000000)) {
//             trap(0x1800);
//           }
//           local_20.vx = (dtL->vx + (iVar4 % local_20.vx) * 2) - local_20.vx;
//           iVar4 = rand();
//           local_20.vz = (long)Me_MOTION_C->width;
//           if (local_20.vz == 0) {
//             trap(0x1c00);
//           }
//           if ((local_20.vz == -1) && (iVar4 == -0x80000000)) {
//             trap(0x1800);
//           }
//           local_20.vz = (dtL->vz + (iVar4 % local_20.vz) * 2) - local_20.vz;
//           uVar5 = rand();
//           uVar6 = rand();
//           SetSplash(&local_20,(short)((uVar5 & 7) << 0xc),(short)((uVar6 & 7) << 0xc),6);
//           iVar7 = iVar7 + 1;
//         } while (iVar7 * 0x10000 >> 0x10 < 0x14);
//         AttackCancelControl(3);
//         if (Me_MOTION_C == StagePlayer) {
//           SetCameraMode(CMODE_SWIM);
//           PadShockAR(0,0xff,10,0);
//         }
//         ActionHalt = 0;
//         FUN_800270f8(Me_MOTION_C,1);
//         uVar3 = GetMotionID(dtM,0x300);
//         if (((int)((uint)uVar3 << 0x10) < 0) || (Me_MOTION_C->life == 0)) {
//           motID = 0x1108;
//           DAT_80097f0e = 1;
//           Sound(Me_MOTION_C,8);
//           h = Me_MOTION_C;
//           Me_MOTION_C->life = 0;
//           ReqLifeBar(h);
//         }
//         else {
//           motID = 0x300;
//           DAT_80097f0e = 1;
//         }
//         iVar7 = 0;
//         if (MotionUpdateMode != 0) {
//           iVar4 = 0;
//           do {
//             iVar7 = iVar7 + 1;
//             if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar4 >> 0xd)) == Me_MOTION_C)
//             goto LAB_8001cc64;
//             iVar4 = iVar7 * 0x10000;
//           } while (iVar7 * 0x10000 >> 0x10 < 5);
//         }
//         SetNowMotion(Me_MOTION_C,motID,DAT_80097f0e);
//         DAT_80097f0e = -1;
// LAB_8001cc64:
//         Sound(Me_MOTION_C,0x16);
//         FUN_8002f7f4();
//       }
//       sVar2 = 1;
//     }
//   }
//   return sVar2;
// }
