#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SelectStage(void);
 *     INFOVIEW.C:675, 15 src lines, frame 1016 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
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
 *     stack sp+16     struct TAdtSelect [10] StageSelect
 *     stack sp+96     unsigned char [9][100] name
 *     reg   $s0       int i
 * END PSX.SYM */

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SelectStage", SelectStage);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/SelectStage", debug_menu_select_stage__override__prt_8005c4d8_8cf8befb);

// triage: MEDIUM — 105 insns, 2 loop, frame 0x528, 2 callees, ~0.12 to AddItem2
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - Stack objects: 0x528 frame — buffer casts / spills

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
//
// void FUN_8005c404(int param_1)
//
// {
//   uchar **ppuVar1;
//   undefined **ppuVar2;
//   TAdtSelect *pTVar3;
//   undefined **ppuVar4;
//   TAdtSelect *pTVar5;
//   undefined *puVar6;
//   char *pcVar7;
//   undefined *puVar8;
//   uint uVar9;
//   TStageConfig *pTVar10;
//   ulong uVar11;
//   TAdtSelect local_518;
//   char *local_510;
//   ulong local_50c;
//   char *local_508;
//   ulong local_504 [5];
//   TAdtSelect local_4f0;
//   undefined *local_4e8;
//   undefined4 local_4e4;
//   undefined4 local_4e0;
//   undefined4 local_4dc;
//   TAdtSelect local_4d8 [14];
//   char acStack_468 [1104];
//
//   ppuVar2 = &PTR_s_JAPANESE_800141c0;
//   pTVar3 = &local_518;
//   do {
//     pTVar5 = pTVar3;
//     ppuVar4 = ppuVar2;
//     puVar6 = ppuVar4[1];
//     pcVar7 = ppuVar4[2];
//     puVar8 = ppuVar4[3];
//     pTVar5->name = *ppuVar4;
//     pTVar5->value = (ulong)puVar6;
//     pTVar5[1].name = pcVar7;
//     pTVar5[1].value = (ulong)puVar8;
//     ppuVar2 = ppuVar4 + 4;
//     pTVar3 = pTVar5 + 2;
//   } while (ppuVar4 + 4 != (undefined **)&UNK_800141e0);
//   pcVar7 = acStack_468;
//   pTVar10 = StageConfig;
//   puVar6 = ppuVar4[5];
//   pTVar5[2].name = _UNK_800141e0;
//   pTVar5[2].value = (ulong)puVar6;
//   local_4f0.name = PTR_s_RIKIMARU_800141f4;
//   local_4f0.value = DAT_800141f8;
//   local_4e8 = PTR_s_AYAME_800141fc;
//   local_4e4 = DAT_80014200;
//   local_4e0 = DAT_80014204;
//   local_4dc = DAT_80014208;
//   for (uVar11 = 0; (int)uVar11 < 0xb; uVar11 = uVar11 + 1) {
//     uVar9 = (uint)pTVar10->uid;
//     ppuVar1 = &pTVar10->name;
//     pTVar10 = pTVar10 + 1;
//     sprintf(pcVar7,s__2d__s_80097dc0,uVar9,*ppuVar1);
//     local_4d8[uVar9].name = pcVar7;
//     pcVar7 = pcVar7 + 100;
//     local_4d8[uVar9].value = uVar11;
//   }
//   local_4d8[uVar11].name = &DAT_80097dc8;
//   local_4d8[uVar11].value = 0xb;
//   local_4d8[uVar11 + 1].name = (char *)0x0;
//   do {
//     uVar11 = AdtSelect("language select",&local_518,0);
//     *(char *)(param_1 + 0x5e) = (char)uVar11;
//     uVar11 = AdtSelect("player select",&local_4f0,0);
//     *(char *)(param_1 + 4) = (char)uVar11;
//     uVar11 = AdtSelect("stage select",local_4d8,0);
//     *(char *)(param_1 + 5) = (char)uVar11;
//   } while (10 < (uVar11 & 0xff));
//   return;
// }
