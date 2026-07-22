#include "common.h"
#include "main.exe.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * void jt_init4(void);
 *     WORLD.C:1329, 110 src lines, frame 0 bytes, saved-reg mask 0x00000000 (DEMO build -- see below)
 * END PSX.SYM */

/*
 * Initializes the 64-entry fast TMD primitive dispatch table. Each block of
 * eight handlers is laid out as L, LFG, NL, L, LFG, NL, N, divide for the
 * F3/G3/TF3/TG3/F4/G4/TF4/TG4 primitive families.
 *
 * Matching notes:
 *  - The assignments must remain in logical index order. Ghidra rendered the
 *    scheduler's physical store order instead; transcribing that order kept
 *    the first handler live until the F4 block and produced a same-length but
 *    479-byte residual. Sequential indices let cc1 retain the three repeated
 *    addresses in $a1/$a0/$v0 and scatter their stores exactly like retail.
 *  - The final configuration stores are one word followed by two halfwords,
 *    as proven by the retail `sw`/`sh` access widths.
 */

typedef void (*GsTMDHandler)(void);

extern GsTMDHandler GsFCALL4[64];

extern void dmyGsTMDfastF3L(void);
extern void dmyGsTMDfastF3LFG(void);
extern void dmyGsTMDfastF3NL(void);
extern void GsTMDfastNF3(void);
extern void dmyGsTMDfastG3L(void);
extern void dmyGsTMDfastG3LFG(void);
extern void dmyGsTMDfastG3NL(void);
extern void GsTMDfastNG3(void);
extern void GsTMDfastTF3L(void);
extern void GsTMDfastTF3LFG(void);
extern void dmyGsTMDfastTF3NL(void);
extern void GsTMDfastTNF3(void);
extern void dmyGsTMDfastTG3L(void);
extern void dmyGsTMDfastTG3LFG(void);
extern void dmyGsTMDfastTG3NL(void);
extern void GsTMDfastTNG3(void);
extern void dmyGsTMDfastF4L(void);
extern void dmyGsTMDfastF4LFG(void);
extern void dmyGsTMDfastF4NL(void);
extern void GsTMDfastNF4(void);
extern void dmyGsTMDfastG4L(void);
extern void dmyGsTMDfastG4LFG(void);
extern void dmyGsTMDfastG4NL(void);
extern void GsTMDfastNG4(void);
extern void dmyGsTMDfastTF4L(void);
extern void dmyGsTMDfastTF4LFG(void);
extern void dmyGsTMDfastTF4NL(void);
extern void GsTMDfastTNF4(void);
extern void dmyGsTMDfastTG4L(void);
extern void dmyGsTMDfastTG4LFG(void);
extern void dmyGsTMDfastTG4NL(void);
extern void GsTMDfastTNG4(void);

extern s32 D_8008F7C8;
extern s16 D_8008F7CC;
extern s16 D_8008F7CE;

