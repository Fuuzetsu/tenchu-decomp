#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutLifeBar(int x, int y, int n, int mx, int style);
 *     INFOVIEW.C:292, 32 src lines, frame 72 bytes, saved-reg mask 0x803f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s3       int x
 *     param $s4       int y
 *     param $s1       int n
 *     param $a3       int mx
 *     param stack+16  int style
 *     reg   $s5       int style
 *     stack sp+16     struct POLY_F4 poly
 *     reg   $a3       int w
 *     reg   $v0       int x
 *     reg   $a0       int y
 *     reg   $a3       int n
 *     reg   $s2       int ou
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE NumberImage;
 *     extern struct WorldType WorldMap[8][8][8];
 *     extern struct GsOT *OTablePt;
 *     extern long GameClock;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PutLifeBar", PutLifeBar);

// triage: MEDIUM — 135 insns, mul/div, 1 loop, 1 callees, ~0.07 to ReqItemNingyo
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void PutLifeBar(int x,int y,int n,int mx,int style)
//
// {
//   undefined2 uVar1;
//   short sVar2;
//   short sVar3;
//   int iVar4;
//   GsOT *pGVar5;
//   uchar uVar6;
//   undefined1 uVar7;
//   int iVar8;
//
//   uVar6 = NumberImage.u;
//   NumberImage.w = 4;
//   sVar2 = (short)x;
//   NumberImage.x = sVar2 + *(short *)(&DAT_8008e418 + style * 0x50);
//   sVar3 = (short)y;
//   NumberImage.y = sVar3 + *(short *)(&DAT_8008e41a + style * 0x50);
//   iVar8 = n;
//   do {
//     iVar4 = iVar8 / 10;
//     NumberImage.u = uVar6 + ((char)iVar8 + (char)iVar4 * -10) * '\x04';
//     GsSortSprite(&NumberImage,OTablePt,0);
//     pGVar5 = OTablePt;
//     NumberImage.x = NumberImage.x + -6;
//     iVar8 = iVar4;
//   } while (iVar4 != 0);
//   iVar8 = style * 0x50;
//   NumberImage.u = uVar6;
//   *(short *)(&DAT_8008e420 + iVar8) = sVar2;
//   *(short *)(iVar8 + -0x7ff71bde) = sVar3;
//   GsSortSprite((GsSPRITE *)(&DAT_8008e41c + style * 0x14),pGVar5,1);
//   if (mx == 0) {
//     trap(0x1c00);
//   }
//   if ((mx == -1) && (*(short *)(&DAT_8008e416 + iVar8) * n == -0x80000000)) {
//     trap(0x1800);
//   }
//   uVar1 = *(undefined2 *)(&DAT_8008e44a + iVar8);
//   *(short *)(&DAT_8008e44a + iVar8) =
//        *(short *)(&DAT_8008e414 + iVar8) + (short)((*(short *)(&DAT_8008e416 + iVar8) * n) / mx);
//   if (mx < 0) {
//     mx = mx + 3;
//   }
//   uVar7 = 0x80;
//   if ((n <= mx >> 2) && (uVar7 = 0xe6, (GameClock & 1U) == 0)) {
//     uVar7 = 0x80;
//   }
//   (&DAT_8008e456)[iVar8] = uVar7;
//   (&DAT_8008e455)[iVar8] = uVar7;
//   (&DAT_8008e454)[iVar8] = uVar7;
//   pGVar5 = OTablePt;
//   *(short *)(&DAT_8008e444 + iVar8) = sVar2;
//   *(short *)(iVar8 + -0x7ff71bba) = sVar3;
//   GsSortSprite((GsSPRITE *)(&DAT_8008e440 + style * 0x14),pGVar5,0);
//   *(undefined2 *)(&DAT_8008e44a + iVar8) = uVar1;
//   return;
// }
