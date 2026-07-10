#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void TurnAroundAllItems(struct Humanoid *user);
 *     ITEM.C:4023, 10 src lines, frame 88 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s5       struct Humanoid * user
 *     reg   $s4       int i
 *     reg   $s1       int j
 *     reg   $s5       struct Humanoid * human
 *     reg   $s4       int itemID
 *     stack sp+16     struct PARAM_ITEM_LAUNCH p
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/TurnAroundAllItems", TurnAroundAllItems);

// triage: MEDIUM — 102 insns, mul/div, 4 callees, ~0.11 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void TurnAroundAllItems(Humanoid *param_1)
//
// {
//   VECTOR *pVVar1;
//   int iVar2;
//   int iVar3;
//   TItemType TVar4;
//   PARAM_ITEM_USE local_48;
//
//   for (TVar4 = ITEM_KAGINAWA; iVar3 = 0, (int)TVar4 < 0x19; TVar4 = TVar4 + ITEM_SHURIKEN) {
//     for (; iVar3 < (int)(uint)param_1->item[TVar4]; iVar3 = iVar3 + 1) {
//       pVVar1 = GetAbsolutePosition(*param_1->model->object,0,0,0);
//       memset((uchar *)&local_48,'\0',0x28);
//       local_48.start.vx = pVVar1->vx;
//       local_48.start.vy = pVVar1->vy;
//       local_48.start.vz = pVVar1->vz;
//       local_48.type = TVar4;
//       local_48.user = param_1;
//       iVar2 = rand();
//       local_48.end.vx = iVar2 % 200 + -100;
//       iVar2 = rand();
//       local_48.end.vy = iVar2 % 100 + -200;
//       iVar2 = rand();
//       local_48.end.vz = iVar2 % 200 + -100;
//       ReqItemDrop(&local_48);
//     }
//     param_1->item[TVar4] = '\0';
//   }
//   return;
// }
