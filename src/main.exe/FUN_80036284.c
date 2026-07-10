#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern struct GsOT *OTablePt;
 *
 * PSX.SYM suggests this may be `SetGore` (LOW confidence, EFFECT.C) — NOT
 * adopted. Corroborate with `tools/callmatch.py --verify` before renaming.
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/FUN_80036284", FUN_80036284);

// triage: MEDIUM — 182 insns, mul/div, 4 callees, ~0.07 to FUN_80038c0c
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void FUN_80036284(undefined4 *param_1)
//
// {
//   byte bVar1;
//   char cVar2;
//   GsOT *pGVar3;
//   POLY_XF4 *ply;
//   int iVar4;
//   uint uVar5;
//   POLY_XF4 local_30;
//
//   SetPolyXF4(&local_30,1);
//   local_30.ply.x0 = -0xa0;
//   local_30.ply.y0 = -0x78;
//   local_30.ply.x1 = 0xa0;
//   local_30.ply.y1 = -0x78;
//   local_30.ply.x2 = -0xa0;
//   local_30.ply.y2 = 0x78;
//   local_30.ply.x3 = 0xa0;
//   local_30.ply.y3 = 0x78;
//   bVar1 = *(byte *)(param_1 + 5);
//   iVar4 = GameClock - param_1[3];
//   uVar5 = param_1[4] - param_1[3];
//   if (bVar1 == 1) {
//     local_30.ply._4_3_ = *(undefined3 *)(param_1 + 1);
//     if ((uint)GameClock < (uint)param_1[4]) goto LAB_800364d4;
//     cVar2 = *(char *)(param_1 + 5);
//     iVar4 = param_1[4] + 0x28;
//   }
//   else {
//     if (1 < bVar1) {
//       if (bVar1 == 2) {
//         iVar4 = uVar5 - iVar4;
//         if ((uint)param_1[4] <= (uint)GameClock) {
//           *param_1 = 0;
//           return;
//         }
//         if (uVar5 == 0) {
//           trap(0x1c00);
//         }
//         if (uVar5 == 0) {
//           trap(0x1c00);
//         }
//         if (uVar5 == 0) {
//           trap(0x1c00);
//         }
//         local_30.ply.g0 = (u_char)((iVar4 * (uint)*(byte *)((int)param_1 + 5)) / uVar5);
//         local_30.ply.r0 = (u_char)((iVar4 * (uint)*(byte *)(param_1 + 1)) / uVar5);
//         local_30.ply.b0 = (u_char)((iVar4 * (uint)*(byte *)((int)param_1 + 6)) / uVar5);
//       }
//       goto LAB_800364d4;
//     }
//     if (bVar1 != 0) goto LAB_800364d4;
//     if (uVar5 == 0) {
//       trap(0x1c00);
//     }
//     if (uVar5 == 0) {
//       trap(0x1c00);
//     }
//     if (uVar5 == 0) {
//       trap(0x1c00);
//     }
//     local_30.ply.g0 = (u_char)((iVar4 * (uint)*(byte *)((int)param_1 + 5)) / uVar5);
//     local_30.ply.r0 = (u_char)((iVar4 * (uint)*(byte *)(param_1 + 1)) / uVar5);
//     local_30.ply.b0 = (u_char)((iVar4 * (uint)*(byte *)((int)param_1 + 6)) / uVar5);
//     if ((uint)GameClock < (uint)param_1[4]) goto LAB_800364d4;
//     cVar2 = *(char *)(param_1 + 5);
//     iVar4 = param_1[4] + 3;
//   }
//   *(char *)(param_1 + 5) = cVar2 + '\x01';
//   param_1[4] = iVar4;
// LAB_800364d4:
//   ply = (POLY_XF4 *)GsGetWorkBase();
//   GsSetWorkBase(ply + 1);
//   pGVar3 = OTablePt;
//   (ply->tpage).tag = local_30.tpage.tag;
//   (ply->tpage).code[0] = local_30.tpage.code[0];
//   (ply->ply).tag = local_30.ply.tag;
//   (ply->ply).r0 = local_30.ply.r0;
//   (ply->ply).g0 = local_30.ply.g0;
//   (ply->ply).b0 = local_30.ply.b0;
//   (ply->ply).code = local_30.ply.code;
//   (ply->ply).x0 = local_30.ply.x0;
//   (ply->ply).y0 = local_30.ply.y0;
//   (ply->ply).x1 = local_30.ply.x1;
//   (ply->ply).y1 = local_30.ply.y1;
//   (ply->ply).x2 = local_30.ply.x2;
//   (ply->ply).y2 = local_30.ply.y2;
//   (ply->ply).x3 = local_30.ply.x3;
//   (ply->ply).y3 = local_30.ply.y3;
//   AddXF4((undefined *)(pGVar3->org + param_1[2]),ply);
//   return;
// }
