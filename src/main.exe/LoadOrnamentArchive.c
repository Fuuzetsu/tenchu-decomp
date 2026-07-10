#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct OrnamentArchiveType * LoadOrnamentArchive(unsigned long *adr, struct ModelType *prnt);
 *     WORLD.C:259, 57 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
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
 *     param $s4       struct ModelType * prnt
 *     reg   $a3       struct ModelType * dim
 *     reg   $s1       struct OrnamentArchiveType * mad
 *     reg   $s3       struct ParentingType * prntp
 *     reg   $s5       unsigned char * tmdp
 *     reg   $s2       short i
 *     reg   $v1       short j
 *     reg   $s0       struct OrnamentType * objp
 *
 * Globals it touches, as the original declared them:
 *     extern struct ModelType World;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/LoadOrnamentArchive", LoadOrnamentArchive);

// triage: MEDIUM — 142 insns, 1 loop, 6 callees, ~0.07 to SetupMotionRegist
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// OrnamentArchiveType * LoadOrnamentArchive(ulong *adr,ModelType *prnt)
//
// {
//   ulong uVar1;
//   ModelType *dim;
//   undefined *puVar2;
//   OrnamentType *pOVar3;
//   int iVar4;
//   int iVar5;
//   ModelType *super;
//   int iVar6;
//   int iVar7;
//   short sVar8;
//   ushort uVar9;
//
//   if (adr == (ulong *)0x0) {
//                     /* WARNING: Subroutine does not return */
//     SystemOut((uchar *)"NO MODEL ARCHIVE DATA");
//   }
//   dim = (ModelType *)valloc(0x68);
//   (dim->object).attribute = (ulong)adr;
//   uVar1 = adr[1];
//   sVar8 = 0;
//   (dim->clip).vx = (ushort)uVar1;
//   puVar2 = valloc((int)((uint)(ushort)uVar1 << 0x10) >> 0xe);
//   *(undefined **)&(dim->clip).vz = puVar2;
//   while( true ) {
//     iVar7 = (int)sVar8;
//     if ((dim->clip).vx <= iVar7) break;
//     sVar8 = sVar8 + 1;
//     pOVar3 = LoadOrnament((ulong *)((int)adr + adr[iVar7 * 4 + 5] + 8));
//     *(uint *)(iVar7 * 4 + *(int *)&(dim->clip).vz) = (uint)pOVar3 | 0xa0000000;
//   }
//   if (prnt == (ModelType *)0x0) {
//     prnt = &World;
//   }
//   GsInitCoordinate2(&prnt->locate,(GsCOORDINATE2 *)dim);
//   (dim->locate).coord.t[0] = 0;
//   (dim->locate).coord.t[1] = 0;
//   (dim->locate).coord.t[2] = 0;
//   (dim->rotate).vx = 0;
//   (dim->rotate).vy = 0;
//   (dim->rotate).vz = 0;
//   UpdateCoordinate(dim);
//   uVar9 = 0;
//   dim->id = -1;
//   dim->attribute = 0;
//   do {
//     iVar6 = (int)(dim->clip).vx;
//     iVar7 = (int)(short)uVar9;
//     if (iVar6 <= iVar7) {
//       (dim->rotate).pad = *(short *)(**(int **)&(dim->clip).vz + 0x1c);
//       return (OrnamentArchiveType *)dim;
//     }
//     pOVar3 = *(OrnamentType **)(iVar7 * 4 + *(int *)&(dim->clip).vz);
//     super = dim;
//     if ((-1 < (short)adr[iVar7 * 4 + 2]) && (iVar5 = 0, 0 < iVar6)) {
//       iVar4 = 0;
//       do {
//         iVar5 = iVar5 + 1;
//         if ((short)adr[iVar7 * 4 + 2] == *(short *)((int)adr + (iVar4 >> 0x10) * 0x10 + 10)) {
//           super = *(ModelType **)((iVar4 >> 0x10) * 4 + *(int *)&(dim->clip).vz);
//           break;
//         }
//         iVar4 = iVar5 * 0x10000;
//       } while (iVar5 * 0x10000 >> 0x10 < iVar6);
//     }
//     GsInitCoordinate2(&super->locate,&pOVar3->locate);
//     iVar7 = (int)((uint)uVar9 << 0x10) >> 0xc;
//     (pOVar3->locate).coord.t[0] = (int)*(short *)((int)adr + iVar7 + 0xc);
//     (pOVar3->locate).coord.t[1] = (int)*(short *)((int)adr + iVar7 + 0xe);
//     (pOVar3->locate).coord.t[2] = (int)*(short *)((int)adr + iVar7 + 0x10);
//     UpdateOrnament(pOVar3,0);
//     uVar9 = uVar9 + 1;
//     (pOVar3->object).attribute = (pOVar3->object).attribute | 0x400;
//   } while( true );
// }