void jt_init4(void)
{
    GsFCALL4[0] = dmyGsTMDfastF3L;
    GsFCALL4[1] = dmyGsTMDfastF3LFG;
    GsFCALL4[2] = dmyGsTMDfastF3NL;
    GsFCALL4[3] = dmyGsTMDfastF3L;
    GsFCALL4[4] = dmyGsTMDfastF3LFG;
    GsFCALL4[5] = dmyGsTMDfastF3NL;
    GsFCALL4[6] = GsTMDfastNF3;
    GsFCALL4[7] = GsA4divNF3;

    GsFCALL4[8] = dmyGsTMDfastG3L;
    GsFCALL4[9] = dmyGsTMDfastG3LFG;
    GsFCALL4[10] = dmyGsTMDfastG3NL;
    GsFCALL4[11] = dmyGsTMDfastG3L;
    GsFCALL4[12] = dmyGsTMDfastG3LFG;
    GsFCALL4[13] = dmyGsTMDfastG3NL;
    GsFCALL4[14] = GsTMDfastNG3;
    GsFCALL4[15] = GsA4divNG3;

    GsFCALL4[16] = GsTMDfastTF3L;
    GsFCALL4[17] = GsTMDfastTF3LFG;
    GsFCALL4[18] = dmyGsTMDfastTF3NL;
    GsFCALL4[19] = GsTMDfastTF3L;
    GsFCALL4[20] = GsTMDfastTF3LFG;
    GsFCALL4[21] = dmyGsTMDfastTF3NL;
    GsFCALL4[22] = GsTMDfastTNF3;
    GsFCALL4[23] = GsA4divTNF3;

    GsFCALL4[24] = dmyGsTMDfastTG3L;
    GsFCALL4[25] = dmyGsTMDfastTG3LFG;
    GsFCALL4[26] = dmyGsTMDfastTG3NL;
    GsFCALL4[27] = dmyGsTMDfastTG3L;
    GsFCALL4[28] = dmyGsTMDfastTG3LFG;
    GsFCALL4[29] = dmyGsTMDfastTG3NL;
    GsFCALL4[30] = GsTMDfastTNG3;
    GsFCALL4[31] = GsA4divTNG3;

    GsFCALL4[32] = dmyGsTMDfastF4L;
    GsFCALL4[33] = dmyGsTMDfastF4LFG;
    GsFCALL4[34] = dmyGsTMDfastF4NL;
    GsFCALL4[35] = dmyGsTMDfastF4L;
    GsFCALL4[36] = dmyGsTMDfastF4LFG;
    GsFCALL4[37] = dmyGsTMDfastF4NL;
    GsFCALL4[38] = GsTMDfastNF4;
    GsFCALL4[39] = GsA4divNF4;

    GsFCALL4[40] = dmyGsTMDfastG4L;
    GsFCALL4[41] = dmyGsTMDfastG4LFG;
    GsFCALL4[42] = dmyGsTMDfastG4NL;
    GsFCALL4[43] = dmyGsTMDfastG4L;
    GsFCALL4[44] = dmyGsTMDfastG4LFG;
    GsFCALL4[45] = dmyGsTMDfastG4NL;
    GsFCALL4[46] = GsTMDfastNG4;
    GsFCALL4[47] = GsA4divNG4;

    GsFCALL4[48] = dmyGsTMDfastTF4L;
    GsFCALL4[49] = dmyGsTMDfastTF4LFG;
    GsFCALL4[50] = dmyGsTMDfastTF4NL;
    GsFCALL4[51] = dmyGsTMDfastTF4L;
    GsFCALL4[52] = dmyGsTMDfastTF4LFG;
    GsFCALL4[53] = dmyGsTMDfastTF4NL;
    GsFCALL4[54] = GsTMDfastTNF4;
    GsFCALL4[55] = GsA4divTNF4;

    GsFCALL4[56] = dmyGsTMDfastTG4L;
    GsFCALL4[57] = dmyGsTMDfastTG4LFG;
    GsFCALL4[58] = dmyGsTMDfastTG4NL;
    GsFCALL4[59] = dmyGsTMDfastTG4L;
    GsFCALL4[60] = dmyGsTMDfastTG4LFG;
    GsFCALL4[61] = dmyGsTMDfastTG4NL;
    GsFCALL4[62] = GsTMDfastTNG4;
    GsFCALL4[63] = GsA4divTNG4;

    D_8008F7C8 = 0x25A;
    D_8008F7CC = 0x200;
    D_8008F7CE = 0x200;
}

// triage: MEDIUM — 156 insns, 0 callees, ~0.07 to initialise_font

// Ghidra decompilation reference:
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

