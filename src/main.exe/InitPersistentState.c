#include "common.h"
#include "main.exe.h"
#include "item.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char gNannido;
 *     extern unsigned char gSound;
 *     extern unsigned char gSoundLevel;
 *     extern unsigned char gSELevel;
 *     extern unsigned char gfMemory;
 * END PSX.SYM */

/*
 * InitPersistentState (0x80016134) — if the persistent-state blob at
 * 0x80010000 is uninitialised or corrupt (CHOSEN_CHARACTER has any bit but
 * bit0 set, or CHOSEN_STAGE is out of range >10), wipe it (memset 0xE70) and
 * seed it with defaults: magic 0x19981110 at offset 0, audio/config bytes at
 * 0x58..0x61, the per-character shop-stock row 0 (0xFE-filled then patched),
 * a copy of that row into char 1's slot, the default item counts, then pick
 * mono/stereo and re-enter the stage-select menu. Returns 0 when it
 * (re)initialised, 1 when the existing state was already valid.
 *
 * Most fields named in game_types.h use member access elsewhere. This
 * initializer keeps raw byte offsets for its tightly ordered stores even
 * though 0x58..0x5E are now identified TLinkInfo options; 0x5F..0x61 remain
 * unknown adjacent flags. The 0x59 field is InitSoundEffect's mono/stereo
 * byte (gSound); the mono/stereo dispatch takes InitSoundEffect's inverted
 * `!= 0 ? Stereo : Mono` polarity (beqz into the physically-later Mono block).
 *
 * STATUS: MATCHING — pure C, all 368 bytes / 92 instructions exact.
 *
 * The stock fill spells the target's two induction values directly: `i`
 * counts down while `stockp` walks down, and the fixed 0x40c displacement is
 * retained on the store. Building `stockp` as `0x80010000 | i` yields the
 * target's `lui/or` producer without letting CSE merge it with `ps`.
 *
 * `magic` and `fill` are named producer values. The empty one-shot boundary
 * after `magic` partitions setup scheduling without changing allocation
 * weight, producing `lui/ori magic; li fill; li i; lui/or stockp; lui ps`.
 * Direct `return 0` / `return 1` paths keep the result in $v0 rather than
 * introducing a function-wide carrier and an epilogue copy.
 */

extern void SsSetMono(void);
extern void SsSetStereo(void);
extern void SelectStage(TLinkInfo *ps);

typedef struct
{
    u32 w[8];
} StockRow;

s32 InitPersistentState(void)
{
    TLinkInfo *pg =
        (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;
    TLinkInfo *ps;
    s32 i;
    u32 magic;
    u8 fill;
    u8 *stockp;

    if ((pg->CharType & 0xfe) != 0 || 10 < pg->StageNo) {
        memset((void *)TENCHU_PERSISTENT_STATE_ADDRESS, 0,
               TENCHU_PERSISTENT_STATE_SIZE);
        magic = 0x19981110;
        
        fill = 0xfe;
        i = 0x1f;
        stockp = (u8 *)(TENCHU_PERSISTENT_STATE_ADDRESS | i);
        ps = (TLinkInfo *)TENCHU_PERSISTENT_STATE_ADDRESS;
        *(u32 *)ps = magic;
        ((u8 *)ps)[0x58] = 0;
        ((u8 *)ps)[0x59] = 1;
        ((u8 *)ps)[0x5a] = 0x7f;
        ((u8 *)ps)[0x5b] = 0x7f;
        ((u8 *)ps)[0x5c] = 0;
        ((u8 *)ps)[0x5d] = 1;
        ((u8 *)ps)[0x61] = 1;
        ((u8 *)ps)[0x60] = 1;
        do {
            stockp[0x40c] = fill;
            i--;
            stockp--;
        } while (i >= 0);
        ps->stock[0] = 0xff;
        ps->stock[1] = 6;
        ps->stock[2] = 6;
        ps->stock[3] = 2;
        ps->stock[4] = 1;
        ps->stock[5] = 1;
        ps->stock[7] = 3;
        ps->stock[8] = 5;
        *(StockRow *)&ps->stock[0x20] = *(StockRow *)&ps->stock[0];
        ps->counts[1] = 10;
        ps->counts[0] = 0xff;
        ps->counts[2] = 5;
        ps->counts[3] = 2;
        ps->layout = 0xff;
        if (((u8 *)ps)[0x59] != 0) {
            SsSetStereo();
        } else {
            SsSetMono();
        }
        SelectStage(ps);
        ((u8 *)ps)[0x5f] = 0;
        return 0;
    }
    return 1;
}

// triage: MEDIUM — 92 insns, 1 loop, 4 callees, ~0.08 to load_font_image_into_global
// likely-relevant cookbook sections:
//   - Loops: 1 back-edge(s) — for/while/do vs goto shape

// Ghidra decompilation (reference — turn this into matching C,
// then drop the INCLUDE_ASM above):
//
//
// undefined4 InitPersistentState(void)
//
// {
//   undefined4 uVar1;
//   int iVar2;
//   int iVar3;
//
//   if (((PersistentState._4_1_ & 0xfe) != 0) || (uVar1 = 1, 10 < (byte)PersistentState._5_1_)) {
//     memset((uchar *)&PersistentState,'\0',0xe70);
//     iVar3 = 0x1f;
//     iVar2 = -0x7ffeffe1;
//     PersistentState._0_4_ = 0x19981110;
//     PersistentState._88_1_ = 0;
//     PersistentState._89_1_ = 1;
//     PersistentState._90_1_ = 0x7f;
//     PersistentState._91_1_ = 0x7f;
//     PersistentState._92_1_ = 0;
//     PersistentState._93_1_ = 1;
//     PersistentState._97_1_ = 1;
//     PersistentState._96_1_ = 1;
//     do {
//       *(undefined1 *)(iVar2 + 0x40c) = 0xfe;
//       iVar3 = iVar3 + -1;
//       iVar2 = iVar2 + -1;
//     } while (-1 < iVar3);
//     PersistentState._1040_2_ = 0x101;
//     PersistentState._1036_4_ = 0x20606ff;
//     PersistentState._1043_1_ = 3;
//     PersistentState._1044_1_ = 5;
//     PersistentState._1068_4_ = 0x20606ff;
//     PersistentState._1072_4_ = PersistentState._1040_4_;
//     PersistentState._1076_4_ = PersistentState._1044_4_;
//     PersistentState._1080_4_ = PersistentState._1048_4_;
//     PersistentState._1084_4_ = PersistentState._1052_4_;
//     PersistentState._1088_4_ = PersistentState._1056_4_;
//     PersistentState._1092_4_ = PersistentState._1060_4_;
//     PersistentState._1096_4_ = PersistentState._1064_4_;
//     PersistentState._8_1_ = 10;
//     PersistentState._7_1_ = 0xff;
//     PersistentState._9_1_ = 5;
//     PersistentState._10_1_ = 2;
//     PersistentState._6_1_ = 0xff;
//     if (PersistentState._89_1_ == '\0') {
//       SsSetMono();
//     }
//     else {
//       SsSetStereo();
//     }
//     FUN_8005c404(&PersistentState);
//     PersistentState._95_1_ = 0;
//     uVar1 = 0;
//   }
//   return uVar1;
// }
