#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * int leFindEnemy(void);
 *     WORLD.C:1099, 31 src lines, frame 88 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $s1       int i
 *     reg   $s6       long px
 *     reg   $s5       long py
 *     reg   $s4       long pz
 *     reg   $s3       int find
 *     reg   $s2       int r
 *     reg   $v1       int rr
 *     stack sp+16     struct SVECTOR pow
 *     stack sp+24     struct VECTOR pos
 *
 * Globals it touches, as the original declared them:
 *     extern struct TEnemyLayout enemy[30];
 *     extern struct Sprite3D *sprSmoke;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/leFindEnemy", leFindEnemy);

// triage: MEDIUM — 105 insns, mul/div, 3 callees, ~0.12 to InsertConflict
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// int leFindEnemy(void)
//
// {
//   ModelArchiveType *pMVar1;
//   int iVar2;
//   int iVar3;
//   int iVar4;
//   long lVar5;
//   TEnemyLayout *pTVar6;
//   int iVar7;
//   int iVar8;
//   int iVar9;
//   int iVar10;
//   int iVar11;
//   int iVar12;
//   SVECTOR local_48;
//   VECTOR local_40;
//   long local_30;
//   long local_2c;
//   long local_28;
//   long local_24;
//
//   iVar9 = -1;
//   iVar8 = 2000;
//   pMVar1 = (CamState.Owner)->model;
//   pTVar6 = enemy;
//   iVar12 = (pMVar1->locate).coord.t[0];
//   iVar11 = (pMVar1->locate).coord.t[1];
//   iVar10 = (pMVar1->locate).coord.t[2];
//   for (iVar7 = 0; iVar7 < 0x1e; iVar7 = iVar7 + 1) {
//     if ((pTVar6->type != -1) &&
//        (iVar2 = pTVar6->x - iVar12, iVar3 = pTVar6->y - iVar11, iVar4 = pTVar6->z - iVar10,
//        lVar5 = SquareRoot0(iVar2 * iVar2 + iVar3 * iVar3 + iVar4 * iVar4), lVar5 < iVar8)) {
//       iVar8 = lVar5;
//       iVar9 = iVar7;
//     }
//     pTVar6 = pTVar6 + 1;
//   }
//   if (iVar9 != -1) {
//     local_48.vx = SVECTOR_80097abc.vx;
//     local_48.vy = SVECTOR_80097abc.vy;
//     local_48.vz = SVECTOR_80097abc.vz;
//     local_48.pad = SVECTOR_80097abc.pad;
//     memset((uchar *)&local_30,'\0',0x10);
//     local_40.vx = enemy[iVar9].x;
//     local_40.vy = enemy[iVar9].y;
//     local_40.vz = enemy[iVar9].z;
//     local_40.pad = local_24;
//     local_30 = local_40.vx;
//     local_2c = local_40.vy;
//     local_28 = local_40.vz;
//     SetExplosion(&local_40,&local_48);
//   }
//   return iVar9;
// }
