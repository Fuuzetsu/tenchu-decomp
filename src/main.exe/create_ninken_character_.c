#include "common.h"
#include "main.exe.h"

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/create_ninken_character_", create_ninken_character_);

// triage: MEDIUM — 144 insns, 2 loop, 2 callees, ~0.09 to FUN_8004a598
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
//
// void FUN_8003d528(short param_1,int param_2)
//
// {
//   Humanoid *human;
//   undefined2 *puVar1;
//   ModelType **ppMVar2;
//   ModelArchiveType *pMVar3;
//   int iVar4;
//
//   DAT_80097f58 = BreedLife(0xa9,0xf3e58,0xf3e58,0xf3e58,0);
//   *(ushort *)(DAT_80097f58 + 4) = *(ushort *)(DAT_80097f58 + 4) | 0x80;
//   pMVar3 = (CamState.Owner)->model;
//   iVar4 = 0;
//   _DAT_800c0630 = (int)(pMVar3->rotate).pad;
//   puVar1 = &DAT_800c0630;
//   if (0 < pMVar3->n) {
//     do {
//       *(ulong **)(puVar1 + 2) = (pMVar3->object[iVar4]->object).tmd;
//       puVar1[4] = (short)(pMVar3->object[iVar4]->locate).coord.t[0];
//       puVar1[5] = (short)(pMVar3->object[iVar4]->locate).coord.t[1];
//       ppMVar2 = pMVar3->object + iVar4;
//       iVar4 = iVar4 + 1;
//       puVar1[6] = (short)((*ppMVar2)->locate).coord.t[2];
//       puVar1 = puVar1 + 6;
//     } while (iVar4 < pMVar3->n);
//   }
//   human = (Humanoid *)
//           BreedLife((&DAT_8008e3ec)[(uint)(param_1 == 1) + ((param_2 << 0x10) >> 0xf)],0xf3e58,
//                     0xf3e58,0xf3e58,0);
//   pMVar3 = human->model;
//   iVar4 = 0;
//   _DAT_800c06f0 = (int)(pMVar3->rotate).pad;
//   puVar1 = &DAT_800c06f0;
//   if (0 < pMVar3->n) {
//     do {
//       *(ulong **)(puVar1 + 2) = (pMVar3->object[iVar4]->object).tmd;
//       puVar1[4] = (short)(pMVar3->object[iVar4]->locate).coord.t[0];
//       puVar1[5] = (short)(pMVar3->object[iVar4]->locate).coord.t[1];
//       ppMVar2 = pMVar3->object + iVar4;
//       iVar4 = iVar4 + 1;
//       puVar1[6] = (short)((*ppMVar2)->locate).coord.t[2];
//       puVar1 = puVar1 + 6;
//     } while (iVar4 < pMVar3->n);
//   }
//   KillHumanoid(human);
//   return;
// }
