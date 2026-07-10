#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void SetExplosion(struct VECTOR *pos, struct SVECTOR *vect);
 *     EFFECT.C:1236, 18 src lines, frame 48 bytes, saved-reg mask 0x800f0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $s2       struct VECTOR * pos
 *     param $s3       struct SVECTOR * vect
 *     reg   $s1       struct tag_EffectSlot * slot
 *     reg   $s0       struct ExplosionType * param
 *     reg   $v1       int i
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/SetExplosion", SetExplosion);

// triage: MEDIUM — 96 insns, mul/div, 1 loop, 2 callees, ~0.07 to AdtFntOpen
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void SetExplosion(VECTOR *pos,SVECTOR *vect)
//
// {
//   short sVar1;
//   int iVar2;
//   int iVar3;
//   tag_EffectSlot *ptVar4;
//
//   iVar3 = 0;
//   ptVar4 = EffectSlot + DAT_80097a3c;
//   iVar2 = DAT_80097a3c;
//   while( true ) {
//     iVar2 = iVar2 + 1;
//     ptVar4 = ptVar4 + 1;
//     if (199 < iVar2) {
//       iVar2 = 0;
//       ptVar4 = EffectSlot;
//     }
//     iVar3 = iVar3 + 1;
//     if (ptVar4->proc == (undefined **)0x0) break;
//     if (199 < iVar3) {
//       ptVar4 = &dmy;
// LAB_800367d8:
//       (ptVar4->param).smoke.scale = 0x1000;
//       iVar2 = rand();
//       (ptVar4->param).smoke.rotate = (iVar2 % 0x168) * 0x1000;
//       (ptVar4->param).blood.py = pos->vx;
//       (ptVar4->param).blood.pz = pos->vy;
//       (ptVar4->param).blood.scale = pos->vz;
//       (ptVar4->param).smoke.vec.vx = vect->vx;
//       (ptVar4->param).smoke.vec.vy = vect->vy;
//       sVar1 = vect->vz;
//       (ptVar4->param).blood.mode = '\x05';
//       (ptVar4->param).blood.bright = '\0';
//       (ptVar4->param).smoke.vec.vz = sVar1;
//       ptVar4->proc = (undefined **)DrawExplosion;
//       SetBleeds((short)pos,200,0x96);
//       return;
//     }
//   }
//   DAT_80097a3c = iVar2 + 1;
//   if (199 < DAT_80097a3c) {
//     DAT_80097a3c = 0;
//   }
//   goto LAB_800367d8;
// }
