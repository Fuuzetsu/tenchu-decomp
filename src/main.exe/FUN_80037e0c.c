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

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80037e0c", FUN_80037e0c);

// triage: MEDIUM — 224 insns, mul/div, 1 loop, 6 callees, ~0.11 to ProcItemKusuri
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_80037e0c(Humanoid *param_1,int param_2)
//
// {
//   int iVar1;
//   VECTOR *pos;
//   int iVar2;
//   int iVar3;
//   tag_EffectSlot *ptVar4;
//   ModelType **ppMVar5;
//   ModelType *model;
//   undefined1 local_38 [20];
//   Humanoid *local_24;
//   int local_20;
//   int local_1c;
//   int local_18;
//
//   if (param_2 == 0) {
//     ppMVar5 = param_1->model->object;
//     if (0 < param_1->model->n) {
//       iVar1 = rand();
//       iVar2 = (int)param_1->model->n;
//       if (iVar2 == 0) {
//         trap(0x1c00);
//       }
//       if ((iVar2 == -1) && (iVar1 == -0x80000000)) {
//         trap(0x1800);
//       }
//       ppMVar5 = ppMVar5 + iVar1 % iVar2;
//     }
//     model = *ppMVar5;
//     memset(local_38 + 0x10,'\0',0x10);
//     iVar1 = rand();
//     local_38._16_4_ = iVar1 % 200 - 100;
//     iVar1 = rand();
//     local_24 = (Humanoid *)(iVar1 % 200 + -100);
//     iVar1 = rand();
//     local_38._8_4_ = iVar1 % 200 + -100;
//     local_38._0_4_ = local_38._16_4_;
//     local_38._4_4_ = local_24;
//     local_38._12_4_ = local_1c;
//     local_38._16_4_ = DAT_80097a58;
//     local_24 = (Humanoid *)DAT_80097a5c;
//     local_20 = local_38._8_4_;
//     iVar2 = rand();
//     iVar3 = 0;
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
//         goto LAB_800380f0;
//       }
//     } while (iVar3 < 200);
//     ptVar4 = &dmy;
// LAB_800380f0:
//     (ptVar4->param).blood.px = local_38._0_4_;
//     (ptVar4->param).blood.py = local_38._4_4_;
//     (ptVar4->param).bleed.vec.vx = 0x3000;
//     (ptVar4->param).frame.mode = '\0';
//     (ptVar4->param).bleed.vec.vy = (short)iVar2 + (short)(iVar2 / 0x3c) * -0x3c + 0x3c;
//     (ptVar4->param).blood.pz = local_38._8_4_;
//     (ptVar4->param).blood.hint = (AreaNodeType *)model;
//     ptVar4->proc = (undefined **)DrawFrame;
//     pos = GetAbsolutePosition(model,0,0,0);
//     SetBleedsDir(pos,(SVECTOR *)(local_38 + 0x10),100,10);
//     SoundEx((VECTOR *)(param_1->model->locate).coord.t,0x39);
//   }
//   else {
//     local_38._0_4_ = ITEM_NAPALM;
//     local_38._8_4_ = (param_1->model->locate).coord.t[0];
//     local_1c = (param_1->model->locate).coord.t[1];
//     local_38._16_4_ = (param_1->model->locate).coord.t[2];
//     local_38._12_4_ = local_1c + -100;
//     local_20 = local_38._8_4_ + (int)(param_1->vector).vx;
//     local_1c = local_1c + -0x73;
//     local_18 = local_38._16_4_ + (int)(param_1->vector).vz;
//     local_38._4_4_ = param_1;
//     ReqItemUse((PARAM_ITEM_USE *)local_38);
//   }
//   return;
// }
