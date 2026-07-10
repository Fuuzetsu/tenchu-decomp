#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void DrawSplash(struct tag_EffectSlot *ef);
 *     EFFECT.C:978, 43 src lines, frame 88 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
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
 *     param $s3       struct tag_EffectSlot * ef
 *     reg   $s1       struct SplashType * param
 *     reg   $s2       struct GsSPRITE * spr
 *     stack sp+24     struct SVECTOR scr
 *     reg   $v0       long x
 *     reg   $a1       long y
 *     reg   $a2       long z
 *     reg   $s0       struct SVECTOR * scr
 *     stack sp+32     struct VECTOR pos
 *     reg   $v1       int z
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
 *     extern struct GsOT *OTablePt;
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/DrawSplash", DrawSplash);

// triage: MEDIUM — 246 insns, mul/div, 6 callees, ~0.06 to InsertConflict
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void DrawSplash(tag_EffectSlot *ef)
//
// {
//   byte bVar1;
//   long lVar2;
//   int iVar3;
//   uint uVar4;
//   int iVar5;
//   int iVar6;
//   short local_40;
//   short local_3e;
//   ushort local_3c;
//   VECTOR local_38;
//   SVECTOR local_28;
//   long local_20;
//   long local_1c;
//
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
//   lVar2 = RotTransPers((SVECTOR *)(Scratchpad + 0x20),(long *)&local_40,(long *)(Scratchpad + 0x28),
//                        (long *)(Scratchpad + 0x2c));
//   local_3c = (ushort)lVar2;
//   iVar5 = (int)(short)local_3c;
//   if (iVar5 < 0x25) {
//     return;
//   }
//   sprSplash.x = local_40;
//   sprSplash.y = local_3e;
//   iVar3 = (ef->param).splash.sx * 300;
//   if (iVar5 == 0) {
//     trap(0x1c00);
//   }
//   if ((iVar5 == -1) && (iVar3 == -0x80000000)) {
//     trap(0x1800);
//   }
//   iVar6 = iVar3 / iVar5 + 1;
//   sprSplash.scalex = (short)iVar6;
//   iVar3 = (ef->param).splash.sy * 300;
//   if (iVar5 == 0) {
//     trap(0x1c00);
//   }
//   if ((iVar5 == -1) && (iVar3 == -0x80000000)) {
//     trap(0x1800);
//   }
//   iVar5 = iVar3 / iVar5 + 1;
//   sprSplash.scaley = (short)iVar5;
//   bVar1 = (ef->param).splash.mode;
//   if (bVar1 != 1) {
//     if (1 < bVar1) {
//       if (bVar1 == 2) {
//         uVar4 = (uint)(ef->param).splash.speed;
//         iVar5 = (iVar5 * 0x10000 >> 0x10) * (uVar4 - (ef->param).splash.count);
//         if (uVar4 == 0) {
//           trap(0x1c00);
//         }
//         if ((uVar4 == 0xffffffff) && (iVar5 == -0x80000000)) {
//           trap(0x1800);
//         }
//         iVar6 = iVar6 * 0x10000;
//         sprSplash.scalex = (short)((iVar6 >> 0x10) - (iVar6 >> 0x1f) >> 1);
//         sprSplash.scaley = (short)(iVar5 / (int)uVar4);
//         bVar1 = (ef->param).splash.count + 1;
//         (ef->param).splash.count = bVar1;
//         if ((ef->param).splash.speed <= bVar1) {
//           ef->proc = (undefined **)0x0;
//         }
//       }
//       goto LAB_80034d60;
//     }
//     if (bVar1 != 0) goto LAB_80034d60;
//     (ef->param).splash.count = '\0';
//     (ef->param).splash.mode = (ef->param).splash.mode + '\x01';
//     memset((uchar *)&local_28,'\0',0x10);
//     local_38.vx = (long)(ef->param).blood.hint;
//     local_38.vy = (ef->param).blood.px;
//     local_38.vz = (ef->param).blood.py;
//     local_38.pad = local_1c;
//     local_28.vx = (short)DAT_80097a50;
//     local_28.vy = DAT_80097a50._2_2_;
//     local_28.vz = (short)DAT_80097a54;
//     local_28.pad = DAT_80097a54._2_2_;
//     local_20 = local_38.vz;
//     SetBleedsDir(&local_38,&local_28,100,6);
//   }
//   iVar5 = (int)sprSplash.scaley * (uint)(ef->param).splash.count;
//   uVar4 = (uint)(ef->param).splash.speed;
//   if (uVar4 == 0) {
//     trap(0x1c00);
//   }
//   if ((uVar4 == 0xffffffff) && (iVar5 == -0x80000000)) {
//     trap(0x1800);
//   }
//   sprSplash.scaley = (short)(iVar5 / (int)uVar4);
//   bVar1 = (ef->param).splash.count + 1;
//   (ef->param).splash.count = bVar1;
//   if ((ef->param).splash.speed <= bVar1) {
//     (ef->param).splash.count = '\0';
//     (ef->param).splash.mode = (ef->param).splash.mode + '\x01';
//   }
// LAB_80034d60:
//   iVar5 = (int)((uint)local_3c << 0x10) >> 0x12;
//   if (iVar5 < 0) {
//     iVar3 = 0;
//   }
//   else {
//     iVar3 = 0x4e1;
//     if (iVar5 < 0x4e2) {
//       iVar3 = iVar5;
//     }
//   }
//   GsSortSprite(&sprSplash,OTablePt,(ushort)iVar3);
//   return;
// }
