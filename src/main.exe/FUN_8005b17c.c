#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsOT *OTablePt;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8005b17c", FUN_8005b17c);

// triage: HARD — 259 insns, 7 loop, 5 callees, ~0.05 to ProcItemGoshikimai
// likely-relevant cookbook sections:
//   - Loops: 7 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short FUN_8005b17c(int param_1,int param_2)
//
// {
//   char cVar1;
//   uchar uVar2;
//   uint uVar3;
//   char *pcVar4;
//   uchar *puVar5;
//   int iVar6;
//   uchar *puVar7;
//   short sVar8;
//   int iVar9;
//
//   if (param_1 == 0) {
//     DAT_80097d38 = 0;
//     return 0;
//   }
//   if (DAT_80097d38 != param_1) {
//     iVar6 = 1;
//     iVar9 = 0;
//     pcVar4 = DAT_80097d24;
//     if (param_1 != 1) {
//       do {
//         cVar1 = *pcVar4;
//         while ((cVar1 != '\0' || (pcVar4[1] != '\0'))) {
//           pcVar4 = pcVar4 + 1;
//           cVar1 = *pcVar4;
//           iVar9 = iVar9 + 1;
//         }
//         iVar6 = iVar6 + 1;
//         iVar9 = iVar9 + 2;
//         pcVar4 = pcVar4 + 2;
//       } while (param_1 != iVar6);
//     }
//     DAT_80097d3c = 0;
//     DAT_80097d40 = (uchar *)(DAT_80097d24 + iVar9);
//     DAT_80097d38 = param_1;
//     SoundEx((VECTOR *)0x0,0x1f);
//   }
//   GsSortSprite((GsSPRITE *)(DAT_80097d28 + 0x68),OTablePt,0);
//   iVar6 = 0;
//   puVar5 = DAT_80097d40;
//   if (*DAT_80097d40 != '\0') {
//     do {
//       uVar2 = *puVar5;
//       while (puVar5 = puVar5 + 1, uVar2 == '\0') {
//         uVar2 = *puVar5;
//         iVar6 = iVar6 + 1;
//         if (uVar2 == '\0') goto LAB_8005b290;
//       }
//     } while( true );
//   }
// LAB_8005b290:
//   iVar6 = (iVar6 * -0x10) / 2;
//   uVar2 = *DAT_80097d40;
//   puVar5 = DAT_80097d40;
//   while (uVar2 != '\0') {
//     uVar2 = *puVar5;
//     puVar7 = puVar5;
//     while (uVar2 != '\0') {
//       puVar7 = puVar7 + 1;
//       uVar2 = *puVar7;
//     }
//     SetupTelop(puVar5);
//     iVar9 = FUN_800576e8(puVar5);
//     FUN_800570b8(OTablePt->org,-(iVar9 / 2),iVar6,puVar5);
//     iVar6 = iVar6 + 0x10;
//     puVar5 = puVar7 + 1;
//     uVar2 = puVar7[1];
//   }
//   sVar8 = 0;
//   if (DAT_80097d3c != 0) {
//     param_2 = 0;
//   }
//   if (puVar5[-2] == '.') {
//     GsSortSprite((GsSPRITE *)(DAT_800c2d5c + 0x68),OTablePt,0);
//     if (param_2 != 0x20) goto LAB_8005b550;
//   }
//   else {
//     if (puVar5[-2] != '?') goto LAB_8005b550;
//     if (param_1 == 3) {
//       if (DAT_80097d2c == 0) {
//         *(uint *)(DAT_800c2d60 + 0x68) = *(uint *)(DAT_800c2d60 + 0x68) | 0x40000000;
//         uVar3 = *(uint *)(DAT_800c2d64 + 0x68) & 0xbfffffff;
//       }
//       else {
//         *(uint *)(DAT_800c2d60 + 0x68) = *(uint *)(DAT_800c2d60 + 0x68) & 0xbfffffff;
//         uVar3 = *(uint *)(DAT_800c2d64 + 0x68) | 0x40000000;
//       }
//       *(uint *)(DAT_800c2d64 + 0x68) = uVar3;
//       GsSortSprite((GsSPRITE *)(DAT_800c2d60 + 0x68),OTablePt,0);
//       GsSortSprite((GsSPRITE *)(DAT_800c2d64 + 0x68),OTablePt,0);
//       GsSortSprite((GsSPRITE *)(DAT_800c2d68 + 0x68),OTablePt,0);
//       if (param_2 == 0x2000) {
//         if (DAT_80097d2c != 0) {
//           SoundEx((VECTOR *)0x0,0x30);
//           DAT_80097d2c = 0;
//         }
//         goto LAB_8005b550;
//       }
//       if (0x2000 < param_2) {
//         if ((param_2 == 0x8000) && (DAT_80097d2c != 1)) {
//           SoundEx((VECTOR *)0x0,0x30);
//           DAT_80097d2c = 1;
//         }
//         goto LAB_8005b550;
//       }
//       if (param_2 != 0x20) goto LAB_8005b550;
//       if (DAT_80097d2c == 0) goto LAB_8005b544;
//     }
//     else {
//       GsSortSprite((GsSPRITE *)(DAT_800c2d58 + 0x68),OTablePt,0);
//       if (param_2 != 0x20) {
//         if (param_2 != 0x40) goto LAB_8005b550;
// LAB_8005b544:
//         SoundEx((VECTOR *)0x0,0x31);
//         sVar8 = 2;
//         goto LAB_8005b550;
//       }
//     }
//   }
//   SoundEx((VECTOR *)0x0,0x30);
//   sVar8 = 1;
// LAB_8005b550:
//   if (sVar8 != 0) {
//     DAT_80097d3c = 1;
//   }
//   return sVar8;
// }
