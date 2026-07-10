#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 *     extern long GameClock;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80038fdc", FUN_80038fdc);

// triage: EASY — 48 insns, 1 loop, 0 callees, ~0.04 to FUN_8004a42c
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_80038fdc(undefined1 param_1,undefined1 param_2,undefined1 param_3,long param_4)
//
// {
//   long lVar1;
//   int iVar2;
//   tag_EffectSlot *ptVar3;
//   int iVar4;
//
//   iVar4 = 0;
//   ptVar3 = EffectSlot + DAT_80097a3c;
//   iVar2 = DAT_80097a3c;
//   while( true ) {
//     iVar2 = iVar2 + 1;
//     ptVar3 = ptVar3 + 1;
//     if (199 < iVar2) {
//       iVar2 = 0;
//       ptVar3 = EffectSlot;
//     }
//     if (ptVar3->proc == (undefined **)0x0) break;
//     iVar4 = iVar4 + 1;
//     if (199 < iVar4) {
//       ptVar3 = &dmy;
// LAB_80039064:
//       *(undefined1 *)&ptVar3->param = param_1;
//       *(undefined1 *)((int)&ptVar3->param + 1) = param_2;
//       *(undefined1 *)((int)&ptVar3->param + 2) = param_3;
//       (ptVar3->param).splash.speed = '\0';
//       lVar1 = GameClock;
//       (ptVar3->param).blood.px = param_4;
//       (ptVar3->param).blood.py = lVar1;
//       (ptVar3->param).blood.pz = lVar1 + 5;
//       ptVar3->proc = (undefined **)FUN_80036284;
//       return;
//     }
//   }
//   DAT_80097a3c = iVar2 + 1;
//   if (199 < iVar2 + 1) {
//     DAT_80097a3c = 0;
//   }
//   goto LAB_80039064;
// }
