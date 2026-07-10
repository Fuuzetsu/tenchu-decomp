#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateEvent(short n, short id);
 *     STAGE.C:249, 18 src lines, frame 32 bytes, saved-reg mask 0x80030000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $a0       short n
 *     param $a1       short id
 *     reg   $a3       short i
 *
 * Globals it touches, as the original declared them:
 *     extern struct EventSeqType *StageEvent;
 *     extern struct Humanoid *StagePlayer;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/UpdateEvent", UpdateEvent);

// triage: EASY — 92 insns, 1 loop, 1 callees, ~0.05 to update_something_for_each_visible_enemy_
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void UpdateEvent(int param_1,ushort param_2)
//
// {
//   Humanoid *pHVar1;
//   int iVar2;
//   int iVar3;
//   int iVar4;
//   EventSeqType *pEVar5;
//   undefined4 *puVar6;
//   int iVar7;
//   int *piVar8;
//
//   iVar4 = (param_1 << 0x10) >> 0xe;
//   puVar6 = (undefined4 *)((int)&DAT_80097f78 + iVar4);
//   *puVar6 = 0;
//   if ((param_2 != 0xff) &&
//      (iVar2._0_1_ = StageEvent->id, iVar2._1_1_ = StageEvent->event, iVar2._2_1_ = StageEvent->next1
//      , iVar2._3_1_ = StageEvent->next2, iVar7 = 0, iVar2 != -1)) {
//     piVar8 = (int *)((int)&DAT_80097f80 + iVar4);
//     iVar4 = 0;
//     do {
//       pEVar5 = StageEvent + (iVar4 >> 0x10);
//       iVar7 = iVar7 + 1;
//       if (pEVar5->id == param_2) {
//         *puVar6 = pEVar5;
//         if (pEVar5->target == 0xff) {
//           *piVar8 = (int)StagePlayer;
//         }
//         else {
//           pHVar1 = GetHumanoid((ushort)pEVar5->target);
//           *piVar8 = (int)pHVar1;
//         }
//         iVar4 = *piVar8;
//         if ((iVar4 != 0) &&
//            ((*(short *)(iVar4 + 2) != 0x11 || (*(short *)(*(int *)(iVar4 + 0x5c) + 4) != -1)))) {
//           if (1 < (ushort)(param_2 - 2)) {
//             return;
//           }
//           if (0 < *(short *)(*piVar8 + 8)) {
//             return;
//           }
//         }
//         *puVar6 = 0;
//         return;
//       }
//       pEVar5 = StageEvent + (iVar7 * 0x10000 >> 0x10);
//       iVar3._0_1_ = pEVar5->id;
//       iVar3._1_1_ = pEVar5->event;
//       iVar3._2_1_ = pEVar5->next1;
//       iVar3._3_1_ = pEVar5->next2;
//       iVar4 = iVar7 * 0x10000;
//     } while (iVar3 != -1);
//   }
//   return;
// }
