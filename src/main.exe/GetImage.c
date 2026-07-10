#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * struct GsIMAGE * GetImage(int index);
 *     IMAGES.C:52, 16 src lines, frame 40 bytes, saved-reg mask 0x801f0000
 *
 * Original parameters and locals (the demo build's register allocation may
 * differ from retail, but the COUNT and TYPES drive cc1's codegen and carry
 * over). A repeated name is a nested-block scope, not a duplicate:
 *     param $a0       int index
 *
 * Globals it touches, as the original declared them:
 *     extern struct GsIMAGE Images[52];
 * END PSX.SYM */

INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/GetImage", GetImage);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/GetImage", load_images_archive__override__prt_8004f2cc_394600bf);
INCLUDE_ASM(".shake/gen/main.exe/asm/nonmatchings/GetImage", load_images_archive__override__prt_8004f34c_394600bf);

// triage: EASY — 64 insns, 1 loop, 6 callees, ~0.09 to FileWrite
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape
//   - gp vs absolute globals: gp-relative smalls — tools/gpsyms.py

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// GsIMAGE * GetImage(int index)
//
// {
//   ulong *pt;
//   ulong *adr;
//   int iVar1;
//   GsIMAGE *pGVar2;
//
//   if (DAT_80097c90 == '\0') {
//     pt = FileRead("K:\\WORK\\CDIMAGE\\IMAGE\\images.arc");
//     if ((short)*pt < 0x3e) {
//       AdtMessageBox("bad image file");
//     }
//     iVar1 = 0;
//     pGVar2 = Images;
//     do {
//       adr = (ulong *)FUN_8004f1d8(pt,iVar1);
//       GetTIMInfo(adr,pGVar2);
//       LoadTIM(adr);
//       iVar1 = iVar1 + 1;
//       pGVar2 = pGVar2 + 1;
//     } while (iVar1 < 0x3e);
//     vfree((undefined *)pt);
//     DAT_80097c90 = '\x01';
//   }
//   if ((uint)index < 0x3e) {
//     pGVar2 = Images + index;
//   }
//   else {
//     AdtMessageBox("bad image index");
//     pGVar2 = Images;
//   }
//   return pGVar2;
// }
