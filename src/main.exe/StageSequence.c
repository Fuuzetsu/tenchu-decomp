#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short StageSequence(void);
 *     STAGE.C:153, 92 src lines, frame 56 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $s1       struct EventSeqType * ev
 *     reg   $s0       struct Humanoid * tgt
 *     reg   $s3       short i
 *     reg   $s2       short flag
 *     reg   $s0       long gc
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *StagePlayer;
 *     extern long StageTime;
 *     extern short ActionHalt;
 *     extern enum TSystemFlag SystemFlag;
 *     extern short SkipFrame;
 *     extern long GameClock;
 *     extern int StageID;
 *     extern short Findenemies;
 *     extern short Murders;
 *     extern short Criticals;
 *     extern short StageEnemies;
 *     extern short FriendHits;
 * END PSX.SYM */

/*
 * StageSequence (0x8004df58) — TODO one-line description.
 *
 * STATUS: NON_MATCHING — split (jump-table) function scaffolded by
 * tools/split-scaffold.py. The #ifndef NON_MATCHING branch is the stub
 * (INCLUDE_ASM pieces + the jump-table pool as one static const array so
 * the .rodata carve has bytes); build the draft with `NON_MATCHING=StageSequence
 * ./Build`. On a full match, delete the guards and the _jtbl array.
 */

#ifndef NON_MATCHING
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/StageSequence", StageSequence);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/StageSequence", handle_character_death_and_some_other_stuff___override__prt_8004e15c_96f729d4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/StageSequence", handle_character_death_and_some_other_stuff___override__prt_8004e17c_2685a9f3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/StageSequence", handle_character_death_and_some_other_stuff___override__prt_8004e19c_2685a9f3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/StageSequence", switchD_8004e250__switchD);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/StageSequence", switchD_8004e250__caseD_1);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/StageSequence", switchD_8004e250__caseD_0);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/StageSequence", switchD_8004e250__caseD_2);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/StageSequence", switchD_8004e250__caseD_3);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/StageSequence", switchD_8004e250__caseD_4);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/StageSequence", switchD_8004e250__caseD_5);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/StageSequence", switchD_8004e250__caseD_6);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/StageSequence", switchD_8004e250__caseD_7);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/StageSequence", switchD_8004e250__caseD_8);

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/StageSequence", switchD_8004e250__caseD_9);

/* jump-table pool @ 0x80012858 (9 words; tables at 0x80012858) — stub-only, one array because the object has one .rodata section; the draft's compiled switch emits its own. */
static const u32 StageSequence_jtbl[9] = {
    0x8004E358, 0x8004E258, 0x8004E360, 0x8004E370,
    0x8004E394, 0x8004E3B8, 0x8004E3C8, 0x8004E454,
    0x8004E480,
};

#else /* NON_MATCHING */
/* Draft — turn this into matching C, then delete the #ifndef/#else/
   #endif guards and the _jtbl array(s) above.  Reference: */