// m2c (mipsel-gcc-c reference — cleaner control flow + register
// temps straight from the asm; Ghidra above has the real types):
//
// extern s32 D_8008F7C8;
// extern s16 D_8008F7CC;
// extern s16 D_8008F7CE;
// extern ? GsA4divNF3;
// extern ? GsA4divNF4;
// extern ? GsA4divNG3;
// extern ? GsA4divNG4;
// extern ? GsA4divTNF3;
// extern ? GsA4divTNF4;
// extern ? GsA4divTNG3;
// extern ? GsA4divTNG4;
// extern ? GsFCALL4;
// extern ? GsTMDfastNF3;
// extern ? GsTMDfastNF4;
// extern ? GsTMDfastNG3;
// extern ? GsTMDfastNG4;
// extern ? GsTMDfastTF3L;
// extern ? GsTMDfastTF3LFG;
// extern ? GsTMDfastTNF3;
// extern ? GsTMDfastTNF4;
// extern ? GsTMDfastTNG3;
// extern ? GsTMDfastTNG4;
// extern ? dmyGsTMDfastF3L;
// extern ? dmyGsTMDfastF3LFG;
// extern ? dmyGsTMDfastF3NL;
// extern ? dmyGsTMDfastF4L;
// extern ? dmyGsTMDfastF4LFG;
// extern ? dmyGsTMDfastF4NL;
// extern ? dmyGsTMDfastG3L;
// extern ? dmyGsTMDfastG3LFG;
// extern ? dmyGsTMDfastG3NL;
// extern ? dmyGsTMDfastG4L;
// extern ? dmyGsTMDfastG4LFG;
// extern ? dmyGsTMDfastG4NL;
// extern ? dmyGsTMDfastTF3NL;
// extern ? dmyGsTMDfastTF4L;
// extern ? dmyGsTMDfastTF4LFG;
// extern ? dmyGsTMDfastTF4NL;
// extern ? dmyGsTMDfastTG3L;
// extern ? dmyGsTMDfastTG3LFG;
// extern ? dmyGsTMDfastTG3NL;
// extern ? dmyGsTMDfastTG4L;
// extern ? dmyGsTMDfastTG4LFG;
// extern ? dmyGsTMDfastTG4NL;
//
// void jt_init4(void) {
//     GsFCALL4.unk0 = &dmyGsTMDfastF3L;
//     GsFCALL4.unk8 = &dmyGsTMDfastF3NL;
//     GsFCALL4.unk14 = &dmyGsTMDfastF3NL;
//     GsFCALL4.unk18 = &GsTMDfastNF3;
//     GsFCALL4.unkC = &dmyGsTMDfastF3L;
//     GsFCALL4.unk4 = &dmyGsTMDfastF3LFG;
//     GsFCALL4.unk10 = &dmyGsTMDfastF3LFG;
//     GsFCALL4.unk1C = &GsA4divNF3;
//     GsFCALL4.unk28 = &dmyGsTMDfastG3NL;
//     GsFCALL4.unk34 = &dmyGsTMDfastG3NL;
//     GsFCALL4.unk38 = &GsTMDfastNG3;
//     GsFCALL4.unk20 = &dmyGsTMDfastG3L;
//     GsFCALL4.unk2C = &dmyGsTMDfastG3L;
//     GsFCALL4.unk24 = &dmyGsTMDfastG3LFG;
//     GsFCALL4.unk30 = &dmyGsTMDfastG3LFG;
//     GsFCALL4.unk3C = &GsA4divNG3;
//     GsFCALL4.unk48 = &dmyGsTMDfastTF3NL;
//     GsFCALL4.unk54 = &dmyGsTMDfastTF3NL;
//     GsFCALL4.unk58 = &GsTMDfastTNF3;
//     GsFCALL4.unk40 = &GsTMDfastTF3L;
//     GsFCALL4.unk4C = &GsTMDfastTF3L;
//     GsFCALL4.unk44 = &GsTMDfastTF3LFG;
//     GsFCALL4.unk50 = &GsTMDfastTF3LFG;
//     GsFCALL4.unk5C = &GsA4divTNF3;
//     GsFCALL4.unk68 = &dmyGsTMDfastTG3NL;
//     GsFCALL4.unk74 = &dmyGsTMDfastTG3NL;
//     GsFCALL4.unk78 = &GsTMDfastTNG3;
//     GsFCALL4.unk60 = &dmyGsTMDfastTG3L;
//     GsFCALL4.unk6C = &dmyGsTMDfastTG3L;
//     GsFCALL4.unk64 = &dmyGsTMDfastTG3LFG;
//     GsFCALL4.unk70 = &dmyGsTMDfastTG3LFG;
//     GsFCALL4.unk7C = &GsA4divTNG3;
//     GsFCALL4.unk80 = &dmyGsTMDfastF4L;
//     GsFCALL4.unk84 = &dmyGsTMDfastF4LFG;
//     GsFCALL4.unk88 = &dmyGsTMDfastF4NL;
//     GsFCALL4.unk94 = &dmyGsTMDfastF4NL;
//     GsFCALL4.unk98 = &GsTMDfastNF4;
//     GsFCALL4.unk8C = &dmyGsTMDfastF4L;
//     GsFCALL4.unk90 = &dmyGsTMDfastF4LFG;
//     GsFCALL4.unk9C = &GsA4divNF4;
//     GsFCALL4.unkA8 = &dmyGsTMDfastG4NL;
//     GsFCALL4.unkB4 = &dmyGsTMDfastG4NL;
//     GsFCALL4.unkB8 = &GsTMDfastNG4;
//     GsFCALL4.unkA0 = &dmyGsTMDfastG4L;
//     GsFCALL4.unkAC = &dmyGsTMDfastG4L;
//     GsFCALL4.unkA4 = &dmyGsTMDfastG4LFG;
//     GsFCALL4.unkB0 = &dmyGsTMDfastG4LFG;
//     GsFCALL4.unkBC = &GsA4divNG4;
//     GsFCALL4.unkC8 = &dmyGsTMDfastTF4NL;
//     GsFCALL4.unkD4 = &dmyGsTMDfastTF4NL;
//     GsFCALL4.unkD8 = &GsTMDfastTNF4;
//     GsFCALL4.unkC0 = &dmyGsTMDfastTF4L;
//     GsFCALL4.unkCC = &dmyGsTMDfastTF4L;
//     GsFCALL4.unkC4 = &dmyGsTMDfastTF4LFG;
//     GsFCALL4.unkD0 = &dmyGsTMDfastTF4LFG;
//     GsFCALL4.unkDC = &GsA4divTNF4;
//     GsFCALL4.unkE8 = &dmyGsTMDfastTG4NL;
//     GsFCALL4.unkF4 = &dmyGsTMDfastTG4NL;
//     GsFCALL4.unkF8 = &GsTMDfastTNG4;
//     GsFCALL4.unkFC = &GsA4divTNG4;
//     D_8008F7C8 = 0x25A;
//     GsFCALL4.unkE0 = &dmyGsTMDfastTG4L;
//     GsFCALL4.unkE4 = &dmyGsTMDfastTG4LFG;
//     GsFCALL4.unkEC = &dmyGsTMDfastTG4L;
//     GsFCALL4.unkF0 = &dmyGsTMDfastTG4LFG;
//     D_8008F7CC = 0x200;
//     D_8008F7CE = 0x200;
// }
