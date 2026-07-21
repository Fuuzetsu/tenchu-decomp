#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void ActCHASE(void);
 *     MOTION.C:1241, 61 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
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
 *     reg   $s1       short turn
 *     reg   $v1       short i
 *     reg   $s0       long y
 *
 * Globals it touches, as the original declared them:
 *     extern struct MotionManager *dtM;
 *     extern short dtPAD;
 *     extern short MotionUpdateMode;
 *     extern short motID;
 *     extern short motMODE;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct VECTOR *dtL;
 *     extern struct SVECTOR *dtR;
 *     extern short dtCMD;
 *     extern short SelectedItem;
 * END PSX.SYM */

/*
 * ActCHASE (0x800217dc) — updates chase movement and dispatches chase-state
 * jump, attack, and selected-item actions.
 *
 * STATUS: MATCHED — exact 1416 bytes / 354 instructions.
 */

extern MotionManager *dtM;
extern Humanoid *Me_MOTION_C;
extern s16 dtPAD;
extern s16 MotionUpdateMode;
extern s16 motID;
extern s16 motMODE;
extern VECTOR *dtL;
extern SVECTOR *dtR;
extern s16 dtCMD;
extern s16 SelectedItem;
extern HumanAnimType CVAhuman[5];

extern s16 SetNowMotion(Humanoid *human, s16 mid, s16 move);
extern void MoveHumanoid(Humanoid *human, s16 order, s16 side);
extern s16 HangCheck(void);
extern void JumpControl(void);
extern void AttackControl(void);
extern void FUN_80033bc0(VECTOR *pos, u16 spread, s16 divisor, s16 count);
extern void Sound(Humanoid *human, s16 id);
extern void ReqItemDefault(Humanoid *human, s32 kind);

void ActCHASE(void)
{
    short turn;

    turn = Me_MOTION_C->turn / 2;
    switch ((short)(dtM->mid - 0x600))
    {
    case 0:
    {
        if (dtM->count == 0 || dtM->count == dtM->motion->time / 2)
        {
            short sound;

            sound = 0x12;
            if (Me_MOTION_C->map.attrib & 8)
            {
                sound = 0x14;
            }
            Sound(Me_MOTION_C, sound);
        }

        if (dtPAD & 0x1000)
        {
            if (Me_MOTION_C->attribute & 0x1000)
            {
                short i;

                motID = 0x801;
                motMODE = 0;
                i = MotionUpdateMode;
                if (i != 0)
                {
                    i = 0;
                    do
                    {
                        if (CVAhuman[i].human == Me_MOTION_C)
                        {
                            goto motion_ready;
                        }
                        i++;
                    } while (i < 5);
                }
                SetNowMotion(Me_MOTION_C, motID, motMODE);
                motMODE = -1;
            motion_ready:
                MoveHumanoid(Me_MOTION_C, 0x23, 0);
                if (dtM->mode & 1)
                {
                    dtM->mode &= 0xfffe;
                    dtM->count = 0xd;
                }
                else
                {
                    dtM->mode |= 1;
                }
                goto common_action;
            }

            if (Me_MOTION_C->attribute & 0x400)
            {
                long y;
                long height;

                y = dtL->vy;
                height = Me_MOTION_C->map.height;
                dtL->vy = y - 400;
                Me_MOTION_C->map.height = 1;
                if (HangCheck() == 0)
                {
                    dtL->vy = y;
                    Me_MOTION_C->map.height = height;
                }
                goto common_action;
            }

            if (dtPAD & 0xa000)
            {
                int current;
                int result;
                MotionDataType *motion;
                SVECTOR *rotation;

                rotation = dtR;
                current = rotation->vy;
                if (dtPAD & 0x2000)
                {
                    result = current + turn;
                }
                else
                {
                    result = current - turn;
                }
                rotation->vy = result;
                motion = Me_MOTION_C->motion->motion;
                MoveHumanoid(Me_MOTION_C, motion->orderspd, motion->sidespd);
                goto common_action;
            }

            if ((dtPAD & 0x20) == 0)
            {
                goto common_action;
            }
            motID = 0xb00;
        }
        else
        {
            motID = 0x501;
        }
        motMODE = 1;
        goto common_action;
    }

    case 2:
    {
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x11);
        }

        if ((dtPAD & 0x4000) == 0)
        {
            motID = 0x501;
            motMODE = 1;
        }
        else if (dtCMD == 0x22)
        {
            motID = 0x712;
            motMODE = 1;
        }
        else if (dtPAD & 0xa000)
        {
            int current;
            int result;
            MotionDataType *motion;
            SVECTOR *rotation;

            rotation = dtR;
            current = rotation->vy;
            if (dtPAD & 0x2000)
            {
                result = current + turn * 4;
            }
            else
            {
                result = current - turn * 4;
            }
            rotation->vy = result;
            motion = Me_MOTION_C->motion->motion;
            MoveHumanoid(Me_MOTION_C, motion->orderspd, motion->sidespd);
        }

        if ((dtPAD & 0x20) == 0)
        {
            goto common_action;
        }
        if (Me_MOTION_C->pad.trig & 0x80)
        {
            motID = 0x70c;
            motMODE = 1;
            return;
        }
        motID = 0xb00;
        motMODE = 1;
        goto common_action;
    }

    case 7:
        if (Me_MOTION_C->pad.trig & 0x80)
        {
            AttackControl();
        }
        else if (Me_MOTION_C->pad.trig & 0x40)
        {
            JumpControl();
        }
        /* fall through */
    case 4:
    case 5:
    case 6:
    {
        if (dtM->count == 1)
        {
            Sound(Me_MOTION_C, 0x13);
        }
        if (dtM->count < 7)
        {
            FUN_80033bc0(dtL, 0x96, 0xc, 1);
        }
        if (dtM->count == 0 && dtM->loop != 0)
        {
            motID = 0x501;
            motMODE = 1;
        }
        return;
    }

    default:
        break;
    }

