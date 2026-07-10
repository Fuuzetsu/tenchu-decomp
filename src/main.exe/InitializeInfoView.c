#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitializeInfoView(void);
 *     INFOVIEW.C:119, 74 src lines, frame 64 bytes, saved-reg mask 0x80070000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $s0       int i
 *     stack sp+16     unsigned char [25] image
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE CursorImage;
 *     extern struct WorldType WorldMap[8][8][8];
 *     extern unsigned char fInitialize;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/InitializeInfoView", InitializeInfoView);

// triage: MEDIUM — 88 insns, 3 loop, 6 callees, ~0.04 to AfsInit
// likely-relevant cookbook sections:
//   - Loops: 3 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void InitializeInfoView(void)
//
// {
//   GsIMAGE *pGVar1;
//   Sprite3D *pSVar2;
//   int *piVar3;
//   GsSPRITE *sprite;
//   int iVar4;
//
//   pGVar1 = GetImage(0x32);
//   InitSprite(pGVar1,&CursorImage);
//   CursorImage.attribute = 0x50000000;
//   pGVar1 = GetImage(0x33);
//   InitSprite(pGVar1,&NumberImage);
//   iVar4 = 0;
//   piVar3 = &DAT_8008e5bc;
//   do {
//     pGVar1 = GetImage(iVar4 + 0x14);
//     pSVar2 = SetupSprite((Sprite3D *)0x0,pGVar1);
//     *piVar3 = (int)pSVar2;
//     pSVar2->scale = 0x3000;
//     iVar4 = iVar4 + 1;
//     *(undefined2 *)(*piVar3 + 0x5a) = 0x1c;
//     piVar3 = piVar3 + 1;
//   } while (iVar4 < 0x14);
//   if (iVar4 < 0x1a) {
//     piVar3 = &DAT_8008e5bc + iVar4;
//     do {
//       pGVar1 = GetImage(0xf);
//       pSVar2 = SetupSprite((Sprite3D *)0x0,pGVar1);
//       *piVar3 = (int)pSVar2;
//       pSVar2->scale = 0x3000;
//       iVar4 = iVar4 + 1;
//       *(undefined2 *)(*piVar3 + 0x5a) = 0x1c;
//       piVar3 = piVar3 + 1;
//     } while (iVar4 < 0x1a);
//   }
//   iVar4 = 0;
//   sprite = (GsSPRITE *)&ItemSprite3Ds;
//   do {
//     pGVar1 = GetImage(iVar4 + 0x2e);
//     InitSprite(pGVar1,sprite);
//     sprite->attribute = 0x50000000;
//     iVar4 = iVar4 + 1;
//     sprite = sprite + 1;
//   } while (iVar4 < 4);
//   leResetEnemyLayout();
//   ResetInfoview(-1);
//   FUN_8004a6bc();
//   fInitialize = 1;
//   return;
// }
