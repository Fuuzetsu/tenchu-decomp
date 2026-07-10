#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void CVAsetup(void);
 *     CHRANIM.C:64, 15 src lines, frame 88 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     stack sp+24     unsigned char [50] name
 *
 * Globals it touches, as the original declared them:
 *     extern int StageID;
 *     extern struct POLY_F4 TelopbgP;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAsetup", CVAsetup);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/CVAsetup", reload_animations__override__prt_80050014_a9b16ba2);

// triage: MEDIUM — 134 insns, mul/div, 1 loop, 7 callees, ~0.04 to AddMisc
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void CVAsetup(void)
//
// {
//   ulong *adr;
//   Sprite3D *pSVar1;
//   int *piVar2;
//   undefined4 uVar3;
//   int iVar4;
//   int iVar5;
//   char acStack_78 [56];
//   GsIMAGE GStack_40;
//
//   if (DAT_80097cb8 != (ulong *)0x0) {
//     vfree((undefined *)DAT_80097cb8);
//   }
//   uVar3 = 0x41;
//   if (PersistentState._4_1_ == '\0') {
//     uVar3 = 0x52;
//   }
//   sprintf(acStack_78,"%sSTAGE%d%c.CAD",
//           (&PTR_s_K__WORK_CDIMAGE_ANIM_ENGLISH__8008ea68)[(byte)PersistentState._94_1_],StageID + 1,
//           uVar3);
//   DAT_80097cb8 = FileRead(acStack_78);
//   SetPolyF4(&TelopbgP);
//   TelopbgP.b0 = '\x01';
//   TelopbgP.g0 = '\x01';
//   TelopbgP.r0 = '\x01';
//   TelopbgP.x2 = -0xa0;
//   TelopbgP.x0 = -0xa0;
//   TelopbgP.x3 = 0xa0;
//   TelopbgP.x1 = 0xa0;
//   if ((StageID == 10) && (PersistentState._4_1_ == '\0')) {
//     adr = FileRead("K:\\WORK\\CDIMAGE\\ANIM\\tanka.tpd");
//     iVar5 = 0;
//     do {
//       iVar4 = (int)(short)iVar5;
//       GetTIMpackInfo(adr,&GStack_40,iVar4);
//       pSVar1 = SetupSprite((Sprite3D *)0x0,&GStack_40);
//       piVar2 = &DAT_800c2cb0 + iVar4;
//       *piVar2 = (int)pSVar1;
//       pSVar1->attribute = pSVar1->attribute | 1;
//       *(short *)(*piVar2 + 0x6c) = (2 - (short)iVar5) * 0x14 + 10;
//       *(short *)(*piVar2 + 0x6e) = (short)((iVar4 % 3) * 0x10000 >> 0xd) + -4;
//       iVar5 = iVar5 + 1;
//       iVar4 = *piVar2;
//       *(undefined1 *)(iVar4 + 0x7e) = 0;
//       *(undefined1 *)(iVar4 + 0x7d) = 0;
//       *(undefined1 *)(iVar4 + 0x7c) = 0;
//     } while (iVar5 * 0x10000 >> 0x10 < 6);
//     *(short *)(DAT_800c2cc4 + 0x6c) = *(short *)(DAT_800c2cc4 + 0x6c) + -8;
//     *(undefined2 *)(DAT_800c2cc4 + 0x6e) = 0x28;
//     LoadTIMpackAndFree(adr);
//   }
//   return;
// }
