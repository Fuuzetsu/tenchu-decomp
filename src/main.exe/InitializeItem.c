#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitializeItem(void);
 *     ITEM.C:533, 40 src lines, frame 32 bytes, saved-reg mask 0x80070000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $s1       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 *     extern struct GsSPRITE TargetSprite[1];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/InitializeItem", InitializeItem);

// triage: EASY — 81 insns, 1 loop, 5 callees, ~0.04 to PrepareGetScreenPositionS
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void InitializeItem(void)
//
// {
//   ulong *puVar1;
//   ModelType *pMVar2;
//   GsIMAGE *pGVar3;
//   tag_TItem *ptVar4;
//   GsSPRITE *sprite;
//   int iVar5;
//
//   puVar1 = GetArcData(0x14);
//   DAT_80097f48 = LoadModel(puVar1);
//   puVar1 = GetArcData(0x15);
//   DAT_80097f4c = LoadModel(puVar1);
//   puVar1 = GetArcData(0x1b);
//   DAT_80097f50 = LoadModel(puVar1);
//   puVar1 = GetArcData(0x1c);
//   DAT_80097f54 = LoadModel(puVar1);
//   iVar5 = 0;
//   ptVar4 = items;
//   do {
//     pMVar2 = LoadModel((ulong *)0x0);
//     ptVar4->locate = pMVar2;
//     ptVar4->proc = (undefined **)0x0;
//     iVar5 = iVar5 + 1;
//     ptVar4 = ptVar4 + 1;
//   } while (iVar5 < 0x1e);
//   sprite = TargetSprite;
//   for (iVar5 = 0; iVar5 < 1; iVar5 = iVar5 + 1) {
//     pGVar3 = GetImage(0x34);
//     InitSprite(pGVar3,sprite);
//     sprite->attribute = 0x50000000;
//     sprite = sprite + 1;
//   }
//   pGVar3 = GetImage(7);
//   DAT_80097f5c = SetupSprite((Sprite3D *)0x0,pGVar3);
//   (DAT_80097f5c->sprite).attribute = 0x50000000;
//   pGVar3 = GetImage(6);
//   DAT_80097f60 = SetupSprite((Sprite3D *)0x0,pGVar3);
//   (DAT_80097f60->sprite).attribute = 0x60000000;
//   pGVar3 = GetImage(0xd);
//   InitSprite(pGVar3,&SpriteGoshikimai);
//   DAT_80097ac8 = 1;
//   return;
// }
