#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void UpdateSplineControl(struct SplineControlType *spc);
 *     ACTION.C:365, 19 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $t0       struct SplineControlType * spc
 *     reg   $a2       struct MotionElementType * key0p
 *     reg   $a3       struct MotionElementType * key1n
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/UpdateSplineControl", UpdateSplineControl);

// triage: MEDIUM — 108 insns, mul/div, 0 callees, ~0.05 to bow_shoot_logic
// likely-relevant cookbook sections:
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void UpdateSplineControl(SplineControlType *spc)
//
// {
//   int iVar1;
//   int iVar2;
//   MotionElementType *pMVar3;
//   int iVar4;
//   MotionElementType *pMVar5;
//   MotionElementType *pMVar6;
//   MotionElementType *pMVar7;
//
//   pMVar5 = spc->key0;
//   pMVar3 = pMVar5;
//   if (pMVar5->time != 0) {
//     pMVar3 = pMVar5 + -1;
//   }
//   pMVar6 = spc->key1;
//   pMVar7 = pMVar6;
//   if (pMVar6->time < (spc->dd0).pad) {
//     pMVar7 = pMVar6 + 1;
//   }
//   iVar4 = (int)(((uint)(ushort)pMVar6->time - (uint)(ushort)pMVar5->time) * 0x1000000) >> 0x10;
//   iVar2 = (int)pMVar6->time - (int)pMVar3->time;
//   if (iVar2 == 0) {
//     trap(0x1c00);
//   }
//   if ((iVar2 == -1) && (iVar4 == -0x80000000)) {
//     trap(0x1800);
//   }
//   iVar1 = (int)pMVar7->time - (int)pMVar5->time;
//   if (iVar1 == 0) {
//     trap(0x1c00);
//   }
//   if ((iVar1 == -1) && (iVar4 == -0x80000000)) {
//     trap(0x1800);
//   }
//   iVar2 = (int)(short)(iVar4 / iVar2);
//   pMVar5 = spc->key1;
//   (spc->dd0).vx = (short)((uint)(iVar2 * ((int)pMVar6->x - (int)pMVar3->x)) >> 8);
//   pMVar6 = spc->key1;
//   (spc->dd0).vy = (short)((uint)(iVar2 * ((int)pMVar5->y - (int)pMVar3->y)) >> 8);
//   pMVar5 = spc->key0;
//   (spc->dd0).vz = (short)((uint)(iVar2 * ((int)pMVar6->z - (int)pMVar3->z)) >> 8);
//   iVar2 = (int)(short)(iVar4 / iVar1);
//   (spc->ds1).vx = (short)((uint)(iVar2 * ((int)pMVar7->x - (int)pMVar5->x)) >> 8);
//   pMVar3 = spc->key0;
//   (spc->ds1).vy = (short)((uint)(iVar2 * ((int)pMVar7->y - (int)spc->key0->y)) >> 8);
//   (spc->ds1).vz = (short)((uint)(iVar2 * ((int)pMVar7->z - (int)pMVar3->z)) >> 8);
//   return;
// }
