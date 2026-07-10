#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void DamageControl(void);
 *     MOTION.C:408, 236 src lines, frame 72 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     stack sp+16     struct VECTOR p
 *     reg   $s1       struct VECTOR * pp
 *     reg   $s2       short i
 *     reg   $s0       short id
 *     reg   $s4       short deg
 *     reg   $s1       short dmg
 *     reg   $s5       short did
 *     reg   $s3       struct Humanoid * enemy
 *     reg   $v1       short i
 *     stack sp+32     struct SVECTOR pv
 *     reg   $v1       short i
 *     reg   $v1       short i
 *
 * Globals it touches, as the original declared them:
 *     extern short MotionUpdateMode;
 *     extern short motID;
 *     extern struct Humanoid *StagePlayer;
 *     extern struct ConflictObjectType ConflictObject[64];
 *     extern struct BattleType BattleDB[78];
 *     extern struct VECTOR *dtL;
 *     extern short Criticals;
 *     extern short Murders;
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern short ActionHalt;
 *     extern struct MotionManager *dtM;
 *     extern struct SVECTOR ConflictDistance;
 * END PSX.SYM */

/*
 * DamageControl (0x8001d6bc) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=DamageControl
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/DamageControl", DamageControl);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/DamageControl", switchD_8001db0c__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/DamageControl", switchD_8001db0c__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/DamageControl", switchD_8001db0c__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/DamageControl", switchD_8001db0c__caseD_e);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/DamageControl", switchD_8001db0c__caseD_13);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/DamageControl", switchD_8001db0c__caseD_14);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/DamageControl", switchD_8001db0c__caseD_15);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/DamageControl", switchD_8001db0c__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/DamageControl", switchD_8001db0c__caseD_2);

/* jump-table pool @ 0x80011278 (23 words; tables at 0x80011278) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 DamageControl_jtbl[23] = {
    0x8001DB20, 0x8001DB14, 0x8001DC58, 0x8001DC1C,
    0x8001DC58, 0x8001DC1C, 0x8001DC58, 0x8001DC58,
    0x8001DC58, 0x8001DC58, 0x8001DC58, 0x8001DC58,
    0x8001DC58, 0x8001DC58, 0x8001DB60, 0x8001DC58,
    0x8001DC58, 0x8001DC58, 0x8001DC58, 0x8001DB70,
    0x8001DB80, 0x8001DBF0, 0x8001DC1C,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
// 
// void DamageControl(void)
// 
// {
//   SVECTOR *pSVar1;
//   MotionManager *pMVar2;
//   ushort uVar3;
//   short sVar4;
//   ushort uVar5;
//   short sVar6;
//   int iVar7;
//   TItemType TVar8;
//   VECTOR *pVVar9;
//   uint uVar10;
//   int iVar11;
//   short sVar12;
//   int iVar13;
//   Humanoid *pHVar14;
//   short sVar15;
//   uint uVar16;
//   Humanoid *pHVar17;
//   SVECTOR local_40;
//   VECTOR local_38;
//   SVECTOR local_28;
//   
//   sVar15 = (Me_MOTION_C->vector).pad;
//   uVar16 = 0;
//   if (Me_MOTION_C->life < 1) {
//     return;
//   }
//   if (MotionUpdateMode != 0) {
//     return;
//   }
//   if (motID == 0x301) {
//     return;
//   }
//   if (Me_MOTION_C == StagePlayer) {
//     SetCameraMode(CMODE_NORMAL);
//   }
//   if ((Me_MOTION_C->type & 0xf0U) == 0xa0) {
//     pHVar17 = (Humanoid *)ConflictObject[sVar15].common;
//     if (pHVar17 == (Humanoid *)&DAT_00000001) {
//       Me_MOTION_C->life = 0;
//     }
//     else {
//       Sound(pHVar17,4);
//       DeleteConflict(ConflictObject[sVar15].model);
//       uVar3 = GetAttackDBID(pHVar17,pHVar17->motion->mid);
//       pHVar14 = Me_MOTION_C;
//       iVar11 = (uint)(ushort)Me_MOTION_C->life -
//                (uint)*(ushort *)((int)&BattleDB[0].power + ((int)((uint)uVar3 << 0x10) >> 0xc));
//       Me_MOTION_C->life = (short)iVar11;
//       if ((iVar11 * 0x10000 < 0) || ((pHVar14->attribute & 0x40U) == 0)) {
//         pHVar14->life = 0;
//       }
//       local_38.vx = dtL->vx;
//       iVar11 = (uint)(ushort)Me_MOTION_C->height << 0x10;
//       local_38.vy = dtL->vy - ((iVar11 >> 0x10) - (iVar11 >> 0x1f) >> 1);
//       local_38.vz = dtL->vz;
//       SetImpact(&local_38,0x6000,2);
//       if (StagePlayer == pHVar17) {
//         PadShockAR(0,0xff,10,10);
//       }
//     }
//     ReqLifeBar(Me_MOTION_C);
//     if (Me_MOTION_C->life == 0) {
//       motID = 0x1100;
//       DAT_80097f0e = 1;
//       if ((Me_MOTION_C->type != 0xa9) &&
//          ((StagePlayer == pHVar17 || (pHVar17 == (Humanoid *)&DAT_00000001)))) {
//         if ((Me_MOTION_C->attribute & 0x42U) == 0) {
//           Criticals = Criticals + 1;
//         }
//         else {
//           Murders = Murders + 1;
//         }
//       }
//       sVar15 = 8;
//       if ((Me_MOTION_C->attribute & 0x42U) == 0) goto LAB_8001d954;
//     }
//     else {
//       motID = 0x1000;
//       sVar15 = 6;
//     }
//     DAT_80097f0e = 1;
//     Sound(Me_MOTION_C,sVar15);
//     FUN_8002f7f4();
// LAB_8001d954:
//     iVar11 = 0;
//     if (MotionUpdateMode != 0) {
//       iVar7 = 0;
//       do {
//         iVar11 = iVar11 + 1;
//         if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar7 >> 0xd)) == Me_MOTION_C)
//         goto LAB_8001d9c0;
//         iVar7 = iVar11 * 0x10000;
//       } while (iVar11 * 0x10000 >> 0x10 < 5);
//     }
//     SetNowMotion(Me_MOTION_C,motID,DAT_80097f0e);
//     DAT_80097f0e = -1;
// LAB_8001d9c0:
//     AttackCancelControl(3);
//     return;
//   }
//   if (0x713 < motID) {
//     if (motID < 0x71a) {
//       ActionHalt = 0;
//       if (Me_MOTION_C == StagePlayer) {
//         SetCameraMode(CMODE_NORMAL);
//       }
//       if ((Me_MOTION_C->attribute & 0x40U) == 0) {
//         motID = 0;
//       }
//       else {
//         motID = 0x501;
//       }
//       DAT_80097f0e = 1;
//       dtL->vy = dtL->vy + -1;
//       return;
//     }
//     if (motID == 0x1009) {
//       return;
//     }
//   }
//   dtM->mask = 0x7fff;
//   AttackCancelControl(3);
//   pHVar17 = (Humanoid *)ConflictObject[sVar15].common;
//   if (pHVar17 != (Humanoid *)&DAT_00000001) {
//     if (((Me_MOTION_C->type & 0xf0U) == 0x80) && (pHVar17 != StagePlayer)) {
//       return;
//     }
//     sVar4 = 1;
//     iVar11 = (uint)(ushort)pHVar17->locate->vx - (uint)(ushort)dtL->vx;
//     local_40.vx = (short)iVar11;
//     iVar11 = iVar11 * 0x10000 >> 0x10;
//     iVar7 = (uint)(ushort)pHVar17->locate->vy - (uint)(ushort)dtL->vy;
//     local_40.vy = (short)iVar7;
//     if (iVar11 < 0) {
//       iVar11 = -iVar11;
//     }
//     iVar13 = (uint)(ushort)pHVar17->locate->vz - (uint)(ushort)dtL->vz;
//     local_40.vz = (short)iVar13;
//     if (100 < iVar11) goto LAB_8001dfb0;
//     iVar11 = iVar7 * 0x10000 >> 0x10;
//     if (iVar11 < 0) {
//       iVar11 = -iVar11;
//     }
//     if (100 < iVar11) goto LAB_8001dfb0;
//     iVar11 = iVar13 * 0x10000 >> 0x10;
//     sVar4 = 1;
//     if (iVar11 < 0) {
//       iVar11 = -iVar11;
//     }
//     while (100 < iVar11) {
// LAB_8001dfb0:
//       do {
//         do {
//           sVar4 = sVar4 << 1;
//           iVar7 = (int)((uint)(ushort)local_40.vx << 0x10) >> 0x11;
//           iVar13 = (int)((uint)(ushort)local_40.vy << 0x10) >> 0x11;
//           iVar11 = (int)((uint)(ushort)local_40.vz << 0x10) >> 0x11;
//           local_40.vx = local_40.vx >> 1;
//           if (iVar7 < 0) {
//             iVar7 = -iVar7;
//           }
//           local_40.vy = local_40.vy >> 1;
//           local_40.vz = local_40.vz >> 1;
//         } while (100 < iVar7);
//         if (iVar13 < 0) {
//           iVar13 = -iVar13;
//         }
//       } while (100 < iVar13);
//       if (iVar11 < 0) {
//         iVar11 = -iVar11;
//       }
//     }
//     local_38.vx = dtL->vx;
//     local_38.vz = dtL->vz;
//     local_38.pad = dtL->pad;
//     local_38.vy = dtL->vy + -1000;
//     pVVar9 = GetAreaMapPassage(GlobalAreaMap,&local_38,&local_40,sVar4);
//     if (pVVar9 != (VECTOR *)0x0) {
//       return;
//     }
//     iVar11 = pHVar17->locate->vx;
//     uVar3 = GetDirection(iVar11 - dtL->vx,pHVar17->locate->vz - dtL->vz,dtR->vy);
//     sVar4 = (short)iVar11;
//     uVar5 = GetAttackDBID(pHVar17,pHVar17->motion->mid);
//     if (Me_MOTION_C == StagePlayer) {
// LAB_8001e1a0:
//       iVar11 = (uint)uVar3 << 0x10;
//     }
//     else {
//       iVar11 = (uint)uVar3 << 0x10;
//       if ((((Me_MOTION_C->status != 7) &&
//            (iVar11 = (uint)uVar3 << 0x10, (Me_MOTION_C->attribute & 0x40U) != 0)) &&
//           (iVar11 = (uint)uVar3 << 0x10, (Me_MOTION_C->map).height == 0)) &&
//          (iVar11 = (uint)uVar3 << 0x10, PersistentState._88_1_ != '\0')) {
//         iVar11 = rand();
//         iVar7 = EngageLevel + 1;
//         if (iVar7 == 0) {
//           trap(0x1c00);
//         }
//         if ((iVar7 == -1) && (iVar11 == -0x80000000)) {
//           trap(0x1800);
//         }
//         if (iVar11 % iVar7 == 0) {
//           if ((Me_MOTION_C->type != 0x87) &&
//              (iVar11 = (uint)uVar3 << 0x10, Me_MOTION_C->type != 0x8a)) goto LAB_8001e1a4;
//           uVar16 = rand();
//           if ((uVar16 & 1) == 0) goto LAB_8001e1a0;
//         }
//         motID = 0x602;
//         goto LAB_8001e1a0;
//       }
//     }
// LAB_8001e1a4:
//     iVar11 = iVar11 >> 0x10;
//     if (iVar11 < 0) {
//       iVar11 = -iVar11;
//     }
//     if (iVar11 < 700) {
//       if (motID != 0x602) {
//         if (motID == 0x500) {
//           return;
//         }
//         goto LAB_8001e1d8;
//       }
// LAB_8001e1e8:
//       if (motID == 0x500) {
//         return;
//       }
//       sVar6 = UpdateMotion(dtM,0x500);
//       pVVar9 = dtL;
//       if (sVar6 != 0) {
//         iVar11 = (int)(*Me_MOTION_C->model->object)->id;
//         if (-1 < iVar11) {
//           dtL->vx = ConflictObject[iVar11].position.vx;
//           pVVar9->vz = ConflictObject[iVar11].position.vz;
//         }
//         pMVar2 = dtM;
//         dtR->vy = dtR->vy + uVar3;
//         Me_MOTION_C->status = 5;
//         pMVar2->count = 0;
//         PlayMotion(pMVar2,1);
//         pHVar14 = Me_MOTION_C;
//         sVar4 = *(short *)((int)&BattleDB[0].power + ((int)((uint)uVar5 << 0x10) >> 0xc));
//         dtM->loop = -8 - sVar4;
//         MoveHumanoid(pHVar14,-((short)((sVar4 * 5) / 2) + 0x50),0);
//         pHVar14 = StagePlayer;
//         if (pHVar17->status == 7) {
//           pHVar17->motion->loop =
//                ((sVar4 >> 0xf) - (short)((ulonglong)((longlong)(int)sVar4 * 0x55555556) >> 0x20)) +
//                -1;
//           (pHVar17->vector).vz = 0;
//           (pHVar17->vector).vx = 0;
//           if (pHVar14 == pHVar17) {
//             PadShockAR(0,0x7f,10,0);
//           }
//         }
//         iVar11 = 0;
//         DeleteConflict(ConflictObject[sVar15].model);
//         pVVar9 = GetAbsolutePosition(Me_MOTION_C->model->object[2],0,sVar4 * 10 + 100,0);
//         do {
//           iVar7 = rand();
//           local_28.vx = (short)iVar7 + (short)(iVar7 / 100) * -100 + -0x32;
//           iVar7 = rand();
//           local_28.vy = (short)iVar7 + (short)(iVar7 / 100) * -100 + -0x32;
//           iVar7 = rand();
//           local_28.vz = (short)iVar7 + (short)(iVar7 / 100) * -100 + -0x32;
//           iVar7 = rand();
//           SetBleed(pVVar9,&local_28,iVar7 % 0x14 + 0x14,0xffff00);
//           iVar11 = iVar11 + 1;
//         } while (iVar11 * 0x10000 >> 0x10 < 10);
//         pHVar14 = Me_MOTION_C;
//         if (StagePlayer == Me_MOTION_C) {
//           PadShockAR(0,0x7f,10,0);
//           pHVar14 = pHVar17;
//         }
//         ReqLifeBar(pHVar14);
//         iVar11 = rand();
//         FUN_8003944c(pVVar9,0,0x2000,0x6000,0xdcdcdc,0,(iVar11 % 0x168) * 0x10000 >> 0x10,6,9,1);
//         uVar16 = rand();
//         if ((uVar16 & 1) != 0) {
//           Sound(Me_MOTION_C,10);
//         }
//         if ((pHVar17->type & 0xf0U) != 0xa0) {
//           Sound(Me_MOTION_C,3);
//           return;
//         }
//         return;
//       }
//     }
//     else {
// LAB_8001e1d8:
//       if (motID == 0x100c) goto LAB_8001e1e8;
//     }
//     pVVar9 = dtL;
//     iVar11 = (int)(*Me_MOTION_C->model->object)->id;
//     if (-1 < iVar11) {
//       dtL->vx = ConflictObject[iVar11].position.vx;
//       pVVar9->vz = ConflictObject[iVar11].position.vz;
//     }
//     uVar5 = *(ushort *)((int)&BattleDB[0].power + ((int)((uint)uVar5 << 0x10) >> 0xc));
//     uVar16 = (uint)uVar5;
//     if (pHVar17 == StagePlayer) {
//       if ((Me_MOTION_C->attribute & 0x40U) == 0) {
//         Me_MOTION_C->life = 0;
//         goto LAB_8001e6b0;
//       }
// LAB_8001e6c4:
//       uVar16 = uVar16 - ((byte)PersistentState._88_1_ - 2);
//     }
//     else {
//       if (Me_MOTION_C != StagePlayer) {
//         uVar16 = (int)(short)uVar5 / 3;
//       }
// LAB_8001e6b0:
//       if (pHVar17 == StagePlayer) goto LAB_8001e6c4;
//     }
//     pSVar1 = dtR;
//     if (pHVar17->type == 0xa9) {
//       uVar16 = (short)uVar16 * 6;
//     }
//     if (Me_MOTION_C->itmctl == 0xc) {
//       uVar16 = (int)(short)uVar16 / 3;
//     }
//     if (pHVar17->itmctl == 0xc) {
//       uVar16 = (uVar16 << 0x10) >> 0xf;
//     }
//     iVar11 = uVar16 << 0x10;
//     if ((Me_MOTION_C == StagePlayer) && (DAT_800979a2 != 0)) {
//       uVar16 = ((short)uVar16 * 7) / 10;
//       iVar11 = uVar16 * 0x10000;
//     }
//     iVar7 = iVar11 >> 0x13;
//     if (3 < iVar7) {
//       iVar7 = 3;
//     }
//     if (0 < (Me_MOTION_C->map).height) {
//       iVar7 = 3;
//     }
//     sVar6 = (short)(((iVar11 >> 0x10) * 5) / 2) + 0x50;
//     sVar12 = dtR->vy + uVar3;
//     iVar11 = (int)(short)uVar3;
//     if (iVar11 < 0) {
//       iVar11 = -iVar11;
//     }
//     dtR->vy = sVar12;
//     if (iVar11 < 0x400) {
//       sVar6 = -sVar6;
//     }
//     else {
//       pSVar1->vy = sVar12 + -0x800;
//     }
//     if ((short)iVar7 == 3) {
//       iVar11 = (int)(short)uVar3;
//       if (iVar11 < 0) {
//         iVar11 = -iVar11;
//       }
//       sVar6 = -0x46;
//       if (0x400 < iVar11) {
//         sVar6 = 0x46;
//       }
//     }
//     MoveHumanoid(Me_MOTION_C,sVar6,0);
//     pHVar14 = Me_MOTION_C;
//     iVar11 = (ushort)Me_MOTION_C->life - uVar16;
//     Me_MOTION_C->life = (short)iVar11;
//     if (iVar11 * 0x10000 < 1) {
//       pHVar14->life = 0;
//       DAT_8009770c = pHVar14;
//       if ((short)iVar7 == 3) {
//         iVar11 = (int)(short)uVar3;
//         if (iVar11 < 0) {
//           iVar11 = -iVar11;
//         }
//         if (0x400 < iVar11) {
//           iVar7 = iVar7 + 4;
//         }
//         dtM->mid = -1;
//         motID = *(short *)(((iVar7 << 0x10) >> 0xf) + -0x7ff79494);
//       }
//       else {
//         motID = 0x1100;
//         DAT_80097f0e = 1;
//         iVar11 = 0;
//         if (MotionUpdateMode != 0) {
//           iVar7 = 0;
//           do {
//             iVar11 = iVar11 + 1;
//             if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar7 >> 0xd)) == pHVar14)
//             goto LAB_8001e91c;
//             iVar7 = iVar11 * 0x10000;
//           } while (iVar11 * 0x10000 >> 0x10 < 5);
//         }
//         SetNowMotion(Me_MOTION_C,0x1100,1);
//         DAT_80097f0e = -1;
// LAB_8001e91c:
//         uVar10 = rand();
//         if ((uVar10 & 1) != 0) {
//           motID = 0x1101;
//           DAT_80097f0e = 1;
//         }
//       }
//       if (pHVar17 == StagePlayer) {
//         if ((Me_MOTION_C->type & 0xf0U) == 0x90) {
//           FriendHits = FriendHits + 1;
//         }
//         else if ((Me_MOTION_C->attribute & 0x42U) == 0) {
//           Criticals = Criticals + 1;
//         }
//         else {
//           Murders = Murders + 1;
//         }
//       }
//       if ((Me_MOTION_C->attribute & 0x42U) != 0) {
//         Sound(Me_MOTION_C,8);
//         goto LAB_8001eaa8;
//       }
//     }
//     else {
//       iVar11 = (int)(short)uVar3;
//       if (iVar11 < 0) {
//         iVar11 = -iVar11;
//       }
//       if (0x400 < iVar11) {
//         iVar7 = iVar7 + 4;
//       }
//       dtM->mid = -1;
//       motID = *(short *)(((iVar7 << 0x10) >> 0xf) + -0x7ff79494);
//       DAT_80097f0e = 0;
// LAB_8001eaa8:
//       FUN_8002f7f4();
//     }
//     pHVar14 = StagePlayer;
//     if (pHVar17->status == 7) {
//       pHVar17->motion->loop =
//            (((short)uVar16 >> 0xf) -
//            (short)((ulonglong)((longlong)(int)(short)uVar16 * 0x55555556) >> 0x20)) + -1;
//       (pHVar17->vector).vz = 0;
//       (pHVar17->vector).vx = 0;
//       if (pHVar14 == pHVar17) {
//         sVar4 = 10;
//         PadShockAR(0,0xff,10,10);
//       }
//     }
//     DeleteConflict(ConflictObject[sVar15].model);
//     local_38.vx = dtL->vx;
//     iVar11 = (uint)(ushort)Me_MOTION_C->height << 0x10;
//     local_38.vy = dtL->vy - ((iVar11 >> 0x10) - (iVar11 >> 0x1f) >> 1);
//     local_38.vz = dtL->vz;
//     SetBlood(&local_38,(SVECTOR *)&DAT_00000005,0x78,sVar4);
//     SetImpact(&local_38,0x6000,2);
//     pHVar14 = Me_MOTION_C;
//     if (StagePlayer == Me_MOTION_C) {
//       PadShockAR(0,0x7f,10,0x1e);
//       pHVar14 = pHVar17;
//     }
//     ReqLifeBar(pHVar14);
//     uVar16 = rand();
//     sVar15 = 7;
//     if ((uVar16 & 1) != 0) {
//       sVar15 = 6;
//     }
//     Sound(Me_MOTION_C,sVar15);
//     uVar16 = rand();
//     sVar15 = 4;
//     if (((uVar16 & 1) == 0) && (Me_MOTION_C->life == 0)) {
//       sVar15 = 5;
//     }
//     Sound(pHVar17,sVar15);
//     goto LAB_8001ec30;
//   }
//   if (Me_MOTION_C->status == 0x10) {
//     return;
//   }
//   GetConflictResult((ModelType *)Me_MOTION_C->model,sVar15);
//   TVar8 = GetItemType((int)sVar15);
//   switch((int)((TVar8 - ITEM_SHURIKEN) * 0x10000) >> 0x10) {
//   case 0:
//     uVar16 = 0x14;
//     if ((Me_MOTION_C->type == 0x87) || (Me_MOTION_C->type == 0x8a)) {
//       Me_MOTION_C->item[1] = Me_MOTION_C->item[1] + '\x01';
//       goto switchD_8001db0c_caseD_e;
//     }
//     goto LAB_8001db64;
//   case 1:
//     uVar16 = 3;
//     motID = 0x100a;
//     goto LAB_8001dc08;
//   default:
//     uVar16 = 0;
//     break;
//   case 3:
//   case 5:
//   case 0x16:
//     uVar16 = 0x1e;
//     if ((short)TVar8 == 6) {
//       uVar16 = 0x2d;
//     }
//     if (((Me_MOTION_C->map).attrib & 4U) == 0) {
//       (Me_MOTION_C->map).height = 1;
//     }
//     break;
//   case 0xe:
// switchD_8001db0c_caseD_e:
// LAB_8001db64:
//     if (uVar16 == 0) {
//       uVar16 = 0x1e;
// switchD_8001db0c_caseD_13:
//       if (uVar16 == 0) {
//         uVar16 = 0x14;
// switchD_8001db0c_caseD_14:
//         if (uVar16 == 0) {
//           uVar16 = 10;
//         }
//       }
//     }
//     local_38.vx = dtL->vx;
//     motID = 0x1000;
//     iVar11 = (uint)(ushort)Me_MOTION_C->height << 0x10;
//     local_38.vy = dtL->vy - ((iVar11 >> 0x10) - (iVar11 >> 0x1f) >> 1);
//     local_38.vz = dtL->vz;
//     DAT_80097f0e = 1;
//     SetBlood(&local_38,(SVECTOR *)&DAT_00000005,0x5a,(short)dtL);
//     break;
//   case 0x13:
//     goto switchD_8001db0c_caseD_13;
//   case 0x14:
//     goto switchD_8001db0c_caseD_14;
//   case 0x15:
//     uVar16 = 0x19;
//     uVar10 = rand();
//     motID = 0x1001;
//     if ((uVar10 & 1) == 0) {
//       motID = 0x1003;
//     }
// LAB_8001dc08:
//     DAT_80097f0e = 1;
//   }
//   if (0 < (Me_MOTION_C->map).height) {
//     sVar4 = GetDirection((int)ConflictDistance.vx,(int)ConflictDistance.vz,dtR->vy);
//     pHVar17 = Me_MOTION_C;
//     iVar11 = (int)sVar4;
//     if (iVar11 < 0) {
//       iVar11 = -iVar11;
//     }
//     sVar6 = -0x46;
//     if (iVar11 < 0x400) {
//       motID = 0x1005;
//       DAT_80097f0e = 0;
//       dtR->vy = dtR->vy + sVar4;
//     }
//     else {
//       sVar6 = 0x46;
//       motID = 0x1006;
//       DAT_80097f0e = 0;
//       dtR->vy = sVar4 + dtR->vy + 0x800;
//     }
//     MoveHumanoid(pHVar17,sVar6,0);
//   }
//   pHVar17 = Me_MOTION_C;
//   if ((Me_MOTION_C == StagePlayer) && (DAT_800979a2 != 0)) {
//     uVar16 = (uint)((short)uVar16 * 7) / 10;
//   }
//   iVar11 = (ushort)Me_MOTION_C->life - uVar16;
//   Me_MOTION_C->life = (short)iVar11;
//   if (iVar11 * 0x10000 < 1) {
//     uVar16 = (uint)(ushort)motID;
//     pHVar17->life = 0;
//     if (1 < uVar16 - 0x1005) {
//       motID = 0x1100;
//       DAT_80097f0e = 1;
//     }
//     Sound(pHVar17,8);
//     TVar8 = GetItemType((int)sVar15);
//     if ((TVar8 < ITEM_GUN) || ((ITEM_ARROW < TVar8 && (TVar8 != ITEM_LIGHTNINGBOLT)))) {
//       if ((Me_MOTION_C->type & 0xf0U) == 0x90) {
//         FriendHits = FriendHits + 1;
//       }
//       else if ((Me_MOTION_C->attribute & 0x42U) == 0) {
//         Criticals = Criticals + 1;
//       }
//       else {
//         Murders = Murders + 1;
//       }
//     }
//   }
//   else {
//     uVar16 = rand();
//     sVar15 = 7;
//     if ((uVar16 & 1) != 0) {
//       sVar15 = 6;
//     }
//     Sound(Me_MOTION_C,sVar15);
//   }
//   if (StagePlayer == Me_MOTION_C) {
//     PadShockAR(0,0xff,10,0x14);
//   }
//   else {
//     ReqLifeBar(Me_MOTION_C);
//   }
//   FUN_8002f7f4();
// LAB_8001ec30:
//   if ((Me_MOTION_C->life == 0) && (Me_MOTION_C->item[10] != '\0')) {
//     ReqItemDefault(Me_MOTION_C,ITEM_KAWARIMI);
//     uVar16 = (uint)(ushort)motID;
//     Me_MOTION_C->life = Me_MOTION_C->lifemax;
//     if (1 < uVar16 - 0x1005) {
//       motID = 0x1002;
//       DAT_80097f0e = 1;
//     }
//   }
//   pMVar2 = dtM;
//   pHVar17 = Me_MOTION_C;
//   (Me_MOTION_C->pad).time = 0;
//   pSVar1 = dtV;
//   if ((pMVar2->mid == 0x300) || (pMVar2->mid == 0x302)) {
//     dtV->vz = 0;
//     pSVar1->vx = 0;
//     if (pHVar17->life != 0) {
//       DAT_80097f0e = 0xffff;
//       return;
//     }
//     motID = 0x1108;
//     DAT_80097f0e = 1;
//   }
//   iVar11 = 0;
//   if (MotionUpdateMode != 0) {
//     iVar7 = 0;
//     do {
//       iVar11 = iVar11 + 1;
//       if (*(Humanoid **)((int)&CVAhuman[0].human + (iVar7 >> 0xd)) == Me_MOTION_C) {
//         return;
//       }
//       iVar7 = iVar11 * 0x10000;
//     } while (iVar11 * 0x10000 >> 0x10 < 5);
//   }
//   SetNowMotion(Me_MOTION_C,motID,DAT_80097f0e);
//   DAT_80097f0e = 0xffff;
//   return;
// }

#endif /* NON_MATCHING */
