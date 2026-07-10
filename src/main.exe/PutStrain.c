#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutStrain(void);
 *     INFOVIEW.C:218, 70 src lines, frame 32 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $s0       int newpow
 *     reg   $a2       int r
 *     reg   $s1       struct GsSPRITE * spr
 *
 * Globals it touches, as the original declared them:
 *     extern long StrainRatio;
 *     extern long GameClock;
 *     extern struct GsSPRITE NumberImage;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PutStrain", PutStrain);

// triage: MEDIUM — 146 insns, mul/div, 1 loop, 3 callees, ~0.04 to AdtFntOpen
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void PutStrain(void)
//
// {
//   GsOT *ot;
//   uchar uVar1;
//   short sVar2;
//   uint uVar3;
//   int iVar4;
//   short in_a0;
//   short in_a1;
//   int iVar5;
//   long lVar6;
//   GsSPRITE *_29;
//
//   uVar1 = NumberImage.u;
//   lVar6 = StrainRatio;
//   if (StrainRatio != 0x7fffffff) {
//     if (StrainRatio == 0) {
//       _29 = (GsSPRITE *)&DAT_800c0850;
//       NumberImage.u = uVar1;
//     }
//     else if (StrainRatio < -20000) {
//       _29 = (GsSPRITE *)0x800c0874;
//       lVar6 = 0;
//       NumberImage.u = uVar1;
//     }
//     else if (StrainRatio < 0) {
//       _29 = (GsSPRITE *)0x800c082c;
//       lVar6 = 0;
//       NumberImage.u = uVar1;
//       if (GameClock == (GameClock / 0x1e) * 0x1e) {
//         SoundEx((VECTOR *)0x0,0xe);
//       }
//     }
//     else {
//       if (20000 < StrainRatio) {
//         return;
//       }
//       _29 = (GsSPRITE *)&ItemSprite3Ds;
//       NumberImage.w = 4;
//       NumberImage.x = in_a0 + 0x22;
//       NumberImage.y = in_a1 + 8;
//       iVar4 = (20000 - StrainRatio) / 200;
//       do {
//         iVar5 = iVar4 / 10;
//         NumberImage.u = uVar1 + ((char)iVar4 + (char)iVar5 * -10) * '\x04';
//         GsSortSprite(&NumberImage,OTablePt,0);
//         NumberImage.x = NumberImage.x + -6;
//         iVar4 = iVar5;
//         NumberImage.u = uVar1;
//       } while (iVar5 != 0);
//     }
//     iVar5 = -lVar6 + 20000;
//     iVar4 = iVar5;
//     if (iVar5 < 0) {
//       iVar4 = -lVar6 + 0x4e3f;
//     }
//     _29->x = in_a0;
//     _29->y = in_a1;
//     uVar3 = (uint)DAT_80097f68 + (iVar4 >> 5);
//     DAT_80097f68 = (ushort)uVar3;
//     iVar4 = rsin(uVar3 & 0xffff);
//     iVar4 = iVar4 * 0x60;
//     if (iVar4 < 0) {
//       iVar4 = iVar4 + 0xfff;
//     }
//     uVar1 = (char)(iVar4 >> 0xc) + '\x7f';
//     _29->b = uVar1;
//     _29->g = uVar1;
//     _29->r = uVar1;
//     ot = OTablePt;
//     sVar2 = (short)((iVar5 * 0x800) / 20000) + 0x800;
//     _29->scalex = sVar2;
//     _29->scaley = sVar2;
//     GsSortSprite(_29,ot,0);
//   }
//   return;
// }
