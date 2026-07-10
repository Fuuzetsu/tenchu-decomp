#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void jt_init4(void);
 *     WORLD.C:1329, 110 src lines, frame 0 bytes, saved-reg mask 0x00000000
 *
 * Globals it touches, as the original declared them:
 *     extern struct tag_TItem items[30];
 * END PSX.SYM */

INCLUDE_ASM("config/../.shake/gen/main.exe/asm/nonmatchings/jt_init4", jt_init4);

// triage: MEDIUM — 156 insns, 0 callees, ~0.07 to initialise_font

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// /* WARNING: Unknown calling convention -- yet parameter storage is locked */
//
// void jt_init4(void)
//
// {
//   GsFCALL4[0] = dmyGsTMDfastF3L;
//   GsFCALL4[2] = dmyGsTMDfastF3NL;
//   GsFCALL4[5] = dmyGsTMDfastF3NL;
//   GsFCALL4[6] = GsTMDfastNF3;
//   GsFCALL4[3] = dmyGsTMDfastF3L;
//   GsFCALL4[1] = dmyGsTMDfastF3LFG;
//   GsFCALL4[4] = dmyGsTMDfastF3LFG;
//   GsFCALL4[7] = GsA4divNF3;
//   GsFCALL4[10] = dmyGsTMDfastG3NL;
//   GsFCALL4[0xd] = dmyGsTMDfastG3NL;
//   GsFCALL4[0xe] = GsTMDfastNG3;
//   GsFCALL4[8] = dmyGsTMDfastG3L;
//   GsFCALL4[0xb] = dmyGsTMDfastG3L;
//   GsFCALL4[9] = dmyGsTMDfastG3LFG;
//   GsFCALL4[0xc] = dmyGsTMDfastG3LFG;
//   GsFCALL4[0xf] = GsA4divNG3;
//   GsFCALL4[0x12] = dmyGsTMDfastTF3NL;
//   GsFCALL4[0x15] = dmyGsTMDfastTF3NL;
//   GsFCALL4[0x16] = GsTMDfastTNF3;
//   GsFCALL4[0x10] = GsTMDfastTF3L;
//   GsFCALL4[0x13] = GsTMDfastTF3L;
//   GsFCALL4[0x11] = GsTMDfastTF3LFG;
//   GsFCALL4[0x14] = GsTMDfastTF3LFG;
//   GsFCALL4[0x17] = GsA4divTNF3;
//   GsFCALL4[0x1a] = dmyGsTMDfastTG3NL;
//   GsFCALL4[0x1d] = dmyGsTMDfastTG3NL;
//   GsFCALL4[0x1e] = GsTMDfastTNG3;
//   GsFCALL4[0x18] = dmyGsTMDfastTG3L;
//   GsFCALL4[0x1b] = dmyGsTMDfastTG3L;
//   GsFCALL4[0x19] = dmyGsTMDfastTG3LFG;
//   GsFCALL4[0x1c] = dmyGsTMDfastTG3LFG;
//   GsFCALL4[0x1f] = GsA4divTNG3;
//   GsFCALL4[0x20] = dmyGsTMDfastF4L;
//   GsFCALL4[0x21] = dmyGsTMDfastF4LFG;
//   GsFCALL4[0x22] = dmyGsTMDfastF4NL;
//   GsFCALL4[0x25] = dmyGsTMDfastF4NL;
//   GsFCALL4[0x26] = GsTMDfastNF4;
//   GsFCALL4[0x23] = dmyGsTMDfastF4L;
//   GsFCALL4[0x24] = dmyGsTMDfastF4LFG;
//   GsFCALL4[0x27] = GsA4divNF4;
//   GsFCALL4[0x2a] = dmyGsTMDfastG4NL;
//   GsFCALL4[0x2d] = dmyGsTMDfastG4NL;
//   GsFCALL4[0x2e] = GsTMDfastNG4;
//   GsFCALL4[0x28] = dmyGsTMDfastG4L;
//   GsFCALL4[0x2b] = dmyGsTMDfastG4L;
//   GsFCALL4[0x29] = dmyGsTMDfastG4LFG;
//   GsFCALL4[0x2c] = dmyGsTMDfastG4LFG;
//   GsFCALL4[0x2f] = GsA4divNG4;
//   GsFCALL4[0x32] = dmyGsTMDfastTF4NL;
//   GsFCALL4[0x35] = dmyGsTMDfastTF4NL;
//   GsFCALL4[0x36] = GsTMDfastTNF4;
//   GsFCALL4[0x30] = dmyGsTMDfastTF4L;
//   GsFCALL4[0x33] = dmyGsTMDfastTF4L;
//   GsFCALL4[0x31] = dmyGsTMDfastTF4LFG;
//   GsFCALL4[0x34] = dmyGsTMDfastTF4LFG;
//   GsFCALL4[0x37] = GsA4divTNF4;
//   GsFCALL4[0x3a] = dmyGsTMDfastTG4NL;
//   GsFCALL4[0x3d] = dmyGsTMDfastTG4NL;
//   GsFCALL4[0x3e] = GsTMDfastTNG4;
//   GsFCALL4[0x3f] = GsA4divTNG4;
//   DAT_8008f7c8 = 0x25a;
//   GsFCALL4[0x38] = dmyGsTMDfastTG4L;
//   GsFCALL4[0x39] = dmyGsTMDfastTG4LFG;
//   GsFCALL4[0x3b] = dmyGsTMDfastTG4L;
//   GsFCALL4[0x3c] = dmyGsTMDfastTG4LFG;
//   DAT_8008f7cc = 0x200;
//   DAT_8008f7ce = 0x200;
//   return;
// }
