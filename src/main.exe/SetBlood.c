#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetBlood(struct VECTOR *pos, struct SVECTOR *vect, short n, short time);
 *     EFFECT.C:745, 45 src lines, frame 64 bytes, saved-reg mask 0xc0ff0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s3       struct VECTOR * pos
 *     param $s5       struct SVECTOR * vect
 *     param $s7       short n
 *     param $fp       short time
 *     reg   $s4       short i
 *     reg   $s6       struct AreaNodeType * hint
 *     reg   $s1       struct tag_EffectSlot * slot
 *     reg   $s0       struct BloodType * blood
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned long *GlobalAreaMap;
 *     extern struct tag_EffectSlot EffectSlot[200];
 *     extern struct AreaNodeType *FieldArea;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetBlood", SetBlood);

// triage: MEDIUM — 193 insns, mul/div, 1 loop, 2 callees, ~0.08 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void SetBlood(VECTOR *pos,SVECTOR *vect,short n,short time)
//
// {
//   AreaNodeType *pAVar1;
//   int iVar2;
//   int iVar3;
//   short sVar4;
//   tag_EffectSlot *ptVar5;
//   int iVar6;
//
//   iVar6 = 0;
//   GetAreaMapLevel(GlobalAreaMap,pos->vx,pos->vy);
//   pAVar1 = FieldArea;
//   do {
//     iVar3 = 0;
//     if ((int)vect << 0x10 <= iVar6 << 0x10) {
//       return;
//     }
//     ptVar5 = EffectSlot + DAT_80097a3c;
//     iVar2 = DAT_80097a3c;
//     do {
//       iVar2 = iVar2 + 1;
//       ptVar5 = ptVar5 + 1;
//       if (199 < iVar2) {
//         iVar2 = 0;
//         ptVar5 = EffectSlot;
//       }
//       iVar3 = iVar3 + 1;
//       if (ptVar5->proc == (undefined **)0x0) {
//         DAT_80097a3c = iVar2 + 1;
//         if (199 < DAT_80097a3c) {
//           DAT_80097a3c = 0;
//         }
//         goto LAB_800332bc;
//       }
//     } while (iVar3 < 200);
//     ptVar5 = &dmy;
// LAB_800332bc:
//     iVar3 = rand();
//     *(char *)((int)&ptVar5->param + 0x22) = (char)iVar3 + (char)(iVar3 / 2) * -2;
//     iVar2 = rand();
//     iVar3 = iVar2;
//     if (iVar2 < 0) {
//       iVar3 = iVar2 + 0xfff;
//     }
//     (ptVar5->param).blood.scale = iVar2 + (iVar3 >> 0xc) * -0x1000 + 0x2000;
//     iVar3 = rand();
//     (ptVar5->param).blood.rotate = (iVar3 % 0x168) * 0x1000;
//     (ptVar5->param).blood.px = pos->vx;
//     (ptVar5->param).blood.py = pos->vy;
//     (ptVar5->param).blood.pz = pos->vz;
//     iVar3 = rand();
//     (ptVar5->param).blood.vx = (short)iVar3 + (short)(iVar3 / 0x78) * -0x78 + -0x3c;
//     iVar3 = rand();
//     (ptVar5->param).blood.vy = (short)iVar3 + (short)(iVar3 / 0x3c) * -0x3c + -0x78;
//     iVar3 = rand();
//     (ptVar5->param).blood.vz = (short)iVar3 + (short)(iVar3 / 0x78) * -0x78 + -0x3c;
//     iVar3 = (int)((uint)(ushort)n << 0x10) >> 0x10;
//     iVar2 = iVar3 - ((int)((uint)(ushort)n << 0x10) >> 0x1f) >> 1;
//     iVar3 = iVar3 - iVar2;
//     sVar4 = (short)iVar2;
//     if (iVar3 < 1) {
//       (ptVar5->param).blood.time = sVar4;
//     }
//     else {
//       iVar2 = rand();
//       if (iVar3 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar3 == -1) && (iVar2 == -0x80000000)) {
//         trap(0x1800);
//       }
//       (ptVar5->param).blood.time = (short)(iVar2 % iVar3) + sVar4;
//     }
//     iVar6 = iVar6 + 1;
//     *(undefined2 *)((int)&ptVar5->param + 0x20) = 0x80;
//     (ptVar5->param).blood.hint = pAVar1;
//     *(undefined1 *)((int)&ptVar5->param + 0x23) = 0;
//     ptVar5->proc = (undefined **)DrawBlood;
//   } while( true );
// }
