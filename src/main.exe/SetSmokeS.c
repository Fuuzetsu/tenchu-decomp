#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetSmokeS(struct VECTOR *pos, short vx, short vy, short vz, int time);
 *     EFFECT.C:858, 14 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct VECTOR * pos
 *     param $a1       short vx
 *     param $a2       short vy
 *     param $a3       short vz
 *     param stack+16  int time
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetSmokeS", SetSmokeS);

// triage: MEDIUM — 124 insns, mul/div, 1 loop, 1 callees, ~0.09 to ReqItemNingyo
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void SetSmokeS(long *param_1,short param_2,short param_3,short param_4,ushort param_5)
//
// {
//   int iVar1;
//   int iVar2;
//   tag_EffectSlot *ptVar3;
//
//   iVar2 = 0;
//   ptVar3 = EffectSlot + DAT_80097a3c;
//   iVar1 = DAT_80097a3c;
//   while( true ) {
//     iVar1 = iVar1 + 1;
//     ptVar3 = ptVar3 + 1;
//     if (199 < iVar1) {
//       iVar1 = 0;
//       ptVar3 = EffectSlot;
//     }
//     iVar2 = iVar2 + 1;
//     if (ptVar3->proc == (undefined **)0x0) break;
//     if (199 < iVar2) {
//       ptVar3 = &dmy;
// LAB_80033a8c:
//       iVar2 = rand();
//       iVar1 = iVar2;
//       if (iVar2 < 0) {
//         iVar1 = iVar2 + 0x1fff;
//       }
//       (ptVar3->param).smoke.scale = iVar2 + (iVar1 >> 0xd) * -0x2000 + 0x1000;
//       iVar1 = rand();
//       (ptVar3->param).smoke.rotate = (iVar1 % 0x168) * 0x1000;
//       (ptVar3->param).blood.py = *param_1;
//       (ptVar3->param).blood.pz = param_1[1];
//       (ptVar3->param).blood.scale = param_1[2];
//       (ptVar3->param).smoke.vec.vx = param_2;
//       (ptVar3->param).smoke.vec.vy = param_3;
//       (ptVar3->param).smoke.vec.vz = param_4;
//       (ptVar3->param).blood.mode = (uchar)param_5;
//       iVar1 = rand();
//       iVar2 = (int)((uint)param_5 << 0x10) >> 0x10;
//       if (iVar2 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar2 == -1) && (iVar1 == -0x80000000)) {
//         trap(0x1800);
//       }
//       *(undefined1 *)((int)&ptVar3->param + 0x22) = 0;
//       (ptVar3->param).blood.bright =
//            ((ptVar3->param).blood.mode + 0xff) -
//            ((char)(iVar2 - ((int)((uint)param_5 << 0x10) >> 0x1f) >> 1) + (char)(iVar1 % iVar2));
//       ptVar3->proc = (undefined **)DrawSmoke;
//       return;
//     }
//   }
//   DAT_80097a3c = iVar1 + 1;
//   if (199 < DAT_80097a3c) {
//     DAT_80097a3c = 0;
//   }
//   goto LAB_80033a8c;
// }
