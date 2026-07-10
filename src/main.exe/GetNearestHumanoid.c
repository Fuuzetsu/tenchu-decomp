#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct Humanoid * GetNearestHumanoid(struct Humanoid *human, short distance);
 *     HUMAN.C:408, 24 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct Humanoid * human
 *     param $a1       short distance
 *
 * Globals it touches, as the original declared them:
 *     extern short Humans;
 *     extern struct Humanoid *HumanGroup[32];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/GetNearestHumanoid", GetNearestHumanoid);

// triage: MEDIUM — 91 insns, mul/div, 1 loop, 1 callees, ~0.09 to GetVectorRotation
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// Humanoid * GetNearestHumanoid(Humanoid *human,short distance)
//
// {
//   int iVar1;
//   int iVar2;
//   long lVar3;
//   Humanoid *pHVar4;
//   Humanoid **ppHVar5;
//   int iVar6;
//   int iVar7;
//   Humanoid *pHVar8;
//
//   pHVar8 = (Humanoid *)0x0;
//   iVar7 = 20000;
//   if (((human->map).height == 0) && (iVar6 = 0, 0 < Humans)) {
//     ppHVar5 = HumanGroup;
//     do {
//       pHVar4 = *ppHVar5;
//       if ((((pHVar4 != human) && (pHVar4->status != 0x11)) && ((pHVar4->attribute & 0x80U) == 0)) &&
//          (((pHVar4->type & 0xf0U) != 0x90 && (-1 < pHVar4->life)))) {
//         iVar1 = pHVar4->locate->vx - human->locate->vx;
//         if (iVar1 < 0) {
//           iVar1 = -iVar1;
//         }
//         iVar2 = pHVar4->locate->vz - human->locate->vz;
//         if (iVar2 < 0) {
//           iVar2 = -iVar2;
//         }
//         lVar3 = SquareRoot0(iVar1 * iVar1 + iVar2 * iVar2);
//         if ((lVar3 < distance) && (lVar3 < iVar7)) {
//           iVar7 = lVar3;
//           pHVar8 = pHVar4;
//         }
//       }
//       iVar6 = iVar6 + 1;
//       ppHVar5 = ppHVar5 + 1;
//     } while (iVar6 < Humans);
//   }
//   return pHVar8;
// }
