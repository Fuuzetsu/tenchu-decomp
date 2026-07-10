#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * static void PutMap(void);
 *     INFOVIEW.C:1322, 65 src lines, frame 32 bytes, saved-reg mask 0x80070000 (DEMO build -- see below)
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
 *     reg   $s2       int rgb
 *     reg   $s1       struct POLY_XF4 * ply
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char PutMapMode;
 *     extern struct GsSPRITE MapImage;
 *     extern struct GsOT *OTablePt;
 *     extern struct tag_TItem items[30];
 *     extern int StageID;
 *     extern struct WorldType WorldMap[8][8][8];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/PutMap", PutMap);

// triage: MEDIUM — 133 insns, 7 callees, ~0.07 to FUN_80038c0c
// likely-relevant cookbook sections:
//   - Dispatch: if/switch ladder — reload vs CSE, signed vs unsigned
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void PutMap(void)
//
// {
//   int iVar1;
//   POLY_XF4 *ply;
//   int iVar2;
//   u_char uVar3;
//
//   ply = (POLY_XF4 *)GsGetWorkBase();
//   GsSetWorkBase(ply + 1);
//   SetPolyXF4(ply,2);
//   (ply->ply).y0 = -0x78;
//   (ply->ply).y1 = -0x78;
//   (ply->ply).y2 = 0x78;
//   (ply->ply).y3 = 0x78;
//   iVar1 = _DAT_80097f6c;
//   (ply->ply).x0 = -0xa0;
//   (ply->ply).x1 = 0xa0;
//   (ply->ply).x2 = -0xa0;
//   iVar2 = -iVar1 + 0xa0;
//   (ply->ply).x3 = 0xa0;
//   if (iVar2 < 0) {
//     iVar2 = -iVar1 + 0xa3;
//   }
//   uVar3 = (u_char)(iVar2 >> 2);
//   if (PutMapMode == 1) {
//     MapImage.r = '<';
//     MapImage.g = '<';
//     MapImage.b = '<';
//     MapImage.scalex = 0x1000;
//     MapImage.scaley = 0x1000;
//     MapImage.x = DAT_80097f6c;
//     MapImage.y = DAT_80097f70;
//     GsSortSprite(&MapImage,OTablePt,2);
//     MapImage.r = 0x80;
//     MapImage.g = 0x80;
//     MapImage.b = 0x80;
//     (ply->ply).r0 = uVar3;
//     (ply->ply).g0 = uVar3;
//     (ply->ply).b0 = uVar3;
//     _DAT_80097f6c = _DAT_80097f6c + -0x28;
//     if (_DAT_80097f6c < 1) {
//       PutMapMode = PutMapMode + 1;
//     }
//   }
//   else if (PutMapMode < 2) {
//     if (PutMapMode == 0) {
//       _DAT_80097f6c = 0xa0;
//       _DAT_80097f70 = 0;
//       PutMapMode = 1;
//       (ply->ply).r0 = '\0';
//       (ply->ply).g0 = '\0';
//       (ply->ply).b0 = '\0';
//       SoundEx((VECTOR *)0x0,0x27);
//     }
//   }
//   else if (PutMapMode == 2) {
//     _DAT_80097f6c = 0;
//     _DAT_80097f70 = 0;
//     (ply->ply).r0 = uVar3;
//     (ply->ply).g0 = uVar3;
//     (ply->ply).b0 = uVar3;
//     FUN_8003d768(((CamState.Owner)->model->locate).coord.t[0],
//                  ((CamState.Owner)->model->locate).coord.t[2],StageID * 0x10 + -0x7ff71af4);
//   }
//   MapImage.scalex = 0x1000;
//   MapImage.scaley = 0x1000;
//   MapImage.x = DAT_80097f6c;
//   MapImage.y = DAT_80097f70;
//   GsSortSprite(&MapImage,OTablePt,1);
//   AddXF4((undefined *)(OTablePt->org + 2),ply);
//   return;
// }
