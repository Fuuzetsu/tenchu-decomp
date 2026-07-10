#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short CVAupdate(void);
 *     CHRANIM.C:145, 89 src lines, frame 56 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     stack sp+24     struct SVECTOR vect
 *     reg   $s1       struct Humanoid * human
 *     reg   $s0       long i
 *
 * Globals it touches, as the original declared them:
 *     extern struct HumanAnimType CVAhuman[5];
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct SVECTOR UnitVector;
 *     extern struct Humanoid *StagePlayer;
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct GsIMAGE Images[52];
 *     extern int StageID;
 * END PSX.SYM */

/*
 * CVAupdate (0x80050628) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=CVAupdate
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", CVAupdate);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_6);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_7);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_8);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAupdate", switchD_80050694__caseD_1);

/* jump-table pool @ 0x80013664 (9 words; tables at 0x80013664) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 CVAupdate_jtbl[9] = {
    0x8005069C, 0x80050E04, 0x800506C4, 0x80050954,
    0x80050B44, 0x80050B5C, 0x80050C40, 0x80050C6C,
    0x80050D14,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// short CVAupdate(void)
// 
// {
//   short *psVar1;
//   undefined4 uVar2;
//   undefined4 uVar3;
//   long lVar4;
//   Humanoid *pHVar5;
//   uchar *telop;
//   short *psVar6;
//   short sVar7;
//   ModelType *pMVar8;
//   VECTOR *pVVar9;
//   HumanAnimType *pHVar10;
//   ModelArchiveType *pMVar11;
//   uint uVar12;
//   int *piVar13;
//   int iVar14;
//   int iVar15;
//   VECTOR local_30;
//   
//   if (*DAT_80097cbc != 1) {
//     do {
//       switch(*DAT_80097cbc) {
//       case 0:
//         if (DAT_80097cbc[5] == -1) {
//           CdaStop();
//         }
//         break;
//       case 2:
//         pHVar5 = GetHumanoid(DAT_80097cbc[1]);
//         iVar15 = 0;
//         if (pHVar5 == (Humanoid *)0x0) {
//           return 0;
//         }
//         pHVar5->attribute = pHVar5->attribute & 0xff7f;
//         pHVar5->motion->mask = 0x7fff;
//         pHVar10 = CVAhuman;
//         do {
//           if (pHVar10->human == (Humanoid *)0x0) break;
//           iVar15 = iVar15 + 1;
//           pHVar10 = pHVar10 + 1;
//         } while (iVar15 < 5);
//         if (iVar15 == 5) {
//           SetNowMotion(pHVar5,0x501,1);
//         }
//         uVar2 = UnitVector._4_4_;
//         sVar7 = UnitVector.vy;
//         (pHVar5->vector).vx = UnitVector.vx;
//         uVar3 = UnitVector._4_4_;
//         (pHVar5->vector).vy = sVar7;
//         UnitVector.vz = (short)uVar2;
//         UnitVector.pad = SUB42(uVar2,2);
//         sVar7 = UnitVector.pad;
//         (pHVar5->vector).vz = UnitVector.vz;
//         UnitVector._4_4_ = uVar3;
//         (pHVar5->vector).pad = sVar7;
//         pMVar8 = *pHVar5->model->object;
//         pMVar8->attribute = pMVar8->attribute | 0x4000;
//         pMVar11 = pHVar5->model;
//         iVar15 = 0;
//         if (0 < pMVar11->n) {
//           do {
//             pMVar11->object[iVar15]->attribute = pMVar11->object[iVar15]->attribute & 0xfffe;
//             iVar15 = iVar15 + 1;
//           } while (iVar15 < pMVar11->n);
//         }
//         if ((StagePlayer != pHVar5) && (pHVar5->life == -1)) {
//           pHVar5->attribute = pHVar5->attribute | 4;
//           pHVar5->life = pHVar5->lifemax;
//         }
//         psVar1 = DAT_80097cbc;
//         if (DAT_80097cbc[5] != -1) {
//           sVar7 = DAT_80097cbc[2];
//           pHVar5->point[0] = sVar7 * 1000;
//           pHVar5->locate->vx = sVar7 * 1000;
//           pVVar9 = pHVar5->locate;
//           iVar15 = psVar1[4] * 1000;
//           pHVar5->point[1] = iVar15;
//           pVVar9->vz = iVar15;
//           iVar14 = psVar1[3] * 1000;
//           lVar4 = GetAreaMapLevel(GlobalAreaMap,pHVar5->locate->vx,iVar14 + -1000);
//           pHVar5->locate->vy = lVar4;
//           iVar15 = pHVar5->locate->vy;
//           if ((iVar14 < iVar15) || (iVar15 == -0x80000000)) {
//             pHVar5->locate->vy = iVar14;
//           }
//           pHVar5->rotate->vy = DAT_80097cbc[5];
//           UpdateCoordinate((ModelType *)pHVar5->model);
//           iVar15 = pHVar5->locate->vy - StagePlayer->locate->vy;
//           if (iVar15 < 0) {
//             iVar15 = -iVar15;
//           }
//           if (20000 < iVar15) {
//             pHVar5->attribute = pHVar5->attribute | 0x80;
//           }
//         }
//         break;
//       case 3:
//         pHVar5 = GetHumanoid(DAT_80097cbc[1]);
//         if (pHVar5 == (Humanoid *)0x0) {
//           return 0;
//         }
//         if ((int)((uint)(ushort)DAT_80097cbc[2] << 0x10) >> 0x10 == -1) {
//           pHVar5->life = -1;
//           pHVar5->attribute = pHVar5->attribute & 0xfffbU | 0x82;
//           pHVar5->motion->mid = -1;
//           SetNowMotion(pHVar5,0,1);
//           PlayMotion(pHVar5->motion,1);
//           pHVar5->motion->count = pHVar5->motion->count + -1;
//         }
//         else {
//           iVar15 = (int)((uint)(ushort)DAT_80097cbc[2] << 0x10) >> 0x18;
//           if ((pHVar5->status == 0x11) && (1 < iVar15 - 0x10U)) {
//             return 0;
//           }
//           if ((0 < pHVar5->life) && (iVar15 == 0x11)) {
//             pHVar5->life = 0;
//             ReqLifeBar(pHVar5);
//           }
//           psVar1 = DAT_80097cbc;
//           iVar15 = 0;
//           pHVar10 = CVAhuman;
//           do {
//             if (pHVar10->human == pHVar5) break;
//             iVar15 = iVar15 + 1;
//             pHVar10 = pHVar10 + 1;
//           } while (iVar15 < 5);
//           if (iVar15 == 5) {
//             iVar15 = 0;
//             pHVar10 = CVAhuman;
//             do {
//               if (pHVar10->human == (Humanoid *)0x0) break;
//               iVar15 = iVar15 + 1;
//               pHVar10 = pHVar10 + 1;
//             } while (iVar15 < 5);
//             if (iVar15 == 5) {
//               return 0;
//             }
//           }
//           pHVar5->motion->mid = -1;
//           SetNowMotion(pHVar5,psVar1[2],1);
//           PlayMotion(pHVar5->motion,1);
//           psVar1 = DAT_80097cbc;
//           pHVar5->motion->count = pHVar5->motion->count + -1;
//           CVAhuman[iVar15].human = pHVar5;
//           sVar7 = psVar1[3];
//           if (psVar1[3] < 1) {
//             sVar7 = 0x7fff;
//           }
//           CVAhuman[iVar15].loop = sVar7;
//           sVar7 = psVar1[4];
//           if (psVar1[4] == 0) {
//             sVar7 = 0x501;
//           }
//           CVAhuman[iVar15].motid = sVar7;
//           if ((pHVar5->type == 0xa7) && (psVar1[2] == 0x1100)) {
//             SoundEx((VECTOR *)0x0,0x41);
//           }
//         }
//         break;
//       case 4:
//         AVCameraSetup();
//         DAT_80097ccc = 1;
//         break;
//       case 5:
//         if (DAT_80097cbc[1] == 0) {
//           ViewInfo.vrx = DAT_80097cbc[2] * 100;
//           ViewInfo.vry = DAT_80097cbc[3] * 100;
//           ViewInfo.vrz = DAT_80097cbc[4] * 100;
//         }
//         else {
//           pHVar5 = GetHumanoid(DAT_80097cbc[5]);
//           if (pHVar5 == (Humanoid *)0x0) {
//             return 0;
//           }
//           ViewInfo.vrx = pHVar5->locate->vx;
//           ViewInfo.vry = (pHVar5->locate->vy - (int)pHVar5->height) + 300;
//           ViewInfo.vrz = pHVar5->locate->vz;
//           DAT_80097cc4 = pHVar5;
//         }
//         GsSetRefView2(&ViewInfo);
//         break;
//       case 6:
//         DAT_80097cca = DAT_80097cbc[1];
//         DAT_80097cc8 = 0x14;
//         if (DAT_80097cbc[5] != 0) {
//           DAT_80097cc8 = DAT_80097cbc[5];
//         }
//         break;
//       case 7:
//         local_30.vx = DAT_80097cbc[2] * 10;
//         local_30.vy = DAT_80097cbc[3] * 10;
//         local_30.vz = DAT_80097cbc[4] * 10;
//         if (DAT_80097cbc[1] == 1) {
//           SetBlood(&local_30,(SVECTOR *)(int)DAT_80097cbc[5],0x1e,(short)DAT_80097cbc);
//         }
//         else if (DAT_80097cbc[1] == 3) {
//           FUN_80038fdc((char)DAT_80097cbc[2],(char)DAT_80097cbc[3],(char)DAT_80097cbc[4],
//                        (int)DAT_80097cbc[5]);
//         }
//         break;
//       case 8:
//         if (DAT_80097cbc[1] != -1) {
//           telop = (uchar *)strcpy((char *)&DAT_800c2c50,(char *)(DAT_80097cb8 + DAT_80097cbc[1]));
//           SetupTelop(telop);
//           DAT_80097ccc = 1;
//           if (((StageID != 10) || (PersistentState._4_1_ != '\0')) ||
//              (uVar12 = (uint)DAT_800c2c50, ((&DAT_8008ffb9)[uVar12] & 4) == 0)) break;
//           if (uVar12 == 0x30) {
//             piVar13 = &DAT_800c2cb0;
//             iVar15 = 0;
//             do {
//               iVar15 = iVar15 + 1;
//               *(ushort *)(*piVar13 + 0x5a) = *(ushort *)(*piVar13 + 0x5a) | 1;
//               piVar13 = piVar13 + 1;
//             } while (iVar15 < 6);
//           }
//           else {
//             *(ushort *)((&DAT_800c2cb0)[uVar12 - 0x31] + 0x5a) =
//                  *(ushort *)((&DAT_800c2cb0)[uVar12 - 0x31] + 0x5a) & 0xfffe;
//           }
//         }
//         DAT_800c2c50 = 0;
//       }
//       psVar6 = DAT_80097cbc + 6;
//       psVar1 = DAT_80097cbc + 6;
//       DAT_80097cbc = psVar6;
//     } while (*psVar1 != 1);
//   }
//   return (ushort)(DAT_80097cbc[1] != 0);
// }

#endif /* NON_MATCHING */
