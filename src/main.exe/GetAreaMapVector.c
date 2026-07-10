#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * long GetAreaMapVector(unsigned long *area, struct MapVector *mvp, struct VECTOR *pos, long wide, int mode);
 *     CONFLICT.C:180, 39 src lines, frame 64 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param stack+0   unsigned long * area
 *     param $s0       struct MapVector * mvp
 *     param $s1       struct VECTOR * pos
 *     param $s7       long wide
 *     param stack+16  int mode
 *     reg   $s4       short mode
 *     reg   $s2       short i
 *     reg   $s1       short v
 *     reg   $a0       long level
 *     reg   $s6       long x
 *     reg   $s3       long y
 *     reg   $s5       long z
 *
 * Globals it touches, as the original declared them:
 *     extern short FieldAttrib;
 *     extern struct AreaNodeType *FieldArea;
 *     extern struct NodeIndexType *FieldIndex;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/GetAreaMapVector", GetAreaMapVector);

// triage: MEDIUM — 137 insns, mul/div, 1 loop, 1 callees, ~0.08 to AdtFntOpen
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// long GetAreaMapVector(MapVector *mvp,VECTOR *pos,long wide)
//
// {
//   short sVar1;
//   AreaNodeType *pAVar2;
//   NodeIndexType *pNVar3;
//   long lVar4;
//   int in_a3;
//   byte bVar5;
//   int iVar6;
//   long z;
//   int x;
//   ushort in_stack_00000010;
//
//   x = *(int *)wide;
//   z = *(long *)(wide + 4);
//   lVar4 = GetAreaMapLevel((ulong *)mvp,x,z);
//   pNVar3 = FieldIndex;
//   pAVar2 = FieldArea;
//   sVar1 = FieldAttrib;
//   pos->vx = lVar4;
//   *(short *)&pos->vz = sVar1;
//   pos[1].vx = (long)pAVar2;
//   pos[1].vy = (long)pNVar3;
//   if (lVar4 == -0x80000000) {
//     pos->vy = 0;
//     if ((in_stack_00000010 & 4) == 0) {
//       *(undefined2 *)&pos->vz = 2;
//       *(undefined1 *)((int)&pos->pad + 3) = 0xf;
//       *(undefined1 *)((int)&pos->pad + 2) = 0xf;
//       *(undefined1 *)&pos->pad = 0xf;
//       return -0x80000000;
//     }
//   }
//   else {
//     pos->vy = lVar4 - *(int *)(wide + 4);
//   }
//   iVar6 = 0;
//   bVar5 = 1;
//   *(undefined1 *)((int)&pos->pad + 3) = 0;
//   *(undefined1 *)((int)&pos->pad + 2) = 0;
//   *(undefined1 *)&pos->pad = 0;
//   do {
//     lVar4 = GetAreaMapLevel((ulong *)mvp,
//                             x + *(short *)((int)direction[0] + ((iVar6 << 0x10) >> 0xe)) * in_a3,z);
//     if ((lVar4 == -0x80000000) ||
//        (((lVar4 - z < -500 && ((in_stack_00000010 & 4) == 0)) &&
//         ((((ushort)pos->vz | FieldAttrib) & 0xc000) == 0)))) {
//       *(byte *)&pos->pad = bVar5 | (byte)pos->pad;
//     }
//     else if (pos->vx < lVar4) {
//       *(byte *)((int)&pos->pad + 2) = bVar5 | *(byte *)((int)&pos->pad + 2);
//     }
//     else if (lVar4 < pos->vx) {
//       *(byte *)((int)&pos->pad + 3) = bVar5 | *(byte *)((int)&pos->pad + 3);
//     }
//     iVar6 = iVar6 + 1;
//     bVar5 = bVar5 << 1;
//   } while (iVar6 * 0x10000 >> 0x10 < 4);
//   return pos->vx;
// }
