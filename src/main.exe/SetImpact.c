#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetImpact(struct VECTOR *pos, short size, short type);
 *     EFFECT.C:893, 13 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
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
 *     param $a1       short size
 *     param $a2       short type
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetImpact", SetImpact);

// triage: MEDIUM — 90 insns, mul/div, 1 loop, 1 callees, ~0.09 to AdtFntLoad
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void SetImpact(VECTOR *pos,short size,short type)
//
// {
//   int iVar1;
//   tag_EffectSlot *ptVar2;
//   long lVar3;
//   int iVar4;
//   int iVar5;
//
//   iVar1 = rand();
//   iVar5 = 0;
//   ptVar2 = EffectSlot + DAT_80097a3c;
//   iVar4 = DAT_80097a3c;
//   while( true ) {
//     iVar4 = iVar4 + 1;
//     ptVar2 = ptVar2 + 1;
//     if (199 < iVar4) {
//       iVar4 = 0;
//       ptVar2 = EffectSlot;
//     }
//     if (ptVar2->proc == (undefined **)0x0) break;
//     iVar5 = iVar5 + 1;
//     if (199 < iVar5) {
//       ptVar2 = &dmy;
// LAB_80034310:
//       ptVar2->proc = (undefined **)FUN_80033f10;
//       (ptVar2->param).blood.hint = (AreaNodeType *)pos->vx;
//       (ptVar2->param).blood.px = pos->vy;
//       lVar3 = pos->vz;
//       (ptVar2->param).blood.pz = 0;
//       (ptVar2->param).blood.vy = 0;
//       (ptVar2->param).blood.vz = (short)iVar1 + (short)(iVar1 / 0x5a) * -0x5a + 0x5a;
//       (ptVar2->param).blood.scale = 0x808080;
//       (ptVar2->param).blood.rotate = 0x808080;
//       (ptVar2->param).blood.time = size;
//       (ptVar2->param).blood.vx = 0;
//       *(undefined1 *)((int)&ptVar2->param + 0x22) = 0xf;
//       (ptVar2->param).blood.bright = '\0';
//       (ptVar2->param).blood.mode = (uchar)type;
//       (ptVar2->param).blood.py = lVar3;
//       return;
//     }
//   }
//   DAT_80097a3c = iVar4 + 1;
//   if (199 < iVar4 + 1) {
//     DAT_80097a3c = 0;
//   }
//   goto LAB_80034310;
// }
