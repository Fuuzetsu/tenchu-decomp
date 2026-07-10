#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SoundEx(struct VECTOR *locate, short seid);
 *     SEMNG.C:42, 15 src lines, frame 40 bytes, saved-reg mask 0x800f0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $a0       struct VECTOR * locate
 *     param $a1       short seid
 *
 * Globals it touches, as the original declared them:
 *     extern struct Humanoid *StagePlayer;
 *     extern struct TCameraStatus CamState;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SoundEx", SoundEx);

// triage: MEDIUM — 128 insns, mul/div, 3 callees, ~0.06 to GetVectorRotation
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// short SoundEx(VECTOR *locate,short seid)
//
// {
//   short sVar1;
//   long lVar2;
//   int iVar3;
//   uint uVar4;
//   int iVar5;
//   VECTOR *pVVar6;
//   int iVar7;
//   int iVar8;
//
//   pVVar6 = StagePlayer->locate;
//   if ((locate == (VECTOR *)0x0) || (locate == pVVar6)) {
//     uVar4 = 0x7f;
//     if (seid == 0x12) {
//       uVar4 = 0x3f;
//     }
//   }
//   else {
//     iVar8 = locate->vx - pVVar6->vx;
//     iVar7 = locate->vz - pVVar6->vz;
//     lVar2 = SquareRoot0(iVar8 * iVar8 + iVar7 * iVar7);
//     if (17999 < lVar2) {
//       return -1;
//     }
//     iVar3 = locate->vy - pVVar6->vy;
//     if (iVar3 < 0) {
//       iVar3 = -iVar3;
//     }
//     if (9999 < iVar3) {
//       return -1;
//     }
//     if ((lVar2 < 2000) && (iVar5 = 0, iVar3 < 2000)) {
//       uVar4 = 0x7f;
//     }
//     else {
//       uVar4 = ((0x7f - (lVar2 << 7) / 18000) * (10000 - iVar3)) / 10000;
//       lVar2 = ratan2(-iVar8,-iVar7);
//       iVar5 = lVar2 - StagePlayer->rotate->vy;
//       if (CamState.Mode == CMODE_DIRECTION) {
//         iVar5 = iVar5 - CamState.DirectionRY;
//       }
//       if (iVar5 < 0x801) {
//         if (iVar5 < -0x7ff) {
//           iVar5 = iVar5 + 0x1000;
//         }
//       }
//       else {
//         iVar5 = 0x1000 - iVar5;
//       }
//     }
//     uVar4 = iVar5 << 8 | uVar4;
//   }
//   sVar1 = PlaySE(PTR_80097cb0,seid,uVar4);
//   return sVar1;
// }
