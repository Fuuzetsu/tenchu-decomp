#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short GetCommand(struct PADtype *pad);
 *     PADCMD.C:333, 22 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $t0       struct PADtype * pad
 *     reg   $a0       unsigned short * cmd
 *     reg   $a2       short i
 *     reg   $a1       short j
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned short *Command[12];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/GetCommand", GetCommand);

// triage: MEDIUM — 73 insns, 3 loop, 0 callees, ~0.07 to LoadAreaMap
// likely-relevant cookbook sections:
//   - Loops: 3 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short GetCommand(PADtype *pad)
//
// {
//   int iVar1;
//   short sVar2;
//   int iVar3;
//   int iVar4;
//   undefined *puVar5;
//   int iVar6;
//   int iVar7;
//
//   iVar1 = 0;
//   puVar5 = Command;
//   while( true ) {
//     if (puVar5 == (undefined *)0x0) {
//       return 0;
//     }
//     iVar3 = *(int *)((int)&Command + ((iVar1 << 0x10) >> 0xe));
//     iVar7 = 0;
//     iVar6 = iVar3 + 2;
//     if (*(short *)(iVar3 + 2) == -1) break;
//     do {
//       iVar3 = (iVar7 << 0x10) >> 0xf;
//       iVar4 = iVar7 + 1;
//       if (*(short *)(iVar3 + iVar6) != *(short *)((int)pad->stream + iVar3)) break;
//       iVar7 = iVar4;
//     } while (*(short *)((iVar4 * 0x10000 >> 0xf) + iVar6) != -1);
//     if (*(short *)(((iVar7 << 0x10) >> 0xf) + iVar6) == -1) break;
//     puVar5 = *(undefined **)((int)&Command + ((iVar1 + 1) * 0x10000 >> 0xe));
//     iVar1 = iVar1 + 1;
//   }
//   iVar7 = 3;
//   do {
//     sVar2 = (short)iVar7;
//     iVar7 = iVar7 + -1;
//     pad->stream[sVar2] = pad->stream[sVar2 + -1];
//   } while (0 < iVar7 * 0x10000);
//   pad->stream[0] = 0;
//   return **(short **)((int)&Command + ((iVar1 << 0x10) >> 0xe));
// }
