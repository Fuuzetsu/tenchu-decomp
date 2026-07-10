#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct ModelArchiveType * LoadModelArchive(unsigned long *adr, struct ModelType *prnt);
 *     3DCTRL.C:335, 54 src lines, frame 56 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s0       unsigned long * adr
 *     param $s6       struct ModelType * prnt
 *     reg   $a3       struct ModelType * dim
 *     reg   $s2       struct ModelArchiveType * mad
 *     reg   $s5       struct ParentingType * prntp
 *     reg   $s7       unsigned char * tmdp
 *     reg   $s3       short i
 *     reg   $v1       short j
 *     reg   $s0       struct ModelType * objp
 *     reg   $s0       struct ModelType * dim
 *     reg   $s2       struct ModelType * dim
 *     reg   $s0       struct ModelType * objp
 *     reg   $s0       struct ModelType * dim
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/LoadModelArchive", LoadModelArchive);

// triage: MEDIUM — 190 insns, 3 loop, 6 callees, ~0.13 to LoadModel
// likely-relevant cookbook sections:
//   - Loops: 3 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// ModelArchiveType * LoadModelArchive(ulong *adr,ModelType *prnt)
//
// {
//   short sVar1;
//   ushort uVar2;
//   ulong uVar3;
//   ModelType *base;
//   ModelType **ppMVar4;
//   int iVar5;
//   ModelType *pMVar6;
//   int iVar7;
//   ModelType *base_00;
//   int iVar8;
//   int iVar9;
//
//   if (adr == (ulong *)0x0) {
//                     /* WARNING: Subroutine does not return */
//     SystemOut((uchar *)"NO MODEL ARCHIVE DATA");
//   }
//   base = (ModelType *)valloc(0x6c);
//   uVar3 = adr[1];
//   iVar9 = 0;
//   *(ushort *)&base->object = (ushort)uVar3;
//   ppMVar4 = (ModelType **)valloc((int)((uint)(ushort)uVar3 << 0x10) >> 0xe);
//   *(ModelType ***)((int)&base->object + 4) = ppMVar4;
//   if (0 < *(short *)&base->object) {
//     iVar5 = 0;
//     do {
//       iVar8 = (int)adr + adr[(iVar5 >> 0x10) * 4 + 5] + 8;
//       pMVar6 = (ModelType *)valloc(0x74);
//       if (iVar8 != 0) {
//         GsMapModelingData((ulong *)(iVar8 + 4));
//         GsLinkObject4(iVar8 + 0xc,&pMVar6->object,0);
//       }
//       (pMVar6->object).coord2 = (GsCOORDINATE2 *)pMVar6;
//       (pMVar6->object).attribute = 0;
//       GsInitCoordinate2(&World.locate,(GsCOORDINATE2 *)pMVar6);
//       (pMVar6->locate).coord.t[0] = 0;
//       (pMVar6->locate).coord.t[1] = 0;
//       (pMVar6->locate).coord.t[2] = 0;
//       (pMVar6->rotate).vx = 0;
//       (pMVar6->rotate).vy = 0;
//       (pMVar6->rotate).vz = 0;
//       (pMVar6->clip).vx = 0;
//       (pMVar6->clip).vy = 0;
//       (pMVar6->clip).vz = 0;
//       RotMatrixYXZ(&pMVar6->rotate,&(pMVar6->locate).coord);
//       iVar9 = iVar9 + 1;
//       pMVar6->id = -1;
//       (pMVar6->locate).flg = 0;
//       pMVar6->attribute = 0;
//       (*(ModelType ***)((int)&base->object + 4))[iVar5 >> 0x10] = pMVar6;
//       iVar5 = iVar9 * 0x10000;
//     } while (iVar9 * 0x10000 >> 0x10 < (int)*(short *)&base->object);
//   }
//   if (prnt == (ModelType *)0x0) {
//     prnt = &World;
//   }
//   GsInitCoordinate2(&prnt->locate,(GsCOORDINATE2 *)base);
//   (base->locate).coord.t[0] = 0;
//   (base->locate).coord.t[1] = 0;
//   (base->locate).coord.t[2] = 0;
//   (base->rotate).vx = 0;
//   (base->rotate).vy = 0;
//   (base->rotate).vz = 0;
//   (base->clip).vx = 0;
//   (base->clip).vy = 0;
//   (base->clip).vz = 0;
//   RotMatrixYXZ(&base->rotate,&(base->locate).coord);
//   iVar9 = 0;
//   uVar2 = *(ushort *)&base->object;
//   sVar1 = *(short *)&base->object;
//   (base->locate).flg = 0;
//   base->id = -1;
//   base->attribute = 0;
//   if (0 < sVar1) {
//     iVar5 = 0;
//     do {
//       base_00 = (*(ModelType ***)((int)&base->object + 4))[iVar5 >> 0x10];
//       pMVar6 = base;
//       if ((-1 < (short)adr[(iVar5 >> 0x10) * 4 + 2]) && (iVar8 = 0, 0 < (int)((uint)uVar2 << 0x10)))
//       {
//         iVar7 = 0;
//         do {
//           iVar8 = iVar8 + 1;
//           if ((short)adr[(iVar5 >> 0x10) * 4 + 2] ==
//               *(short *)((int)adr + (iVar7 >> 0x10) * 0x10 + 10)) {
//             pMVar6 = (*(ModelType ***)((int)&base->object + 4))[iVar7 >> 0x10];
//             break;
//           }
//           iVar7 = iVar8 * 0x10000;
//         } while (iVar8 * 0x10000 >> 0x10 < (int)*(short *)&base->object);
//       }
//       GsInitCoordinate2(&pMVar6->locate,&base_00->locate);
//       iVar5 = (iVar9 << 0x10) >> 0xc;
//       (base_00->locate).coord.t[0] = (int)*(short *)((int)adr + iVar5 + 0xc);
//       (base_00->locate).coord.t[1] = (int)*(short *)((int)adr + iVar5 + 0xe);
//       (base_00->locate).coord.t[2] = (int)*(short *)((int)adr + iVar5 + 0x10);
//       RotMatrixYXZ(&base_00->rotate,&(base_00->locate).coord);
//       iVar9 = iVar9 + 1;
//       (base_00->locate).flg = 0;
//       uVar2 = (ushort)(base->object).attribute;
//       iVar5 = iVar9 * 0x10000;
//     } while (iVar9 * 0x10000 >> 0x10 < (int)(short)(base->object).attribute);
//   }
//   (base->rotate).pad = *(short *)(((base->object).coord2)->flg + 0x1c);
//   return (ModelArchiveType *)base;
// }
