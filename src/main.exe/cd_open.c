#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern enum TSystemFlag SystemFlag;
 * END PSX.SYM */

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/cd_open", cd_open);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/cd_open", FUN_8005f278__override__prt_8005f2b8_e1c3c140);

// triage: MEDIUM — 66 insns, 4 loop, 3 callees, ~0.08 to DisposeWeapon
// likely-relevant cookbook sections:
//   - Loops: 4 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// disc_file_descriptor_t * cd_open(char *param_1)
//
// {
//   int iVar1;
//   CdlFILE *pCVar2;
//   int iVar3;
//   disc_file_descriptor_t *_30;
//   char *pcVar4;
//   char acStack_60 [80];
//
//   if (*param_1 == '\\') {
//     pcVar4 = s__s_1_80097e88;
//   }
//   else {
//     pcVar4 = s___s_1_80097e80;
//   }
//   sprintf(acStack_60,pcVar4);
//   iVar3 = 0;
//   iVar1 = 0;
//   do {
//     _30 = (disc_file_descriptor_t *)(&FileHandlePool[0].location.minute + (iVar1 >> 0xb));
//     iVar3 = iVar3 + 1;
//     if (*(int *)(&FileHandlePool[0].used + (iVar1 >> 0xb)) == 0) goto LAB_8005f304;
//     iVar1 = iVar3 * 0x10000;
//   } while (iVar3 * 0x10000 >> 0x10 < 10);
//   _30 = (disc_file_descriptor_t *)0x0;
// LAB_8005f304:
//   iVar1 = 0;
//   if (_30 == (disc_file_descriptor_t *)0x0) {
//     pcVar4 = "open:out of handle";
//   }
//   else {
//     do {
//       pCVar2 = CdSearchFile((CdlFILE *)_30,acStack_60);
//       iVar1 = iVar1 + 1;
//       if (pCVar2 != (CdlFILE *)0x0) {
//         _30->used = '\x01';
//         _30->field19_0x19 = '\0';
//         _30->field20_0x1a = '\0';
//         _30->field21_0x1b = '\0';
//         _30->cursor = 0;
//         return _30;
//       }
//     } while (iVar1 * 0x10000 >> 0x10 < 10);
//     pcVar4 = "open:file not found";
//   }
//   puts(pcVar4);
//   return (disc_file_descriptor_t *)0x0;
// }