// 
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
// 
// short StageSequence(void)
// 
// {
//   bool bVar1;
//   bool bVar2;
//   long lVar3;
//   short sVar4;
//   short sVar5;
//   uint uVar6;
//   undefined4 uVar7;
//   int iVar8;
//   uint uVar9;
//   ushort sid;
//   VECTOR *pVVar10;
//   char *pcVar11;
//   VECTOR *pVVar12;
//   char *pcVar13;
//   int iVar14;
//   byte *pbVar15;
//   Humanoid *pHVar16;
//   undefined1 auStack_30 [16];
//   
//   bVar2 = false;
//   if (StagePlayer->status == 0x11) {
//     if ((StagePlayer->motion->loop != 0) || (sVar5 = 0, 0x1d < StagePlayer->motion->count)) {
//       if ((DAT_80097f78 == (byte *)0x0) && (DAT_80097f7c == (byte *)0x0)) {
//         if (StagePlayer->motion->loop != 0) {
//           StageTime = StageTime + 1;
//         }
//         sVar5 = 0;
//         if (-1 < StageTime) {
//           sVar5 = -1;
//         }
//       }
//       else {
//         UpdateEvent(0,2);
//         UpdateEvent(1,3);
//         StagePlayer->status = 1;
//         sVar4 = StageSequence();
//         sVar5 = -1;
//         if (sVar4 == 0) {
//           StageTime = -100;
//           DAT_80097f7c = (byte *)0x0;
//           DAT_80097f78 = (byte *)0x0;
//           if (CamState.Mode != CMODE_FALL) {
//             SetCameraMode(CMODE_CRITICAL_HIT);
//           }
//           sVar5 = 0;
//           ActionHalt = 0xffff;
//           StagePlayer->status = 0x11;
//         }
//       }
//     }
//   }
//   else {
//     if (((SystemFlag & 2) != 0) && (SkipFrame == 0)) {
//       FntPrint("%d-%d-%d-%d ",(char *)(StageID + 1),(char *)(uint)(byte)PersistentState._6_1_,
//                GameClock / 0x1e);
//       iVar14 = (int)Criticals;
//       FntPrint("%d/%d/%d(%d/%d) ",(char *)(int)Findenemies,(char *)(int)Murders,iVar14);
//       pcVar11 = (char *)(int)FriendHits;
//       pcVar13 = (char *)(int)StageCitizens;
//       FntPrint(s__d__d__80097c7c,pcVar11,pcVar13,iVar14);
//       if (DAT_80097f78 != (byte *)0x0) {
//         pcVar11 = (char *)(uint)*DAT_80097f78;
//         FntPrint(s___d__80097c84,pcVar11,pcVar13,iVar14);
//       }
//       if (DAT_80097f7c != (byte *)0x0) {
//         pcVar11 = (char *)(uint)*DAT_80097f7c;
//         FntPrint(s___d__80097c84,pcVar11,pcVar13,iVar14);
//       }
//       FntPrint(&DAT_80097c8c,pcVar11,pcVar13,iVar14);
//     }
//     iVar8 = 0;
//     StageTime = StageTime + 1;
//     iVar14 = 0;
//     do {
//       pbVar15 = *(byte **)((int)&DAT_80097f78 + (iVar14 >> 0xe));
//       if ((pbVar15 == (byte *)0x0) || ((1 < *pbVar15 - 2 && (StagePlayer->life == 0))))
//       goto LAB_8004e5e4;
//       pHVar16 = *(Humanoid **)((int)&DAT_80097f80 + (iVar14 >> 0xe));
//       switch(pbVar15[5]) {
//       case 0:
// switchD_8004e250_caseD_0:
//         bVar2 = true;
//         break;
//       case 1:
//         if (*pbVar15 - 2 < 2) {
//           pHVar16 = StagePlayer;
//         }
//         pVVar10 = pHVar16->locate;
//         iVar14 = (pVVar10->vx / 1000) * 0x10000 >> 0x10;
//         if ((((*(short *)(pbVar15 + 8) <= iVar14) && (iVar14 <= *(short *)(pbVar15 + 10))) &&
//             (iVar14 = (pVVar10->vz / 1000) * 0x10000 >> 0x10, *(short *)(pbVar15 + 0x10) <= iVar14))
//            && (((iVar14 <= *(short *)(pbVar15 + 0x12) &&
//                 (iVar14 = (pVVar10->vy / 1000) * 0x10000 >> 0x10,
//                 *(short *)(pbVar15 + 0xc) <= iVar14)) && (iVar14 <= *(short *)(pbVar15 + 0xe)))))
//         goto switchD_8004e250_caseD_0;
//         break;
//       case 2:
//         uVar9 = (uint)*(ushort *)(pbVar15 + 6);
//         uVar6 = (uint)(pHVar16->attribute & *(ushort *)(pbVar15 + 6));
//         goto code_r0x8004e3a8;
//       case 3:
//         if (StagePlayer->status != 7) {
//           sVar5 = pHVar16->status;
//           goto LAB_8004e3a0;
//         }
//         break;
//       case 4:
//         sVar5 = pHVar16->motion->mid;
// LAB_8004e3a0:
//         uVar9 = (uint)sVar5;
//         uVar6 = (uint)*(short *)(pbVar15 + 6);
// code_r0x8004e3a8:
//         if (uVar9 == uVar6) {
//           bVar2 = true;
//         }
//         break;
//       case 5:
//         bVar1 = *(short *)(pbVar15 + 6) < pHVar16->life;
// LAB_8004e470:
//         if (!bVar1) {
//           bVar2 = true;
//         }
//         break;
//       case 6:
//         pVVar12 = pHVar16->locate;
//         pVVar10 = StagePlayer->locate;
//         iVar14 = pVVar10->vy - pVVar12->vy;
//         if (iVar14 < 0) {
//           iVar14 = -iVar14;
//         }
//         if (iVar14 < 0x7d1) {
//           iVar14 = pVVar10->vx - pVVar12->vx;
//           if (iVar14 < 0) {
//             iVar14 = -iVar14;
//           }
//           if (iVar14 < 0x7d1) {
//             iVar14 = pVVar10->vz - pVVar12->vz;
//             if (iVar14 < 0) {
//               iVar14 = -iVar14;
//             }
//             if (iVar14 < 0x7d1) {
//               bVar2 = true;
//             }
//           }
//         }
//         break;
//       case 7:
//         if (-1 < *(short *)(pbVar15 + 6)) {
//           bVar1 = StageTime < *(short *)(pbVar15 + 6);
//           goto LAB_8004e470;
//         }
//         break;
//       case 8:
//         bVar2 = true;
//         PlayMusicFromID((int)*(short *)(pbVar15 + 6));
//       }
//       lVar3 = GameClock;
//       if (bVar2) {
//         bVar2 = false;
//         if ((*pbVar15 - 2 < 2) || (StagePlayer->life != 0)) {
//           if (pbVar15[1] == 0xff) {
// LAB_8004e554:
//             if (((StagePlayer->type == 0) && (pbVar15[5] == 5)) && (pHVar16->type == 0x8d)) {
//               pbVar15[2] = 100;
//             }
//             StageTime = 0;
//             GameClock = lVar3;
//             UpdateEvent(0,pbVar15[2]);
//             UpdateEvent(1,pbVar15[3]);
//             if (2 < *pbVar15 - 1) {
//               sVar5 = StageSequence();
//               return sVar5;
//             }
//             return 1;
//           }
//           sid = (ushort)pbVar15[1];
//           if ((sid == 0) && (StageID == 8)) {
//             uVar7 = FUN_8004e964(auStack_30);
//             iVar14 = FUN_8004e794(uVar7,(int)(short)StageID);
//             sid = 5 - *(short *)(iVar14 + 10);
//           }
//           sVar5 = CVAsequence(sid);
//           if (sVar5 != 0) goto LAB_8004e554;
//         }
//         *(undefined4 *)((int)&DAT_80097f78 + ((iVar8 << 0x10) >> 0xe)) = 0;
//       }
// LAB_8004e5e4:
//       iVar8 = iVar8 + 1;
//       iVar14 = iVar8 * 0x10000;
//     } while (iVar8 * 0x10000 >> 0x10 < 2);
//     sVar5 = 0;
//   }
//   return sVar5;
// }

#endif /* NON_MATCHING */
