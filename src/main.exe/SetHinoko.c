#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetHinoko(struct VECTOR *pos, struct SVECTOR *power, int n);
 *     EFFECT.C:1321, 21 src lines, frame 48 bytes, saved-reg mask 0x807f0000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $s3       struct VECTOR * pos
 *     param $s4       struct SVECTOR * power
 *     param $s5       int n
 *     reg   $s2       short i
 *     reg   $s1       struct tag_EffectSlot * slot
 *     reg   $s0       struct ExplosionType * param
 *     reg   $a0       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetHinoko", SetHinoko);

// triage: MEDIUM — 181 insns, mul/div, 1 loop, 1 callees, ~0.10 to ReqItemNingyo
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void SetHinoko(VECTOR *pos,SVECTOR *power,int n)
//
// {
//   int iVar1;
//   int iVar2;
//   int iVar3;
//   tag_EffectSlot *ptVar4;
//   short sVar5;
//
//   sVar5 = 0;
//   do {
//     iVar3 = 0;
//     if (n <= sVar5) {
//       return;
//     }
//     ptVar4 = EffectSlot + DAT_80097a3c;
//     iVar1 = DAT_80097a3c;
//     do {
//       iVar1 = iVar1 + 1;
//       ptVar4 = ptVar4 + 1;
//       if (199 < iVar1) {
//         iVar1 = 0;
//         ptVar4 = EffectSlot;
//       }
//       iVar3 = iVar3 + 1;
//       if (ptVar4->proc == (undefined **)0x0) {
//         DAT_80097a3c = iVar1 + 1;
//         if (199 < DAT_80097a3c) {
//           DAT_80097a3c = 0;
//         }
//         goto LAB_80036ab0;
//       }
//     } while (iVar3 < 200);
//     ptVar4 = &dmy;
// LAB_80036ab0:
//     iVar1 = rand();
//     iVar3 = iVar1;
//     if (iVar1 < 0) {
//       iVar3 = iVar1 + 0xfff;
//     }
//     (ptVar4->param).smoke.scale = iVar1 + (iVar3 >> 0xc) * -0x1000 + 0x1000;
//     iVar3 = rand();
//     (ptVar4->param).smoke.rotate = (iVar3 % 0x168) * 0x1000;
//     (ptVar4->param).blood.py = pos->vx;
//     (ptVar4->param).blood.pz = pos->vy;
//     (ptVar4->param).blood.scale = pos->vz;
//     iVar3 = rand();
//     iVar2 = (uint)(ushort)power->vx << 0x10;
//     iVar1 = iVar2 >> 0x10;
//     if (iVar1 == 0) {
//       trap(0x1c00);
//     }
//     if ((iVar1 == -1) && (iVar3 == -0x80000000)) {
//       trap(0x1800);
//     }
//     (ptVar4->param).smoke.vec.vx = (short)(iVar3 % iVar1) - (short)(iVar1 - (iVar2 >> 0x1f) >> 1);
//     iVar3 = rand();
//     iVar2 = (uint)(ushort)power->vy << 0x10;
//     iVar1 = iVar2 >> 0x10;
//     if (iVar1 == 0) {
//       trap(0x1c00);
//     }
//     if ((iVar1 == -1) && (iVar3 == -0x80000000)) {
//       trap(0x1800);
//     }
//     (ptVar4->param).smoke.vec.vy = -((short)(iVar3 % iVar1) + (short)(iVar1 - (iVar2 >> 0x1f) >> 1))
//     ;
//     iVar3 = rand();
//     iVar2 = (uint)(ushort)power->vz << 0x10;
//     iVar1 = iVar2 >> 0x10;
//     if (iVar1 == 0) {
//       trap(0x1c00);
//     }
//     if ((iVar1 == -1) && (iVar3 == -0x80000000)) {
//       trap(0x1800);
//     }
//     (ptVar4->param).smoke.vec.vz = (short)(iVar3 % iVar1) - (short)(iVar1 - (iVar2 >> 0x1f) >> 1);
//     iVar3 = rand();
//     sVar5 = sVar5 + 1;
//     (ptVar4->param).blood.bright = '\0';
//     (ptVar4->param).blood.mode = (char)iVar3 + (char)(iVar3 / 0xf) * -0xf + '\x0f';
//     ptVar4->proc = (undefined **)DrawHinoko;
//   } while( true );
// }
