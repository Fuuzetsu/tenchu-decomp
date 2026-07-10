#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void EndDrawing(short sync);
 *     3DCTRL.C:151, 48 src lines, frame 24 bytes, saved-reg mask 0x80010000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     param $a0       short sync
 *
 * Globals it touches, as the original declared them:
 *     extern long GameClock;
 *     extern short SkipFrame;
 *     extern struct GsOT OTable[2];
 *     extern short DrawingPage;
 *     extern struct GsOT *OTablePt;
 *     extern struct GsFOGPARAM Fog;
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/EndDrawing", EndDrawing);

// triage: MEDIUM — 134 insns, mul/div, 7 callees, ~0.04 to ProcItemDrop
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - Expressions: mult/div — magic-multiply constants, fold
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// void EndDrawing(short sync)
//
// {
//   PACKET *pPVar1;
//   int iVar2;
//   uint uVar3;
//   int iVar4;
//
//   if ((GameClock == (GameClock / 0x1e) * 0x1e) && (SkipFrame == 0)) {
//     pPVar1 = GsGetWorkBase();
//     DAT_800976b8 = pPVar1 + DrawingPage * -0x10000 + 0x7ff67fc0;
//     if ((PACKET *)0x10000 < DAT_800976b8) {
//       DAT_800976b8 = (PACKET *)0x10000;
//     }
//   }
//   if (SkipFrame == 1) {
//     uVar3 = (uint)(ushort)DrawingPage;
//     sync = (short)(((uint)(ushort)sync << 0x10) >> 0xf);
//     SkipFrame = 0;
//     DrawingPage = (short)(1 - uVar3);
//     OTablePt = OTable + ((int)((1 - uVar3) * 0x10000) >> 0x10);
//   }
//   else if (SkipFrame < 2) {
//     if ((SkipFrame == 0) && (iVar4 = VSync(1), sync * -0xf0 + -10 < iVar4)) {
//       SkipFrame = 1;
//       return;
//     }
//   }
//   else if (SkipFrame == 2) {
//     SkipFrame = 0;
//   }
//   OTablePt->org[0x7fe] = OTablePt->org[0x4e2];
//   iVar4 = (int)sync;
//   if (iVar4 < 1) {
//     DrawSync(0);
//     VSync(-iVar4);
//   }
//   else {
//     iVar2 = VSync(-1);
//     if (iVar2 - time < iVar4) {
//       VSync(iVar4);
//     }
//     time = VSync(-1);
//     ResetGraph(1);
//   }
//   GsSwapDispBuff();
//   GsSortClear(Fog.rfc,Fog.gfc,Fog.bfc,OTablePt);
//   GsDrawOt(OTablePt);
//   return;
// }
