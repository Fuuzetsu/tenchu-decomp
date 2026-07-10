#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct BackGround * SetupBG(struct GsIMAGE *image, short w, short h);
 *     3DCTRL.C:622, 60 src lines, frame 96 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param stack+0   struct GsIMAGE * image
 *     param $a1       short w
 *     param $a2       short h
 *     reg   $s4       struct BackGround * bg
 *     reg   $s2       struct GsCELL * cell
 *     reg   $s5       short x
 *     stack sp+16     short y
 *     stack sp+24     short sy
 *     reg   $a1       short n
 *     reg   $s1       short pmode
 *     stack sp+32     short size
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetupBG", SetupBG);

// triage: HARD — 268 insns, mul/div, 3 loop, 6 callees, ~0.04 to GetVectorRotation
// likely-relevant cookbook sections:
//   - Loops: 3 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Removing unreachable block (ram,0x80017e70) */
// /* WARNING: Removing unreachable block (ram,0x80017e2c) */
// /* WARNING: Removing unreachable block (ram,0x80017e3c) */
// /* WARNING: Removing unreachable block (ram,0x80017e44) */
// /* WARNING: Removing unreachable block (ram,0x80017e80) */
// /* WARNING: Removing unreachable block (ram,0x80017e88) */
//
// BackGround * SetupBG(BackGround *bg,short w,short h)
//
// {
//   byte bVar1;
//   short sVar2;
//   ushort uVar3;
//   ulong uVar4;
//   u_short uVar5;
//   BackGround *bg_00;
//   ushort *puVar6;
//   GsCELL *pGVar7;
//   u_long *work;
//   int iVar8;
//   uint uVar9;
//   uint tp;
//   int iVar10;
//   ushort uVar11;
//   int iVar12;
//   ushort local_50;
//
//   if (bg == (BackGround *)0x0) {
//                     /* WARNING: Subroutine does not return */
//     SystemOut((uchar *)"NO BACKGROUND IMAGE DATA");
//   }
//   bg_00 = (BackGround *)valloc(0x48);
//   bg_00->id = -1;
//   bg_00->attribute = 0;
//   memset((uchar *)bg_00,'\0',0x24);
//   uVar4 = (bg->hundle).attribute;
//   (bg_00->map).cellh = '\x10';
//   (bg_00->hundle).b = 0x80;
//   (bg_00->hundle).g = 0x80;
//   (bg_00->hundle).r = 0x80;
//   (bg_00->hundle).scaley = 0x1000;
//   (bg_00->hundle).scalex = 0x1000;
//   (bg_00->hundle).w = w;
//   (bg_00->hundle).h = h;
//   (bg_00->hundle).map = &bg_00->map;
//   (bg_00->map).cellw = '\x10';
//   tp = (ushort)uVar4 & 3;
//   (bg_00->map).ncellw = w / 0x10;
//   (bg_00->hundle).attribute = tp << 0x18;
//   (bg_00->hundle).mx = w >> 1;
//   (bg_00->hundle).my = h >> 1;
//   (bg_00->map).ncellh = h / 0x10;
//   iVar12 = (int)(short)((w / 0x10) * (h / 0x10));
//   puVar6 = (ushort *)valloc(iVar12 << 1);
//   iVar10 = 0;
//   bg_00->index = puVar6;
//   (bg_00->map).index = puVar6;
//   if (0 < iVar12) {
//     do {
//       iVar8 = iVar10 + 1;
//       *(undefined2 *)(((iVar10 << 0x10) >> 0xf) + (int)bg_00->index) = 0xffff;
//       iVar10 = iVar8;
//     } while (iVar8 * 0x10000 >> 0x10 < iVar12);
//   }
//   uVar9 = (uint)(bg_00->map).cellw;
//   iVar10 = (uint)(ushort)(bg->hundle).w << (2 - tp & 0x1f);
//   if (uVar9 == 0) {
//     trap(0x1c00);
//   }
//   if ((uVar9 == 0xffffffff) && (iVar10 == -0x80000000)) {
//     trap(0x1800);
//   }
//   uVar3 = (bg->hundle).h;
//   bVar1 = (bg_00->map).cellh;
//   uVar11 = uVar3 / bVar1;
//   if (bVar1 == 0) {
//     trap(0x1c00);
//   }
//   if ((bVar1 == 0xffffffff) && (uVar3 == 0x80000000)) {
//     trap(0x1800);
//   }
//   iVar10 = (int)(short)(iVar10 / (int)uVar9);
//   local_50 = 0;
//   pGVar7 = (GsCELL *)valloc(iVar10 * (short)uVar11 * 8);
//   bg_00->cell = pGVar7;
//   (bg_00->map).base = pGVar7;
//   if (0 < (short)uVar11) {
//     do {
//       iVar12 = 0;
//       if (0 < iVar10) {
//         do {
//           bVar1 = (bg_00->map).cellw;
//           uVar3 = (bg->hundle).x;
//           iVar8 = (uint)(ushort)(bg->hundle).y + (int)(short)local_50 * (uint)(bg_00->map).cellh;
//           pGVar7 = bg_00->cell + (short)local_50 * iVar10 + (int)(short)iVar12;
//           pGVar7->v = (uchar)iVar8;
//           pGVar7->u = (char)(1 << (8 - tp & 0x1f)) - 1U &
//                       (char)((int)(short)uVar3 << (2 - tp & 0x1f)) + (char)iVar12 * bVar1;
//           sVar2._0_1_ = (bg->hundle).r;
//           sVar2._1_1_ = (bg->hundle).g;
//           uVar5 = GetClut((int)sVar2,(int)*(short *)&(bg->hundle).b);
//           pGVar7->cba = uVar5;
//           pGVar7->flag = 0;
//           uVar5 = GetTPage(tp,0,(int)(((uint)uVar3 +
//                                       (int)(short)iVar12 * ((int)(uint)bVar1 >> (2 - tp & 0x1f))) *
//                                      0x10000) >> 0x10,iVar8 * 0x10000 >> 0x10);
//           iVar12 = iVar12 + 1;
//           pGVar7->tpage = uVar5;
//         } while (iVar12 * 0x10000 >> 0x10 < iVar10);
//       }
//       local_50 = local_50 + 1;
//     } while ((int)((uint)local_50 << 0x10) < (int)((uint)uVar11 << 0x10));
//   }
//   work = (u_long *)
//          valloc((int)((((bg_00->map).ncellw + 1) * ((bg_00->map).ncellh + 2) * 0xc + 10) * 0x10000)
//                 >> 0xe);
//   bg_00->work = work;
//   GsInitFixBg16((GsBG *)bg_00,work);
//   bg_00->sz = 0;
//   return bg_00;
// }
