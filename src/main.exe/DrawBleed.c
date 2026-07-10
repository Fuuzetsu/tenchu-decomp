#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawBleed(struct tag_EffectSlot *ef);
 *     EFFECT.C:910, 34 src lines, frame 40 bytes, saved-reg mask 0x80030000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       struct tag_EffectSlot * ef
 *     reg   $s1       struct BleedType * param
 *     stack sp+16     struct SVECTOR scr
 *     reg   $v0       long x
 *     reg   $a1       long y
 *     reg   $a2       long z
 *     reg   $s0       struct SVECTOR * scr
 *     reg   $v1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
 *     extern struct POLY_F4 plyBleed;
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DrawBleed", DrawBleed);

// triage: MEDIUM — 133 insns, mul/div, 4 callees, ~0.06 to FUN_800396c0
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Removing unreachable block (ram,0x800344dc) */
//
// void DrawBleed(tag_EffectSlot *ef)
//
// {
//   short sVar1;
//   short sVar2;
//   long lVar3;
//   int iVar4;
//   int iVar5;
//   short local_18;
//   short local_16;
//   undefined2 local_14;
//
//   if ((ef->param).bleed.mode == '\0') {
//     if ((ef->param).bleed.time == '\0') {
//       ef->proc = (undefined **)0x0;
//     }
//     else {
//       (ef->param).blood.hint =
//            (AreaNodeType *)((int)&((ef->param).blood.hint)->y + (int)(ef->param).bleed.vec.vx);
//       sVar2 = (ef->param).bleed.vec.vz;
//       (ef->param).blood.px = (ef->param).blood.px + (int)(ef->param).bleed.vec.vy;
//       sVar1 = (ef->param).bleed.vec.vy;
//       (ef->param).blood.py = (ef->param).blood.py + (int)sVar2;
//       (ef->param).bleed.vec.vy = sVar1 + 1;
//     }
//   }
//   (ef->param).bleed.time = (ef->param).bleed.time + 0xff;
//   Scratchpad[0x14] = 0;
//   Scratchpad[0x15] = 0;
//   Scratchpad[0x16] = 0;
//   Scratchpad[0x17] = 0;
//   Scratchpad[0x18] = 0;
//   Scratchpad[0x19] = 0;
//   Scratchpad[0x1a] = 0;
//   Scratchpad[0x1b] = 0;
//   Scratchpad[0x1c] = 0;
//   Scratchpad[0x1d] = 0;
//   Scratchpad[0x1e] = 0;
//   Scratchpad[0x1f] = 0;
//   Scratchpad._32_2_ = (short)(ef->param).bleed.pos.vx - (short)ViewInfo.vpx;
//   Scratchpad._34_2_ = (short)(ef->param).blood.px - (short)ViewInfo.vpy;
//   Scratchpad._36_2_ = (short)(ef->param).blood.py - (short)ViewInfo.vpz;
//   SetTransMatrix((MATRIX *)Scratchpad);
//   SetRotMatrix(&GsWSMATRIX);
//   lVar3 = RotTransPers((SVECTOR *)(Scratchpad + 0x20),(long *)&local_18,(long *)(Scratchpad + 0x28),
//                        (long *)(Scratchpad + 0x2c));
//   local_14 = (undefined2)lVar3;
//   iVar4 = (lVar3 << 0x10) >> 0x10;
//   if (0x24 < iVar4) {
//     if (iVar4 == 0) {
//       trap(0x1c00);
//     }
//     plyBleed.x0 = local_18;
//     plyBleed.y0 = local_16;
//     plyBleed.y1 = local_16;
//     plyBleed.x2 = local_18;
//     sVar2 = (short)(900 / iVar4) + 1;
//     plyBleed.x1 = local_18 + sVar2;
//     plyBleed.y2 = local_16 + sVar2;
//     plyBleed.r0 = (ef->param).bleed.r;
//     plyBleed.g0 = (ef->param).bleed.g;
//     plyBleed.b0 = (ef->param).bleed.b;
//     iVar4 = (lVar3 << 0x10) >> 0x12;
//     if (iVar4 < 0) {
//       iVar5 = 0;
//     }
//     else {
//       iVar5 = 0x4e1;
//       if (iVar4 < 0x4e2) {
//         iVar5 = iVar4;
//       }
//     }
//     plyBleed.x3 = plyBleed.x1;
//     plyBleed.y3 = plyBleed.y2;
//     GsSortPoly(&plyBleed,OTablePt,(ushort)iVar5);
//   }
//   return;
// }
