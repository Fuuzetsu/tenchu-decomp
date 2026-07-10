#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void StartStageSequence(void);
 *     STAGE.C:93, 56 src lines, frame 48 bytes, saved-reg mask 0x801f0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $s2       struct StageCharType * stg
 *     reg   $s0       struct Humanoid * human
 *     reg   $v0       long y
 *     reg   $a1       short i
 *     reg   $a0       short tp
 *
 * Globals it touches, as the original declared them:
 *     extern struct StageCharType StageChar[18];
 *     extern struct Humanoid *HumanGroup[32];
 *     extern int StageID;
 *     extern struct Humanoid *StagePlayer;
 *     extern short Humans;
 *     extern unsigned long *GlobalAreaMap;
 *     extern short StageCitizens;
 *     extern short StageEnemies;
 *     extern struct EventSeqType *StageEvent;
 *     extern long GameClock;
 *     extern long StageTime;
 *     extern short ActionHalt;
 * END PSX.SYM */

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/StartStageSequence", StartStageSequence);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/StartStageSequence", INIT_STAGE_STATS);

// triage: HARD — 378 insns, 7 loop, 5 callees, ~0.04 to GetHumanoid
// likely-relevant cookbook sections:
//   - Loops: 7 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void StartStageSequence(void)
//
// {
//   int iVar1;
//   Humanoid *pHVar2;
//   long lVar3;
//   int iVar4;
//   VECTOR *pVVar5;
//   ModelType *pMVar6;
//   int iVar7;
//   ushort *puVar8;
//   short sVar9;
//   ushort uVar10;
//   int iVar11;
//   short sVar12;
//   int iVar13;
//   undefined4 *puVar14;
//   short *psVar15;
//   StageCharType *pSVar16;
//   int local_b8 [40];
//
//   pSVar16 = StageChar;
//   if (StageChar[0].stage != -1) {
//     psVar15 = &StageChar[0].think;
//     do {
//       if ((int)pSVar16->stage == StageID + 1) {
//         sVar9 = psVar15[-5];
//         if (psVar15[-5] == -2) {
//           sVar9 = 5;
//           if (StagePlayer->type == 0) {
//             sVar9 = 6;
//           }
//         }
//         else if ((psVar15[-5] == -1) && (sVar9 = 2, StagePlayer->type == 0)) {
//           sVar9 = 3;
//         }
//         iVar13 = 0;
//         sVar12 = 0;
//         if (0 < Humans) {
//           iVar1 = 0;
//           do {
//             sVar12 = (short)iVar13;
//             iVar13 = iVar13 + 1;
//             if (**(short **)((int)HumanGroup + (iVar1 >> 0xe)) == sVar9) break;
//             iVar1 = iVar13 * 0x10000;
//             sVar12 = (short)iVar13;
//           } while (iVar13 * 0x10000 >> 0x10 < (int)Humans);
//         }
//         if ((int)sVar12 == (int)Humans) {
//           pHVar2 = (Humanoid *)BreedLife((int)sVar9,0,0,0,0);
//         }
//         else {
//           pHVar2 = HumanGroup[sVar12];
//         }
//         iVar13 = ((SVECTOR *)(psVar15 + -4))->vx * 1000;
//         pHVar2->point[0] = iVar13;
//         pHVar2->locate->vx = iVar13;
//         sVar9 = psVar15[-2];
//         pVVar5 = pHVar2->locate;
//         pHVar2->point[1] = sVar9 * 1000;
//         pVVar5->vz = sVar9 * 1000;
//         pHVar2->locate->vy = psVar15[-3] * 1000;
//         lVar3 = GetAreaMapLevel(GlobalAreaMap,pHVar2->locate->vx,pHVar2->locate->vy + -1000);
//         if (lVar3 < pHVar2->locate->vy) {
//           pHVar2->locate->vy = lVar3;
//         }
//         pHVar2->rotate->vy = psVar15[-1];
//         UpdateCoordinate((ModelType *)pHVar2->model);
//         SetupThinkFunction(pHVar2,*psVar15);
//         pMVar6 = (ModelType *)StagePlayer->model;
//         pHVar2->attribute = pHVar2->attribute & 0xfffbU | 0x82;
//         pHVar2->life = -1;
//         pHVar2->target = pMVar6;
//       }
//       pSVar16 = pSVar16 + 1;
//       psVar15 = psVar15 + 7;
//     } while (pSVar16->stage != -1);
//   }
//   iVar1 = 0;
//   iVar7 = (int)Humans;
//   iVar13 = 0;
//   if (0 < iVar7) {
//     iVar4 = 0;
//     iVar11 = iVar13;
//     do {
//       puVar14 = (undefined4 *)((int)HumanGroup + (iVar4 >> 0xe));
//       puVar8 = (ushort *)*puVar14;
//       iVar13 = iVar11;
//       if ((puVar8 != (ushort *)0x0) && ((*puVar8 & 0xf0) == 0)) {
//         iVar13 = iVar11 + 1;
//         *(ushort **)((int)local_b8 + ((iVar11 << 0x10) >> 0xe)) = puVar8;
//         *puVar14 = 0;
//       }
//       iVar1 = iVar1 + 1;
//       iVar4 = iVar1 * 0x10000;
//       iVar11 = iVar13;
//     } while (iVar1 * 0x10000 >> 0x10 < iVar7);
//     iVar7 = (int)Humans;
//   }
//   iVar1 = 0;
//   if (0 < iVar7) {
//     iVar4 = 0;
//     iVar11 = iVar13;
//     do {
//       puVar14 = (undefined4 *)((int)HumanGroup + (iVar4 >> 0xe));
//       puVar8 = (ushort *)*puVar14;
//       iVar13 = iVar11;
//       if ((puVar8 != (ushort *)0x0) && ((*puVar8 & 0xf0) == 0x80)) {
//         iVar13 = iVar11 + 1;
//         *(ushort **)((int)local_b8 + ((iVar11 << 0x10) >> 0xe)) = puVar8;
//         *puVar14 = 0;
//       }
//       iVar1 = iVar1 + 1;
//       iVar4 = iVar1 * 0x10000;
//       iVar11 = iVar13;
//     } while (iVar1 * 0x10000 >> 0x10 < iVar7);
//   }
//   iVar1 = 0;
//   if (0 < Humans) {
//     iVar7 = 0;
//     do {
//       iVar7 = *(int *)((int)HumanGroup + (iVar7 >> 0xe));
//       iVar11 = iVar13;
//       if (iVar7 != 0) {
//         iVar11 = iVar13 + 1;
//         *(int *)((int)local_b8 + ((iVar13 << 0x10) >> 0xe)) = iVar7;
//       }
//       iVar1 = iVar1 + 1;
//       iVar7 = iVar1 * 0x10000;
//       iVar13 = iVar11;
//     } while (iVar1 * 0x10000 >> 0x10 < (int)Humans);
//   }
//   iVar13 = (int)Humans;
//   iVar1 = 0;
//   if (0 < iVar13) {
//     do {
//       iVar7 = iVar1 << 0x10;
//       iVar1 = iVar1 + 1;
//       iVar7 = iVar7 >> 0xe;
//       *(undefined4 *)((int)HumanGroup + iVar7) = *(undefined4 *)((int)local_b8 + iVar7);
//     } while (iVar1 * 0x10000 >> 0x10 < iVar13);
//   }
//   StageCitizens = 0;
//   StageEnemies = 0;
//   StageBosses = 0;
//   iVar13 = 0;
//   if (0 < Humans) {
//     iVar1 = 0;
//     do {
//       pHVar2 = *(Humanoid **)((int)HumanGroup + (iVar1 >> 0xe));
//       if (((-1 < pHVar2->lifemax) && (pHVar2 != StagePlayer)) && (pHVar2->type != 0xa9)) {
//         uVar10 = pHVar2->type & 0xf0;
//         if (uVar10 == 0x80) {
//           StageBosses = StageBosses + 1;
//         }
//         else if (uVar10 == 0x90) {
//           StageCitizens = StageCitizens + 1;
//           goto LAB_8004de70;
//         }
//         StageEnemies = StageEnemies + 1;
//       }
// LAB_8004de70:
//       iVar13 = iVar13 + 1;
//       iVar1 = iVar13 * 0x10000;
//     } while (iVar13 * 0x10000 >> 0x10 < (int)Humans);
//   }
//   if (StageID < 2) goto LAB_8004def4;
//   if (3 < StageID) {
//     if (StageID != 10) goto LAB_8004def4;
//     StageBosses = StageBosses + -1;
//     StageEnemies = StageEnemies + -1;
//     if (StagePlayer->type == 1) goto LAB_8004def4;
//   }
//   StageBosses = StageBosses + -1;
//   StageEnemies = StageEnemies + -1;
// LAB_8004def4:
//   GameClock = 0;
//   StageTime = 0;
//   ActionHalt = 0;
//   AttackActionCount = 0;
//   FriendHits = 0;
//   Murders = 0;
//   Findenemies = 0;
//   Criticals = 0;
//   if (StageEvent != (EventSeqType *)0x0) {
//     UpdateEvent(0,0);
//     DAT_80097f7c = 0;
//   }
//   return;
// }
