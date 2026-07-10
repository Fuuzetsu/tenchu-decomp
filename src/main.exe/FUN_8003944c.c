#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_8003944c", FUN_8003944c);

// triage: EASY — 62 insns, 1 loop, 0 callees, ~0.06 to ReqItemManebue
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_8003944c(undefined4 *param_1,long param_2,short param_3,short param_4,long param_5,
//                  long param_6,short param_7,short param_8,undefined1 param_9,uchar param_10)
//
// {
//   int iVar1;
//   long lVar2;
//   tag_EffectSlot *ptVar3;
//   int iVar4;
//
//   iVar4 = 0;
//   ptVar3 = EffectSlot + DAT_80097a3c;
//   iVar1 = DAT_80097a3c;
//   while( true ) {
//     iVar1 = iVar1 + 1;
//     ptVar3 = ptVar3 + 1;
//     if (199 < iVar1) {
//       iVar1 = 0;
//       ptVar3 = EffectSlot;
//     }
//     if (ptVar3->proc == (undefined **)0x0) break;
//     iVar4 = iVar4 + 1;
//     if (199 < iVar4) {
//       ptVar3 = &dmy;
// LAB_800394e8:
//       ptVar3->proc = (undefined **)FUN_80033f10;
//       (ptVar3->param).blood.hint = (AreaNodeType *)*param_1;
//       (ptVar3->param).blood.px = param_1[1];
//       lVar2 = param_1[2];
//       (ptVar3->param).blood.pz = param_2;
//       (ptVar3->param).blood.vy = param_7;
//       (ptVar3->param).blood.vz = param_8;
//       (ptVar3->param).blood.scale = param_5;
//       (ptVar3->param).blood.time = param_3;
//       (ptVar3->param).blood.vx = param_4;
//       *(undefined1 *)((int)&ptVar3->param + 0x22) = param_9;
//       (ptVar3->param).blood.bright = '\0';
//       (ptVar3->param).blood.mode = param_10;
//       (ptVar3->param).blood.py = lVar2;
//       (ptVar3->param).blood.rotate = param_6;
//       return;
//     }
//   }
//   DAT_80097a3c = iVar1 + 1;
//   if (199 < iVar1 + 1) {
//     DAT_80097a3c = 0;
//   }
//   goto LAB_800394e8;
// }
