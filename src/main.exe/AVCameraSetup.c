#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void AVCameraSetup(void);
 *     CHRANIM.C:289, 29 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     stack sp+16     struct SVECTOR vect
 *     reg   $a1       struct Humanoid * human
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsRVIEW2 ViewInfo;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/AVCameraSetup", AVCameraSetup);

// triage: EASY — 109 insns, 3 callees, ~0.05 to PrepareGetScreenPositionS
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void AVCameraSetup(void)
//
// {
//   Humanoid *pHVar1;
//   int iVar2;
//   short ordr;
//   SVECTOR local_10;
//
//   iVar2 = (int)*(short *)(DAT_80097cbc + 2);
//   if (iVar2 == 4) {
//     ViewInfo.vpx = *(short *)(DAT_80097cbc + 4) * 100;
//     ViewInfo.vpy = *(short *)(DAT_80097cbc + 6) * 100;
//     ViewInfo.vpz = *(short *)(DAT_80097cbc + 8) * 100;
//   }
//   else if (iVar2 < 5) {
//     if (-1 < iVar2) {
//       iVar2 = (uint)(ushort)DAT_80097cc4->rotate->vy + iVar2 * 0x400;
//       local_10.pad = (short)iVar2;
//       ordr = 3000;
//       if (*(short *)(DAT_80097cbc + 10) != 0) {
//         ordr = *(short *)(DAT_80097cbc + 10);
//       }
//       GetMoveSpeed(&local_10,(short)((uint)(iVar2 * 0x10000) >> 0x10),ordr,0);
//       ViewInfo.vpx = DAT_80097cc4->locate->vx + (int)local_10.vx;
//       ViewInfo.vpy = (DAT_80097cc4->locate->vy - (int)DAT_80097cc4->height) + 300;
//       ViewInfo.vpz = DAT_80097cc4->locate->vz + (int)local_10.vz;
//     }
//   }
//   else if (iVar2 == 5) {
//     pHVar1 = GetHumanoid(*(short *)(DAT_80097cbc + 10));
//     if (pHVar1 == (Humanoid *)0x0) {
//       return;
//     }
//     ViewInfo.vpx = pHVar1->locate->vx;
//     ViewInfo.vpy = (pHVar1->locate->vy - (int)pHVar1->height) + 300;
//     ViewInfo.vpz = pHVar1->locate->vz;
//     DAT_80097cc4 = pHVar1;
//   }
//   GsSetRefView2(&ViewInfo);
//   return;
// }
