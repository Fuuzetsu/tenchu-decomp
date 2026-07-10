#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void InitMisc(void);
 *     MISC.C:162, 83 src lines, frame 40 bytes, saved-reg mask 0x800f0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     reg   $s1       int i
 *     reg   $a0       int iDoor1
 *     reg   $s1       int iDoor2
 *     reg   $v0       struct ModelType * data
 *     reg   $a0       int id1
 *     reg   $s1       int id2
 *     reg   $v0       struct ModelType * data
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TMisc misc[200];
 *     extern struct tag_TItem items[30];
 *     extern struct MISC__183fake DoorData[11];
 *     extern struct MISC__185fake SpriteData[2];
 *     extern struct MISC__184fake PitfallData[2];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/InitMisc", InitMisc);

// triage: MEDIUM — 90 insns, 4 loop, 4 callees, ~0.07 to ResetAllMisc
// likely-relevant cookbook sections:
//   - Loops: 4 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void InitMisc(void)
//
// {
//   tag_TMisc *ptVar1;
//   ulong *puVar2;
//   ModelType *pMVar3;
//   GsIMAGE *image;
//   Sprite3D *pSVar4;
//   _183fake_25 *p_Var5;
//   _185fake_27 *p_Var6;
//   _184fake_26 *p_Var7;
//   int iVar8;
//   ModelType *pMVar9;
//
//   iVar8 = 199;
//   ptVar1 = misc + 199;
//   do {
//     ptVar1->proc = (undefined **)0x0;
//     iVar8 = iVar8 + -1;
//     ptVar1 = ptVar1 + -1;
//   } while (-1 < iVar8);
//   iVar8 = 0;
//   p_Var5 = DoorData;
//   do {
//     pMVar9 = p_Var5->Model[1];
//     if (p_Var5->Model[0] != (ModelType *)0xffffffff) {
//       puVar2 = GetArcData((int)p_Var5->Model[0]);
//       pMVar3 = LoadModel(puVar2);
//       p_Var5->Model[0] = pMVar3;
//     }
//     if (pMVar9 != (ModelType *)0xffffffff) {
//       puVar2 = GetArcData((int)pMVar9);
//       pMVar9 = LoadModel(puVar2);
//       p_Var5->Model[1] = pMVar9;
//     }
//     iVar8 = iVar8 + 1;
//     p_Var5 = p_Var5 + 1;
//   } while (iVar8 < 0xb);
//   iVar8 = 0;
//   p_Var6 = SpriteData;
//   do {
//     iVar8 = iVar8 + 1;
//     image = GetImage((int)p_Var6->spr);
//     pSVar4 = SetupSprite((Sprite3D *)0x0,image);
//     p_Var6->spr = pSVar4;
//     (pSVar4->sprite).attribute = 0x50000000;
//     p_Var6->spr->scale = p_Var6->scale;
//     p_Var6 = p_Var6 + 1;
//   } while (iVar8 < 2);
//   iVar8 = 0;
//   p_Var7 = PitfallData;
//   do {
//     pMVar9 = p_Var7->Model[1];
//     if (p_Var7->Model[0] != (ModelType *)0xffffffff) {
//       puVar2 = GetArcData((int)p_Var7->Model[0]);
//       pMVar3 = LoadModel(puVar2);
//       p_Var7->Model[0] = pMVar3;
//     }
//     if (pMVar9 != (ModelType *)0xffffffff) {
//       puVar2 = GetArcData((int)pMVar9);
//       pMVar9 = LoadModel(puVar2);
//       p_Var7->Model[1] = pMVar9;
//     }
//     iVar8 = iVar8 + 1;
//     p_Var7 = p_Var7 + 1;
//   } while (iVar8 < 3);
//   DAT_80097c44 = 1;
//   return;
// }
