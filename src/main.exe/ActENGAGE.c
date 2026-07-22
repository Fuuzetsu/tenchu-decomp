#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActENGAGE(void);
 *     MOTION.C:1181, 56 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern short motID;
 *     extern short motMODE;
 *     extern short dtCMD;
 *     extern short ActionHalt;
 *     extern struct SVECTOR *dtR;
 *     extern struct SVECTOR *dtV;
 *     extern struct Humanoid *StagePlayer;
 *     extern long GameClock;
 *     extern struct VECTOR *dtL;
 *     extern short SelectedItem;
 * END PSX.SYM */

/*
 * ActENGAGE (0x80021270) — updates the player's engage movement and selects
 * the next command, jump, attack, or item-use motion.
 *
 * Matching notes (1,388 bytes / 347 instructions):
 *  - The successful command and item arms repeat the complete
 *    motID/motMODE/return tail.  jump2 merges the stores onto the final
 *    0x602 arm while retaining the target's separate constant-load islands.
 *  - The two-way switch around the camera branch leaves a referenced case
 *    label that prevents an otherwise over-aggressive cross-jump merge.
 *  - Loading dtV before each component value gives the velocity pointer and
 *    component value the target's $v1/$v0 allocation.
 */

extern s16 dtPAD;
extern s16 motID;
extern s16 motMODE;
extern Humanoid *Me_MOTION_C;

extern void FUN_80033bc0(VECTOR *pos, u16 spread, s16 divisor, s16 count);
extern void JumpControl(void);
extern void AttackControl(void);

void ActENGAGE(void)
{
    short one;
    short mask;
    int random;

    switch ((short)(dtM->mid - 0x500))
    {
    case 1:
    {
        if (dtPAD & 0x2000)
        {
            motID = 0x504;
            motMODE = 0;
            goto engage_case_post;
        }
        if (dtPAD & 0x8000)
        {
            motID = 0x505;
            motMODE = 0;
            goto engage_case_post;
        }
        if (dtCMD == 0x22)
        {
            motID = 0x712;
            motMODE = 1;
            goto engage_case_post;
        }
        if (dtCMD == 0x31)
        {
            motID = 0x907;
            motMODE = 0;
            MoveHumanoid(Me_MOTION_C, 0x78, 0);
            goto engage_case_post;
        }
        if (dtM->count != 0)
            goto engage_case_post;
        random = rand();
        if (random != (random / 20) * 20)
            goto engage_case_post;
        motID = 0x713;
        motMODE = 1;
engage_case_post:
        if (ActionHalt == -1 && dtM->count == 0)
        {
            mask = GetMotionID(dtM, 0x503);
            if (mask < 0)
            {
                motID = 0x80f;
                motMODE = 1;
            }
            else
            {
                motID = 0x503;
                motMODE = 1;
            }
        }
        break;
    }

    case 4:
        dtR->vy = dtR->vy + Me_MOTION_C->turn;
        one = 1;
        if (dtM->count == one)
            Sound(Me_MOTION_C, 0x10);
        if ((dtPAD & 0x2000) == 0)
        {
            motID = 0x501;
            motMODE = one;
        }
        break;

    case 5:
        dtR->vy = dtR->vy - Me_MOTION_C->turn;
        one = 1;
        if (dtM->count == one)
            Sound(Me_MOTION_C, 0x10);
        if ((dtPAD & 0x8000) == 0)
        {
            motID = 0x501;
            motMODE = one;
        }
        break;

    case 0:
    {
        SVECTOR *velocity;
        MotionManager *motion;
        register int value;
        short count;

        velocity = dtV;
        value = velocity->vx;
        if (value != 0)
        {
            if (value > 0)
                value -= 4;
            else
                value += 4;
            velocity->vx = value;
        }
        velocity = dtV;
        value = velocity->vz;
        if (value != 0)
        {
            if (value > 0)
                value -= 4;
            else
                value += 4;
            velocity->vz = value;
        }
        motion = dtM;
        count = motion->count - 1;
        motion->count = count;
        if (count < motion->loop)
        {
            switch (dtPAD & 0x4000)
            {
            default:
                motID = 0x602;
                motMODE = 1;
                break;
            case 0:
                if (Me_MOTION_C == StagePlayer)
                    SetCameraMode(CMODE_NORMAL);
                if (Me_MOTION_C->attribute & 0x40)
                {
                    motID = 0x501;
                    motMODE = 1;
                }
                else
                {
                    motID = 0;
                    motMODE = 1;
                }
                break;
            }
        }
        if ((GameClock & 3) != 0)
            return;
        FUN_80033bc0(dtL, 300, 10, 5);
        return;
    }

    case 3:
        if (dtM->count != 0)
            return;
        if (dtM->loop == 0)
            return;
        motID = 0x80f;
        motMODE = 1;
        return;

    case 2:
        if (dtM->count != 0)
            return;
        if (dtM->loop == 0)
            return;
        motID = 0x501;
        motMODE = 1;
        return;
    }

    if ((Me_MOTION_C->attribute & 0x40) == 0)
    {
        motID = 0;
        motMODE = 1;
        return;
    }
    else if (dtCMD != 0)
    {
        switch ((short)(dtCMD - 1))
        {
        case 0:
            motID = 0x607;
            motMODE = 1;
            return;
        case 0x20:
            motID = 0x70d;
            motMODE = 1;
            return;
        case 1:
            motID = 0x604;
            motMODE = 1;
            return;
        case 3:
            motID = 0x605;
            motMODE = 1;
            return;
        case 2:
            motID = 0x606;
            motMODE = 1;
            return;
        default:
            return;
        }
    }
    else
    {
        mask = Me_MOTION_C->pad.trig;
        if (mask & 0x40)
        {
            JumpControl();
            return;
        }
        if (mask & 0x10)
        {
            switch ((short)(SelectedItem + 1))
            {
            case 2:
                motID = 0xe00;
                motMODE = 1;
                return;
            case 1:
                motID = 0x400;
                motMODE = 1;
                return;
            case 3:
                motID = 0xf00;
                motMODE = 1;
                return;
            case 6:
                motID = 0xf02;
                motMODE = 1;
                return;
            case 5:
                motID = 0xf02;
                motMODE = 1;
                return;
            case 7:
                motID = 0xf03;
                motMODE = 1;
                return;
            case 0:
            case 0xb:
                SoundEx(Me_MOTION_C->locate, 0xc);
                return;
            default:
                ReqItemDefault(Me_MOTION_C,
                               (short)SelectedItem);
                return;
            }
        }
        else
        {
            if (dtPAD & 0x20)
            {
                if (mask & 0x80)
                {
                    motID = 0x70c;
                    motMODE = 1;
                    return;
                }
                motID = 0xb00;
                motMODE = 1;
                return;
            }
            else
            {
                if (mask & 0x80)
                {
                    AttackControl();
                    return;
                }
                if (dtPAD & 0x1000)
                {
                    motID = 0x600;
                    motMODE = 1;
                    return;
                }
                if ((dtPAD & 0x4000) == 0)
                    return;
                motID = 0x602;
                motMODE = 1;
            }
        }
    }
}

