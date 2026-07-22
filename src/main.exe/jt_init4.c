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
 * Initializes the fast TMD primitive dispatch table. Each family contains
 * two rows of L/LFG/NL handlers followed by the N/divide pair.
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

extern _GsFCALL GsFCALL4;

extern unsigned char *dmyGsTMDfastF3L();
extern unsigned char *dmyGsTMDfastF3LFG();
extern unsigned char *dmyGsTMDfastF3NL();
extern unsigned char *GsTMDfastNF3();
extern unsigned char *dmyGsTMDfastG3L();
extern unsigned char *dmyGsTMDfastG3LFG();
extern unsigned char *dmyGsTMDfastG3NL();
extern unsigned char *GsTMDfastNG3();
extern unsigned char *GsTMDfastTF3L();
extern unsigned char *GsTMDfastTF3LFG();
extern unsigned char *dmyGsTMDfastTF3NL();
extern unsigned char *GsTMDfastTNF3();
extern unsigned char *dmyGsTMDfastTG3L();
extern unsigned char *dmyGsTMDfastTG3LFG();
extern unsigned char *dmyGsTMDfastTG3NL();
extern unsigned char *GsTMDfastTNG3();
extern unsigned char *dmyGsTMDfastF4L();
extern unsigned char *dmyGsTMDfastF4LFG();
extern unsigned char *dmyGsTMDfastF4NL();
extern unsigned char *GsTMDfastNF4();
extern unsigned char *dmyGsTMDfastG4L();
extern unsigned char *dmyGsTMDfastG4LFG();
extern unsigned char *dmyGsTMDfastG4NL();
extern unsigned char *GsTMDfastNG4();
extern unsigned char *dmyGsTMDfastTF4L();
extern unsigned char *dmyGsTMDfastTF4LFG();
extern unsigned char *dmyGsTMDfastTF4NL();
extern unsigned char *GsTMDfastTNF4();
extern unsigned char *dmyGsTMDfastTG4L();
extern unsigned char *dmyGsTMDfastTG4LFG();
extern unsigned char *dmyGsTMDfastTG4NL();
extern unsigned char *GsTMDfastTNG4();

extern s32 D_8008F7C8;
extern s16 D_8008F7CC;
extern s16 D_8008F7CE;

void jt_init4(void)
{
    GsFCALL4.f3[0][0] = dmyGsTMDfastF3L;
    GsFCALL4.f3[0][1] = dmyGsTMDfastF3LFG;
    GsFCALL4.f3[0][2] = dmyGsTMDfastF3NL;
    GsFCALL4.f3[1][0] = dmyGsTMDfastF3L;
    GsFCALL4.f3[1][1] = dmyGsTMDfastF3LFG;
    GsFCALL4.f3[1][2] = dmyGsTMDfastF3NL;
    GsFCALL4.nf3[0] = GsTMDfastNF3;
    GsFCALL4.nf3[1] = GsA4divNF3;

    GsFCALL4.g3[0][0] = dmyGsTMDfastG3L;
    GsFCALL4.g3[0][1] = dmyGsTMDfastG3LFG;
    GsFCALL4.g3[0][2] = dmyGsTMDfastG3NL;
    GsFCALL4.g3[1][0] = dmyGsTMDfastG3L;
    GsFCALL4.g3[1][1] = dmyGsTMDfastG3LFG;
    GsFCALL4.g3[1][2] = dmyGsTMDfastG3NL;
    GsFCALL4.ng3[0] = GsTMDfastNG3;
    GsFCALL4.ng3[1] = GsA4divNG3;

    GsFCALL4.tf3[0][0] = GsTMDfastTF3L;
    GsFCALL4.tf3[0][1] = GsTMDfastTF3LFG;
    GsFCALL4.tf3[0][2] = dmyGsTMDfastTF3NL;
    GsFCALL4.tf3[1][0] = GsTMDfastTF3L;
    GsFCALL4.tf3[1][1] = GsTMDfastTF3LFG;
    GsFCALL4.tf3[1][2] = dmyGsTMDfastTF3NL;
    GsFCALL4.ntf3[0] = GsTMDfastTNF3;
    GsFCALL4.ntf3[1] = GsA4divTNF3;

    GsFCALL4.tg3[0][0] = dmyGsTMDfastTG3L;
    GsFCALL4.tg3[0][1] = dmyGsTMDfastTG3LFG;
    GsFCALL4.tg3[0][2] = dmyGsTMDfastTG3NL;
    GsFCALL4.tg3[1][0] = dmyGsTMDfastTG3L;
    GsFCALL4.tg3[1][1] = dmyGsTMDfastTG3LFG;
    GsFCALL4.tg3[1][2] = dmyGsTMDfastTG3NL;
    GsFCALL4.ntg3[0] = GsTMDfastTNG3;
    GsFCALL4.ntg3[1] = GsA4divTNG3;

    GsFCALL4.f4[0][0] = dmyGsTMDfastF4L;
    GsFCALL4.f4[0][1] = dmyGsTMDfastF4LFG;
    GsFCALL4.f4[0][2] = dmyGsTMDfastF4NL;
    GsFCALL4.f4[1][0] = dmyGsTMDfastF4L;
    GsFCALL4.f4[1][1] = dmyGsTMDfastF4LFG;
    GsFCALL4.f4[1][2] = dmyGsTMDfastF4NL;
    GsFCALL4.nf4[0] = GsTMDfastNF4;
    GsFCALL4.nf4[1] = GsA4divNF4;

    GsFCALL4.g4[0][0] = dmyGsTMDfastG4L;
    GsFCALL4.g4[0][1] = dmyGsTMDfastG4LFG;
    GsFCALL4.g4[0][2] = dmyGsTMDfastG4NL;
    GsFCALL4.g4[1][0] = dmyGsTMDfastG4L;
    GsFCALL4.g4[1][1] = dmyGsTMDfastG4LFG;
    GsFCALL4.g4[1][2] = dmyGsTMDfastG4NL;
    GsFCALL4.ng4[0] = GsTMDfastNG4;
    GsFCALL4.ng4[1] = GsA4divNG4;

    GsFCALL4.tf4[0][0] = dmyGsTMDfastTF4L;
    GsFCALL4.tf4[0][1] = dmyGsTMDfastTF4LFG;
    GsFCALL4.tf4[0][2] = dmyGsTMDfastTF4NL;
    GsFCALL4.tf4[1][0] = dmyGsTMDfastTF4L;
    GsFCALL4.tf4[1][1] = dmyGsTMDfastTF4LFG;
    GsFCALL4.tf4[1][2] = dmyGsTMDfastTF4NL;
    GsFCALL4.ntf4[0] = GsTMDfastTNF4;
    GsFCALL4.ntf4[1] = GsA4divTNF4;

    GsFCALL4.tg4[0][0] = dmyGsTMDfastTG4L;
    GsFCALL4.tg4[0][1] = dmyGsTMDfastTG4LFG;
    GsFCALL4.tg4[0][2] = dmyGsTMDfastTG4NL;
    GsFCALL4.tg4[1][0] = dmyGsTMDfastTG4L;
    GsFCALL4.tg4[1][1] = dmyGsTMDfastTG4LFG;
    GsFCALL4.tg4[1][2] = dmyGsTMDfastTG4NL;
    GsFCALL4.ntg4[0] = GsTMDfastTNG4;
    GsFCALL4.ntg4[1] = GsA4divTNG4;

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