common_action:
    if (dtCMD == 0x31)
    {
        motID = 0x907;
        motMODE = 0;
        MoveHumanoid(Me_MOTION_C, 0x78, 0);
        return;
    }

    if (Me_MOTION_C->pad.trig & 0x40)
    {
        JumpControl();
        return;
    }
    if (Me_MOTION_C->pad.trig & 0x10)
    {
        switch ((short)(SelectedItem + 1))
        {
        case 2:
            motID = 0xe00;
            break;
        case 1:
            motID = 0x400;
            break;
        case 3:
            motID = 0xf00;
            break;
        case 6:
            motID = 0xf02;
            break;
        case 5:
            motID = 0xf02;
            break;
        case 7:
            motID = 0xf03;
            break;
        case 0:
        case 11:
            goto item_sound;
        default:
            goto item_default;
        }
        motMODE = 1;
        return;

    item_sound:
        SoundEx(Me_MOTION_C->locate, 0xc);
        return;

    item_default:
        ReqItemDefault(Me_MOTION_C, SelectedItem);
        return;
    }
    if (Me_MOTION_C->pad.trig & 0x80)
    {
        AttackControl();
    }
}

// 
// void ActCHASE(void)
// 
// {
//   ushort uVar1;
//   Humanoid *pHVar2;
//   MotionManager *pMVar3;
//   short sVar4;
//   int iVar5;
//   int iVar6;
//   MotionDataType *pMVar7;
//   uint uVar8;
//   long lVar9;
//   
//   iVar5 = (uint)(ushort)Me_MOTION_C->turn << 0x10;
//   uVar8 = (uint)((iVar5 >> 0x10) - (iVar5 >> 0x1f)) >> 1;
//   switch((int)(((ushort)dtM->mid - 0x600) * 0x10000) >> 0x10) {
//   case 0:
//     if ((dtM->count == 0) ||
//        (iVar5 = (uint)(ushort)dtM->motion->time << 0x10,
//        (int)dtM->count == (iVar5 >> 0x10) - (iVar5 >> 0x1f) >> 1)) {
//       sVar4 = 0x12;
//       if (((Me_MOTION_C->map).attrib & 8U) != 0) {
//         sVar4 = 0x14;
//       }
//       Sound(Me_MOTION_C,sVar4);
//     }
//     pHVar2 = Me_MOTION_C;
//     sVar4 = 0x501;
//     if ((dtPAD & 0x1000) != 0) {
//       if ((Me_MOTION_C->attribute & 0x1000U) != 0) {
//         motID = 0x801;
//         DAT_80097f0e = 0;
//         iVar5 = 0;
//         if (MotionUpdateMode != 0) {
//           iVar6 = 0;
//           do {
//             iVar5 = iVar5 + 1;
//             if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar6 >> 0xd)) == Me_MOTION_C)
//             goto LAB_80021950;
//             iVar6 = iVar5 * 0x10000;
//           } while (iVar5 * 0x10000 >> 0x10 < 5);
//         }
//         SetNowMotion(Me_MOTION_C,0x801,0);
//         DAT_80097f0e = 0xffff;
// LAB_80021950:
//         MoveHumanoid(Me_MOTION_C,0x23,0);
//         pMVar3 = dtM;
//         uVar1 = dtM->mode;
//         if ((uVar1 & 1) == 0) {
//           dtM->mode = uVar1 | 1;
//         }
//         else {
//           dtM->mode = uVar1 & 0xfffe;
//           pMVar3->count = 0xd;
//         }
//         goto switchD_8002183c_caseD_1;
//       }
//       if ((Me_MOTION_C->attribute & 0x400U) != 0) {
//         iVar5 = dtL->vy;
//         lVar9 = (Me_MOTION_C->map).height;
//         dtL->vy = iVar5 + -400;
//         (pHVar2->map).height = 1;
//         sVar4 = HangCheck();
//         pHVar2 = Me_MOTION_C;
//         if (sVar4 == 0) {
//           dtL->vy = iVar5;
//           (pHVar2->map).height = lVar9;
//         }
//         goto switchD_8002183c_caseD_1;
//       }
//       if ((dtPAD & 0xa000) != 0) {
//         sVar4 = (short)uVar8;
//         if ((dtPAD & 0x2000) == 0) {
//           sVar4 = -sVar4;
//         }
//         dtR->vy = dtR->vy + sVar4;
//         pMVar7 = pHVar2->motion->motion;
//         MoveHumanoid(pHVar2,(ushort)pMVar7->orderspd,(ushort)pMVar7->sidespd);
//         goto switchD_8002183c_caseD_1;
//       }
//       sVar4 = 0xb00;
//       if ((dtPAD & 0x20) == 0) goto switchD_8002183c_caseD_1;
//     }
//     break;
//   default:
//     goto switchD_8002183c_caseD_1;
//   case 2:
//     if (dtM->count == 1) {
//       Sound(Me_MOTION_C,0x11);
//     }
//     pHVar2 = Me_MOTION_C;
//     if ((dtPAD & 0x4000) == 0) {
//       motID = 0x501;
// LAB_80021ab0:
//       DAT_80097f0e = 1;
//     }
//     else {
//       if (dtCMD == 0x22) {
//         motID = 0x712;
//         goto LAB_80021ab0;
//       }
//       if ((dtPAD & 0xa000) != 0) {
//         sVar4 = (short)((int)(uVar8 << 0x10) >> 0xe);
//         if ((dtPAD & 0x2000) == 0) {
//           sVar4 = -sVar4;
//         }
//         dtR->vy = dtR->vy + sVar4;
//         pMVar7 = pHVar2->motion->motion;
//         MoveHumanoid(pHVar2,(ushort)pMVar7->orderspd,(ushort)pMVar7->sidespd);
//       }
//     }
//     if ((dtPAD & 0x20) == 0) goto switchD_8002183c_caseD_1;
//     if (((Me_MOTION_C->pad).trig & 0x80) != 0) {
//       motID = 0x70c;
//       DAT_80097f0e = 1;
//       return;
//     }
//     sVar4 = 0xb00;
//     break;
//   case 7:
//     uVar1 = (Me_MOTION_C->pad).trig;
//     if ((uVar1 & 0x80) == 0) {
//       if ((uVar1 & 0x40) != 0) {
//         JumpControl();
//       }
//     }
//     else {
//       AttackControl();
//     }
//   case 4:
//   case 5:
//   case 6:
//     if (dtM->count == 1) {
//       Sound(Me_MOTION_C,0x13);
//     }
//     if (dtM->count < 7) {
//       FUN_80033bc0(dtL,0x96,0xc,1);
//     }
//     if (dtM->count == 0) {
//       if (dtM->loop != 0) {
//         motID = 0x501;
//         DAT_80097f0e = 1;
//         return;
//       }
//       return;
//     }
//     return;
//   }
//   DAT_80097f0e = 1;
//   motID = sVar4;
// switchD_8002183c_caseD_1:
//   if (dtCMD == 0x31) {
//     motID = 0x907;
//     DAT_80097f0e = 0;
//     MoveHumanoid(Me_MOTION_C,0x78,0);
//     return;
//   }
//   uVar1 = (Me_MOTION_C->pad).trig;
//   if ((uVar1 & 0x40) != 0) {
//     JumpControl();
//     return;
//   }
//   if ((uVar1 & 0x10) != 0) {
//     switch((int)((DAT_80097b1e + 1) * 0x10000) >> 0x10) {
//     case 0:
//     case 0xb:
//       SoundEx(Me_MOTION_C->locate,0xc);
//       return;
//     case 1:
//       motID = 0x400;
//       break;
//     case 2:
//       motID = 0xe00;
//       break;
//     case 3:
//       motID = 0xf00;
//       break;
//     default:
//       ReqItemDefault(Me_MOTION_C,(int)(short)DAT_80097b1e);
//       return;
//     case 5:
//       motID = 0xf02;
//       break;
//     case 6:
//       motID = 0xf02;
//       break;
//     case 7:
//       motID = 0xf03;
//     }
//     DAT_80097f0e = 1;
//     return;
//   }
//   if ((uVar1 & 0x80) != 0) {
//     AttackControl();
//     return;
//   }
//   return;
// }