/* Ghidra reference: */
// 
// void ActENGAGE(void)
// 
// {
//   short sVar1;
//   Humanoid *human;
//   MotionManager *pMVar2;
//   short sVar3;
//   ushort uVar4;
//   int iVar5;
//   
//   pMVar2 = dtM;
//   human = Me_MOTION_C;
//   switch((int)(((ushort)dtM->mid - 0x500) * 0x10000) >> 0x10) {
//   case 0:
//     sVar3 = dtV->vx;
//     if (sVar3 != 0) {
//       if (sVar3 < 1) {
//         sVar1 = 4;
//       }
//       else {
//         sVar1 = -4;
//       }
//       dtV->vx = sVar3 + sVar1;
//     }
//     sVar3 = dtV->vz;
//     if (sVar3 != 0) {
//       if (sVar3 < 1) {
//         sVar1 = 4;
//       }
//       else {
//         sVar1 = -4;
//       }
//       dtV->vz = sVar3 + sVar1;
//     }
//     pMVar2 = dtM;
//     sVar3 = dtM->count + -1;
//     dtM->count = sVar3;
//     if (sVar3 < pMVar2->loop) {
//       if ((dtPAD & 0x4000) == 0) {
//         if (Me_MOTION_C == StagePlayer) {
//           SetCameraMode(CMODE_NORMAL);
//         }
//         if ((Me_MOTION_C->attribute & 0x40U) == 0) {
//           motID = 0;
//         }
//         else {
//           motID = 0x501;
//         }
//       }
//       else {
//         motID = 0x602;
//       }
//       DAT_80097f0e = 1;
//     }
//     if ((GameClock & 3U) != 0) {
//       return;
//     }
//     FUN_80033bc0(dtL,300,10,5);
//     return;
//   case 1:
//     sVar3 = 0x504;
//     if (((dtPAD & 0x2000) == 0) && (sVar3 = 0x505, (dtPAD & 0x8000) == 0)) {
//       if (dtCMD == 0x22) {
//         sVar3 = 0x712;
// LAB_8002137c:
//         DAT_80097f0e = 1;
//         motID = sVar3;
//       }
//       else if (dtCMD == 0x31) {
//         motID = 0x907;
//         DAT_80097f0e = 0;
//         MoveHumanoid(Me_MOTION_C,0x78,0);
//       }
//       else if (dtM->count == 0) {
//         iVar5 = rand();
//         sVar3 = 0x713;
//         if (iVar5 == (iVar5 / 0x14) * 0x14) goto LAB_8002137c;
//       }
//     }
//     else {
//       DAT_80097f0e = 0;
//       motID = sVar3;
//     }
//     if ((ActionHalt == -1) && (dtM->count == 0)) {
//       uVar4 = GetMotionID(dtM,0x503);
//       motID = 0x503;
//       if ((int)((uint)uVar4 << 0x10) < 0) {
//         motID = 0x80f;
//       }
//       DAT_80097f0e = 1;
//     }
//     break;
//   case 2:
//     if (dtM->count != 0) {
//       return;
//     }
//     uVar4 = dtM->loop;
//     sVar3 = 0x501;
//     goto joined_r0x800215fc;
//   case 3:
//     if (dtM->count != 0) {
//       return;
//     }
//     uVar4 = dtM->loop;
//     sVar3 = 0x80f;
//     goto joined_r0x800215fc;
//   case 4:
//     dtR->vy = dtR->vy + Me_MOTION_C->turn;
//     if (pMVar2->count == 1) {
//       Sound(human,0x10);
//     }
//     uVar4 = dtPAD & 0x2000;
//     goto LAB_80021460;
//   case 5:
//     dtR->vy = dtR->vy - Me_MOTION_C->turn;
//     if (pMVar2->count == 1) {
//       Sound(human,0x10);
//     }
//     uVar4 = dtPAD & 0x8000;
// LAB_80021460:
//     if (uVar4 == 0) {
//       motID = 0x501;
//       DAT_80097f0e = 1;
//     }
//   }
//   if ((Me_MOTION_C->attribute & 0x40U) == 0) {
//     motID = 0;
//     sVar3 = motID;
//   }
//   else if (dtCMD == 0) {
//     uVar4 = (Me_MOTION_C->pad).trig;
//     if ((uVar4 & 0x40) != 0) {
//       JumpControl();
//       return;
//     }
//     if ((uVar4 & 0x10) == 0) {
//       if ((dtPAD & 0x20) == 0) {
//         if ((uVar4 & 0x80) != 0) {
//           AttackControl();
//           return;
//         }
//         sVar3 = 0x600;
//         if ((dtPAD & 0x1000) == 0) {
//           uVar4 = dtPAD & 0x4000;
//           sVar3 = 0x602;
// joined_r0x800215fc:
//           if (uVar4 == 0) {
//             return;
//           }
//         }
//       }
//       else {
//         sVar3 = 0x70c;
//         if ((uVar4 & 0x80) == 0) {
//           sVar3 = 0xb00;
//         }
//       }
//     }
//     else {
//       switch((int)((DAT_80097b1e + 1) * 0x10000) >> 0x10) {
//       case 0:
//       case 0xb:
//         SoundEx(Me_MOTION_C->locate,0xc);
//         return;
//       case 1:
//         sVar3 = 0x400;
//         break;
//       case 2:
//         sVar3 = 0xe00;
//         break;
//       case 3:
//         sVar3 = 0xf00;
//         break;
//       default:
//         ReqItemDefault(Me_MOTION_C,(int)(short)DAT_80097b1e);
//         return;
//       case 5:
//         sVar3 = 0xf02;
//         break;
//       case 6:
//         sVar3 = 0xf02;
//         break;
//       case 7:
//         sVar3 = 0xf03;
//       }
//     }
//   }
//   else {
//     switch((int)((dtCMD - 1) * 0x10000) >> 0x10) {
//     case 0:
//       sVar3 = 0x607;
//       break;
//     case 1:
//       sVar3 = 0x604;
//       break;
//     case 2:
//       sVar3 = 0x606;
//       break;
//     case 3:
//       sVar3 = 0x605;
//       break;
//     default:
//       goto switchD_8002166c_caseD_4;
//     case 0x20:
//       sVar3 = 0x70d;
//     }
//   }
//   motID = sVar3;
//   DAT_80097f0e = 1;
// switchD_8002166c_caseD_4:
//   return;
// }
