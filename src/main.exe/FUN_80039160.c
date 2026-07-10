#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_EffectSlot EffectSlot[200];
 *     extern unsigned long *GlobalAreaMap;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80039160", FUN_80039160);

// triage: EASY — 77 insns, 1 loop, 1 callees, ~0.12 to ReqItemGoshikimai
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_80039160(undefined4 *param_1,short *param_2,long param_3,undefined1 param_4)
//
// {
//   short sVar1;
//   ulong *area;
//   long lVar2;
//   int iVar3;
//   int iVar4;
//   tag_EffectSlot *ptVar5;
//
//   iVar4 = 0;
//   ptVar5 = EffectSlot + DAT_80097a3c;
//   iVar3 = DAT_80097a3c;
//   while( true ) {
//     iVar3 = iVar3 + 1;
//     ptVar5 = ptVar5 + 1;
//     if (199 < iVar3) {
//       iVar3 = 0;
//       ptVar5 = EffectSlot;
//     }
//     iVar4 = iVar4 + 1;
//     if (ptVar5->proc == (undefined **)0x0) break;
//     if (199 < iVar4) {
//       ptVar5 = &dmy;
// LAB_800391f4:
//       (ptVar5->param).blood.hint = (AreaNodeType *)*param_1;
//       (ptVar5->param).blood.px = param_1[1];
//       (ptVar5->param).blood.py = param_1[2];
//       (ptVar5->param).blood.time = *param_2;
//       (ptVar5->param).blood.vx = param_2[1];
//       sVar1 = param_2[2];
//       *(undefined1 *)((int)&ptVar5->param + 0x1e) = param_4;
//       area = GlobalAreaMap;
//       (ptVar5->param).blood.rotate = param_3;
//       (ptVar5->param).blood.scale = (ptVar5->param).blood.px + -8000;
//       (ptVar5->param).blood.vy = sVar1;
//       lVar2 = GetAreaMapLevel(area,(ptVar5->param).bleed.pos.vx,(ptVar5->param).blood.scale);
//       (ptVar5->param).blood.pz = lVar2;
//       ptVar5->proc = (undefined **)FUN_80034dbc;
//       return;
//     }
//   }
//   DAT_80097a3c = iVar3 + 1;
//   if (199 < DAT_80097a3c) {
//     DAT_80097a3c = 0;
//   }
//   goto LAB_800391f4;
// }
