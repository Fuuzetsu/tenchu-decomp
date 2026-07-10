#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitEffect(void);
 *     EFFECT.C:271, 91 src lines, frame 88 bytes, saved-reg mask 0x801f0000 (DEMO build -- see below)
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
 *     stack sp+16     struct GsIMAGE img
 *     reg   $s2       short i
 *     stack sp+48     int [3] img
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsSPRITE sprBlood;
 *     extern struct GsSPRITE sprSplash;
 *     extern struct GsSPRITE sprFrame[4];
 *     extern struct POLY_F4 plyBleed;
 *     extern struct tag_TItem items[30];
 *     extern struct Sprite3D *sprSmoke;
 *     extern struct Sprite3D *sprBomb[3];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/InitEffect", InitEffect);

// triage: MEDIUM — 226 insns, 2 loop, 6 callees, ~0.07 to DebugMenuItemSet
// likely-relevant cookbook sections:
//   - Loops: 2 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void InitEffect(void)
//
// {
//   GsIMAGE *pGVar1;
//   Sprite3D *pSVar2;
//   ulong *puVar3;
//   int iVar4;
//   short sVar5;
//   int iVar6;
//   int aiStack_20040 [16382];
//   byte abStack_10048 [65536];
//   undefined4 local_48;
//   undefined4 local_44;
//   int local_40 [4];
//   undefined4 local_30;
//
//   iVar6 = 0;
//   local_48 = DAT_80097a40;
//   local_44 = DAT_80097a44;
//   do {
//     iVar4 = (int)(short)iVar6;
//     pGVar1 = GetImage((uint)*(byte *)((int)&local_48 + iVar4 * 2));
//     InitSprite(pGVar1,&sprBlood + iVar4);
//     (&sprBlood)[iVar4].attribute = 0x50000000;
//     pGVar1 = GetImage((uint)*(byte *)((int)&local_48 + iVar4 * 2 + 1));
//     InitSprite(pGVar1,(GsSPRITE *)(&DAT_800be9e8 + iVar4 * 9));
//     iVar6 = iVar6 + 1;
//     ((GsSPRITE *)(&DAT_800be9e8 + iVar4 * 9))->attribute = 0x60000000;
//   } while (iVar6 * 0x10000 >> 0x10 < 4);
//   pGVar1 = GetImage(0xe);
//   InitSprite(pGVar1,&sprSplash);
//   sVar5 = 0;
//   sprSplash.attribute = 0x50000000;
//   sprSplash.my = sprSplash.h;
//   while( true ) {
//     iVar6 = (int)sVar5;
//     if (3 < iVar6) break;
//     sVar5 = sVar5 + 1;
//     pGVar1 = GetImage(pat[iVar6]);
//     InitSprite(pGVar1,sprFrame + iVar6);
//     sprFrame[iVar6].attribute = 0x50000000;
//   }
//   sVar5 = 0;
//   while( true ) {
//     iVar6 = (int)sVar5;
//     if (4 < iVar6) break;
//     sVar5 = sVar5 + 1;
//     pGVar1 = GetImage((uint)(byte)(&DAT_80097a48)[iVar6]);
//     InitSprite(pGVar1,(GsSPRITE *)(&DAT_800beaa8 + iVar6 * 9));
//     ((GsSPRITE *)(&DAT_800beaa8 + iVar6 * 9))->attribute = 0x50000000;
//   }
//   plyBleed.tag._3_1_ = 5;
//   plyBleed.code = '(';
//   for (sVar5 = 0; iVar6 = (int)sVar5, iVar6 < 2; sVar5 = sVar5 + 1) {
//     local_40[1] = 0x3a;
//     local_40[0] = 6;
//     pGVar1 = GetImage(local_40[iVar6]);
//     pSVar2 = SetupSprite((Sprite3D *)0x0,pGVar1);
//     (&sprSmoke)[iVar6] = pSVar2;
//     (pSVar2->sprite).attribute = 0x50000000;
//   }
//   for (sVar5 = 0; iVar6 = (int)sVar5, iVar6 < 3; sVar5 = sVar5 + 1) {
//     local_40[2] = DAT_80011c90;
//     local_40[3] = DAT_80011c94;
//     local_30 = DAT_80011c98;
//     pGVar1 = GetImage(local_40[iVar6 + 2]);
//     pSVar2 = SetupSprite((Sprite3D *)0x0,pGVar1);
//     sprBomb[iVar6] = pSVar2;
//     (pSVar2->sprite).attribute = 0x50000000;
//   }
//   puVar3 = GetArcData(0x19);
//   DAT_80097f34 = LoadModel(puVar3);
//   puVar3 = GetArcData(0x1f);
//   DAT_80097f38 = LoadModel(puVar3);
//   DAT_80097f3c = GetImage(10);
//   puVar3 = GetArcData(0x1a);
//   DAT_80097f28 = LoadModel(puVar3);
//   iVar6 = 0;
//   do {
//     pGVar1 = GetImage(0x37);
//     pSVar2 = SetupSprite((Sprite3D *)0x0,pGVar1);
//     iVar4 = iVar6 << 0x10;
//     iVar6 = iVar6 + 1;
//     *(Sprite3D **)((int)&DAT_80097f2c + (iVar4 >> 0xe)) = pSVar2;
//     (pSVar2->sprite).attribute = 0x50000000;
//   } while (iVar6 * 0x10000 < 1);
//   DAT_80097f30 = 0x340;
//   DAT_80097f32 = 0x100;
//   FUN_80039c14();
//   return;
// }
