#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * SetupCharacterParameter(short type, struct Humanoid *human);
 *     APPEAR.C:173, 25 src lines, frame 32 bytes, saved-reg mask 0x80070000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $s2       short type
 *     param $s1       struct Humanoid * human
 *     reg   $a0       int idx
 *     reg   $a2       short * idtbl
 *
 * Globals it touches, as the original declared them:
 *     extern struct HumanDataType HumanData[63];
 *     extern short NowStage;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetupCharacterParameter", SetupCharacterParameter);

// triage: MEDIUM — 93 insns, 2 loop, 2 callees, ~0.07 to ReqItemGoshikimai
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// Humanoid * SetupCharacterParameter(short type,Humanoid *human)
//
// {
//   short sVar1;
//   short sVar2;
//   MotionManager *pMVar3;
//   HumanDataType *pHVar4;
//   short *psVar5;
//   int iVar6;
//
//   iVar6 = 0;
//   if (HumanData[0].type != -1) {
//     pHVar4 = HumanData;
//     sVar1 = HumanData[0].type;
//     do {
//       pHVar4 = pHVar4 + 1;
//       if (sVar1 == type) break;
//       sVar1 = pHVar4->type;
//       iVar6 = iVar6 + 1;
//     } while (sVar1 != -1);
//   }
//   human->turn = HumanData[iVar6].turn;
//   human->width = HumanData[iVar6].width;
//   human->height = HumanData[iVar6].height;
//   if ((HumanData[iVar6].mtbl)->motion == (MotionDataType *)0x0) {
//     SetupMotionRegist(HumanData[iVar6].mtbl);
//   }
//   pMVar3 = SetupMotionManager(human->model,HumanData[iVar6].mtbl);
//   human->motion = pMVar3;
//   sVar1 = HumanData[iVar6].life;
//   human->lifemax = sVar1;
//   human->life = sVar1;
//   sVar1 = -1;
//   if (1 < (ushort)type) {
//     psVar5 = *(short **)(&DAT_8008f188 + NowStage * 4);
//     sVar1 = 0;
//     if (*psVar5 != type) {
//       sVar2 = *psVar5;
//       do {
//         if (sVar2 == -1) break;
//         psVar5 = psVar5 + 1;
//         sVar2 = *psVar5;
//         sVar1 = sVar1 + 1;
//       } while (sVar2 != type);
//     }
//   }
//   human->sound = (sVar1 + 6) * 0x10;
//   return human;
// }
