#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetSmoke(struct VECTOR *pos, struct SVECTOR *vect, short n, short time);
 *     EFFECT.C:835, 21 src lines, frame 56 bytes, saved-reg mask 0x80ff0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $s5       struct VECTOR * pos
 *     param $s6       struct SVECTOR * vect
 *     param $s7       short n
 *     param $s3       short time
 *     reg   $s4       short i
 *     reg   $s1       struct tag_EffectSlot * slot
 *     reg   $s0       struct SmokeType * smoke
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetSmoke", SetSmoke);

// triage: MEDIUM — 197 insns, mul/div, 1 loop, 1 callees, ~0.09 to ReqItemNingyo
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void SetSmoke(VECTOR *pos,SVECTOR *vect,short n,short time)
//
// {
//   int iVar1;
//   int iVar2;
//   tag_EffectSlot *ptVar3;
//   int iVar4;
//
//   iVar4 = 0;
//   do {
//     iVar2 = 0;
//     if ((int)((uint)(ushort)n << 0x10) <= iVar4 << 0x10) {
//       return;
//     }
//     ptVar3 = EffectSlot + DAT_80097a3c;
//     iVar1 = DAT_80097a3c;
//     do {
//       iVar1 = iVar1 + 1;
//       ptVar3 = ptVar3 + 1;
//       if (199 < iVar1) {
//         iVar1 = 0;
//         ptVar3 = EffectSlot;
//       }
//       iVar2 = iVar2 + 1;
//       if (ptVar3->proc == (undefined **)0x0) {
//         DAT_80097a3c = iVar1 + 1;
//         if (199 < DAT_80097a3c) {
//           DAT_80097a3c = 0;
//         }
//         goto LAB_8003379c;
//       }
//     } while (iVar2 < 200);
//     ptVar3 = &dmy;
// LAB_8003379c:
//     iVar1 = rand();
//     iVar2 = iVar1;
//     if (iVar1 < 0) {
//       iVar2 = iVar1 + 0x1fff;
//     }
//     (ptVar3->param).smoke.scale = iVar1 + (iVar2 >> 0xd) * -0x2000 + 0x1000;
//     iVar2 = rand();
//     (ptVar3->param).smoke.rotate = (iVar2 % 0x168) * 0x1000;
//     (ptVar3->param).blood.py = pos->vx;
//     (ptVar3->param).blood.pz = pos->vy;
//     (ptVar3->param).blood.scale = pos->vz;
//     iVar2 = rand();
//     (ptVar3->param).smoke.vec.vx = vect->vx + -0x32 + (short)iVar2 + (short)(iVar2 / 100) * -100;
//     iVar2 = rand();
//     (ptVar3->param).smoke.vec.vy = vect->vy + -0x32 + (short)iVar2 + (short)(iVar2 / 100) * -100;
//     iVar2 = rand();
//     (ptVar3->param).smoke.vec.vz = vect->vz + -0x32 + (short)iVar2 + (short)(iVar2 / 100) * -100;
//     iVar2 = rand();
//     (ptVar3->param).blood.mode = (char)time + (char)iVar2 + (char)(iVar2 / 0xa0) * '`';
//     iVar2 = rand();
//     iVar1 = (int)((uint)(ushort)time << 0x10) >> 0x10;
//     if (iVar1 == 0) {
//       trap(0x1c00);
//     }
//     if ((iVar1 == -1) && (iVar2 == -0x80000000)) {
//       trap(0x1800);
//     }
//     iVar4 = iVar4 + 1;
//     *(undefined1 *)((int)&ptVar3->param + 0x22) = 0;
//     (ptVar3->param).blood.bright =
//          ((ptVar3->param).blood.mode + 0xff) -
//          ((char)(iVar1 - ((int)((uint)(ushort)time << 0x10) >> 0x1f) >> 1) + (char)(iVar2 % iVar1));
//     ptVar3->proc = (undefined **)DrawSmoke;
//   } while( true );
// }
