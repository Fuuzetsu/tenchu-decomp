#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct WorldType WorldMap[8][8][8];
 *     extern enum TSystemFlag SystemFlag;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/CheckCheatCodes", CheckCheatCodes);

// triage: EASY — 91 insns, 1 loop, 3 callees, ~0.16 to DebugMenuItemSet
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void CheckCheatCodes(undefined4 param_1,int param_2)
//
// {
//   adt_menu_entry_t *paVar1;
//   TAdtSelect *pTVar2;
//   int iVar3;
//   adt_menu_entry_t *paVar4;
//   TAdtSelect *pTVar5;
//   short seid;
//   ulong uVar6;
//   char *pcVar7;
//   ulong uVar8;
//   TAdtSelect local_d8;
//   char *local_d0;
//   ulong local_cc;
//   char *local_c8;
//   ulong local_c4 [45];
//
//   iVar3 = memcmp(param_1,&DAT_8008e4f0,param_2 << 1);
//   if (iVar3 == 0) {
//     SoundEx((VECTOR *)0x0,10);
//     paVar1 = adt_menu_entry_t_ARRAY_800123f8;
//     pTVar2 = &local_d8;
//     do {
//       pTVar5 = pTVar2;
//       paVar4 = paVar1;
//       uVar6 = paVar4->index;
//       pcVar7 = paVar4[1].label;
//       uVar8 = paVar4[1].index;
//       pTVar5->name = paVar4->label;
//       pTVar5->value = uVar6;
//       pTVar5[1].name = pcVar7;
//       pTVar5[1].value = uVar8;
//       paVar1 = paVar4 + 2;
//       pTVar2 = pTVar5 + 2;
//     } while (paVar4 + 2 != adt_menu_entry_t_ARRAY_800123f8 + 0x18);
//     uVar6 = paVar4[2].index;
//     pTVar5[2].name = adt_menu_entry_t_ARRAY_800123f8[0x18].label;
//     pTVar5[2].value = uVar6;
//     uVar6 = AdtSelect("select item",&local_d8,0);
//     local_d8.name = PTR_s_10_800124cc;
//     local_d8.value = DAT_800124d0;
//     local_d0 = PTR_s_100_800124d4;
//     local_cc = DAT_800124d8;
//     local_c8 = PTR_s_FULL_800124dc;
//     local_c4[0] = DAT_800124e0;
//     local_c4[1] = DAT_800124e4;
//     local_c4[2] = DAT_800124e8;
//     uVar8 = AdtSelect("number of",&local_d8,0);
//     seid = 0x4c;
//     (CamState.Owner)->item[uVar6] = (CamState.Owner)->item[uVar6] + (char)uVar8;
//   }
//   else {
//     iVar3 = memcmp(param_1,&DAT_8008e4c4,param_2 << 1);
//     if (iVar3 != 0) {
//       return;
//     }
//     seid = 10;
//     SystemFlag = SystemFlag | 2;
//   }
//   SoundEx((VECTOR *)0x0,seid);
//   return;
// }
