#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DrawModelArchive(struct ModelArchiveType *mad, long gap);
 *     3DCTRL.C:393, 28 src lines, frame 88 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s2       struct ModelArchiveType * mad
 *     param $s3       long gap
 *     reg   $s0       struct ModelType * objp
 *     stack sp+16     struct MATRIX mat
 *     stack sp+48     struct SVECTOR pos
 *     reg   $s1       short i
 *     reg   $s2       struct ModelType * objp
 *     reg   $v1       long sz
 *     reg   $s0       short atr
 *     stack sp+56     short [2] rxy
 *
 * Globals it touches, as the original declared them:
 *     extern short SkipFrame;
 *     extern struct SVECTOR UnitVector;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DrawModelArchive", DrawModelArchive);

// triage: MEDIUM — 122 insns, 1 loop, 4 callees, ~0.05 to update_something_for_each_visible_enemy_
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
//
// short DrawModelArchive(ModelArchiveType *mad,long gap)
//
// {
//   ushort uVar1;
//   int iVar2;
//   long lVar3;
//   int iVar4;
//   GsCOORDINATE2 *m;
//   MATRIX MStack_48;
//   short local_20;
//   short local_1e;
//
//   if (SkipFrame == 1) {
//     return 1;
//   }
//   if (gap < 0) goto LAB_800177d4;
//   GsGetLs(&mad->locate,&MStack_48);
//   GsSetLsMatrix(&MStack_48);
//   uVar1 = mad->attribute;
//   iVar2 = -1;
//   if ((uVar1 & 1) == 0) {
//     if ((uVar1 & 2) == 0) {
//       lVar3 = RotTransPers(&mad->clip,(long *)&local_20,(long *)0x0,(long *)0x0);
//       if (((uVar1 & 4) == 0) || (iVar2 = -1, lVar3 >> 2 != 0)) {
//         if ((uVar1 & 8) == 0) {
// LAB_80017764:
//           if (((uVar1 & 0x10) == 0) || (iVar2 = -1, lVar3 >> 2 < 0x4e3)) goto LAB_80017774;
//         }
//         else {
//           iVar4 = (int)local_20;
//           if (iVar4 < 0) {
//             iVar4 = -iVar4;
//           }
//           iVar2 = -1;
//           if (iVar4 < 0xf1) {
//             iVar2 = (int)local_1e;
//             if (iVar2 < 0) {
//               iVar2 = -iVar2;
//             }
//             if (iVar2 < 0xb5) goto LAB_80017764;
//             goto LAB_8001779c;
//           }
//         }
//       }
//     }
//     else {
// LAB_80017774:
//       lVar3 = RotTransPers(&UnitVector,(long *)0x0,(long *)0x0,(long *)0x0);
//       iVar2 = lVar3 >> 2;
//       if (iVar2 < 0x4e3) {
//         if (iVar2 < 300) {
//           _DrawTMDmode = 0;
//         }
//         else {
//           _DrawTMDmode = 0x20;
//         }
//       }
//       else {
// LAB_8001779c:
//         iVar2 = -1;
//       }
//     }
//   }
//   if (iVar2 + gap < 0) {
//     return 0;
//   }
// LAB_800177d4:
//   iVar2 = 0;
//   if (0 < mad->n) {
//     iVar4 = 0;
//     do {
//       m = *(GsCOORDINATE2 **)((iVar4 >> 0xe) + (int)mad->object);
//       if ((m[1].coord.m[1][0] & 1U) == 0) {
//         GsGetLs(m,&MStack_48);
//         GsSetLsMatrix(&MStack_48);
//         DrawTMD(m[1].coord.m[2] + 2,OTablePt,gap);
//       }
//       iVar2 = iVar2 + 1;
//       iVar4 = iVar2 * 0x10000;
//     } while (iVar2 * 0x10000 >> 0x10 < (int)mad->n);
//   }
//   return 1;
// }
