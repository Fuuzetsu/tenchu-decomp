#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static struct Humanoid * SearchItemTarget2(struct Humanoid *owner, struct SVECTOR *rot, struct VECTOR *start, struct VECTOR *target);
 *     ITEM.C:352, 62 src lines, frame 136 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $fp       struct Humanoid * owner
 *     param $s1       struct SVECTOR * rot
 *     param $s5       struct VECTOR * start
 *     param $s7       struct VECTOR * target
 *     reg   $s1       int i
 *     reg   $s4       int dist
 *     reg   $s6       struct Humanoid * ret
 *     stack sp+16     struct MATRIX mat
 *     stack sp+48     struct SVECTOR rrot
 *     stack sp+56     struct SVECTOR v
 *     reg   $s0       struct VECTOR * gpv
 *     reg   $s0       struct Humanoid * human
 *     stack sp+64     struct VECTOR tv
 *     stack sp+80     struct VECTOR lv
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *HumanGroup[32];
 *     extern short Humans;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SearchItemTarget2", SearchItemTarget2);

// triage: MEDIUM — 182 insns, 7 callees, ~0.09 to AfsRead
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// Humanoid * SearchItemTarget2(Humanoid *owner,SVECTOR *rot,VECTOR *start,VECTOR *target)
//
// {
//   bool bVar1;
//   int iVar2;
//   VECTOR *pVVar3;
//   int iVar4;
//   Humanoid *pHVar5;
//   Humanoid **ppHVar6;
//   int iVar7;
//   Humanoid *pHVar8;
//   MATRIX MStack_70;
//   SVECTOR local_50;
//   VECTOR local_48;
//   VECTOR local_38;
//
//   pHVar8 = (Humanoid *)0x0;
//   local_48.vx = DAT_800121f0;
//   local_48.vy = DAT_800121f4;
//   local_48.vz = DAT_800121f8;
//   local_48.pad = DAT_800121fc;
//   RotateVector(&local_48,(int)rot->vx,(int)rot->vy,(int)rot->vz);
//   local_48.vx = local_48.vx + start->vx;
//   local_48.vy = local_48.vy + start->vy;
//   local_48.vz = local_48.vz + start->vz;
//   FUN_80039ddc(start,&local_48,&local_38,0);
//   target->vx = local_38.vx;
//   target->vy = local_38.vy;
//   target->vz = local_38.vz;
//   iVar2 = GetVectorDistance(&local_38,start);
//   local_50.vx = -rot->vx;
//   local_50.vy = -rot->vy;
//   local_50.vz = -rot->vz;
//   RotMatrix(&local_50,&MStack_70);
//   ppHVar6 = HumanGroup;
//   for (iVar7 = 0; iVar7 < Humans; iVar7 = iVar7 + 1) {
//     pHVar5 = *ppHVar6;
//     pVVar3 = GetAbsolutePosition(*pHVar5->model->object,0,0,0);
//     local_48.vx = pVVar3->vx;
//     local_48.vy = pVVar3->vy;
//     local_48.vz = pVVar3->vz;
//     local_48.pad = pVVar3->pad;
//     local_38.vx = local_48.vx;
//     local_38.vy = local_48.vy;
//     local_38.vz = local_48.vz;
//     local_38.pad = local_48.pad;
//     if ((0 < pHVar5->life) && (pHVar5 != owner)) {
//       local_38.vx = local_48.vx - start->vx;
//       local_38.vy = local_48.vy - start->vy;
//       local_38.vz = local_48.vz - start->vz;
//       ApplyMatrixLV(&MStack_70,&local_38,&local_38);
//       local_38.vz = -local_38.vz;
//       if (100 < local_38.vz) {
//         bVar1 = false;
//         iVar4 = abs(local_38.vx);
//         if (iVar4 < 500) {
//           iVar4 = abs(local_38.vy);
//           bVar1 = iVar4 < 1000;
//         }
//         if ((bVar1) && (local_38.vz < iVar2)) {
//           target->vx = local_48.vx;
//           target->vy = local_48.vy;
//           target->vz = local_48.vz;
//           target->pad = local_48.pad;
//           iVar2 = local_38.vz;
//           pHVar8 = pHVar5;
//         }
//       }
//     }
//     ppHVar6 = ppHVar6 + 1;
//   }
//   return pHVar8;
// }
