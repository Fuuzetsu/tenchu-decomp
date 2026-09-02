#include "common.h"
#include "main.exe.h"
#include "memcard.h"

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short SaveCard(int target, unsigned char *name, void *mem, long size);
 *     MEMCARD.C:157, 38 src lines, frame 8464 bytes, saved-reg mask 0x80ff0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int target
 *     param $s5       unsigned char * name
 *     param $s6       void * mem
 *     param $s7       long size
 *     stack sp+24     unsigned char [200] fn
 *     stack sp+8416   long cmd
 *     stack sp+8420   long result
 *     reg   $s4       long chan
 *     stack sp+224    unsigned char [8192] block
 *     reg   $s2       struct TCardHeader * hd
 *     reg   $s3       void * data
 *     reg   $s2       struct TCardHeader * hd
 *     reg   $t1       unsigned char * icon3
 *     reg   $s1       unsigned char * icon2
 *     reg   $s0       unsigned char * icon1
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char *TENCHU_ID;
 * END PSX.SYM */

extern char CardPathFormat[];
extern char str_sjis_title[];

extern void *memset(void *dst, s32 value, u32 size);
extern void *memcpy(void *dst, const void *src, u32 size);
extern int sprintf(char *buf, char *fmt, ...);

/* The card header and save payload are one MemoryCardFileBlock. */
card_result SaveCard(s32 target, u8 *name, void *mem, s32 size, s16 write_data)
{
    u8 fn[200];
    MemoryCardFileBlock block;
    s32 cmd;
    enum card_result result;
    enum memcard_channel chan;
    TCardHeader *hd;
    void *data;
    u8 *icon1;
    u8 *icon2;
    u8 *icon3;

    hd = &block.header;

    hd->Magic[0] = 'S';
    hd->Magic[1] = 'C';
    hd->Type = SAVE_ICON_3_FRAMES;
    hd->BlockEntry = CARD_FILE_BLOCKS;
    memset(hd->Title, 0, sizeof(hd->Title));
    sprintf(hd->Title, str_sjis_title);
    memset(hd->reserve, 0, sizeof(hd->reserve));

    icon1 = (u8 *)GetArcData(ICON_CARD1);
    icon2 = (u8 *)GetArcData(ICON_CARD2);
    chan = MEMCARD_CHANNEL_0;
    icon3 = (u8 *)GetArcData(ICON_CARD3);
    data = block.payload;
    __builtin_memcpy(hd->Clut, CARD_ICON_TIM_CLUT(icon1), sizeof(hd->Clut));
    __builtin_memcpy(hd->Icon[0], CARD_ICON_TIM_PIXELS(icon1),
                     sizeof(hd->Icon[0]));
    __builtin_memcpy(hd->Icon[1], CARD_ICON_TIM_PIXELS(icon2),
                     sizeof(hd->Icon[1]));
    __builtin_memcpy(hd->Icon[2], CARD_ICON_TIM_PIXELS(icon3),
                     sizeof(hd->Icon[2]));

    sprintf(fn, CardPathFormat, TENCHU_ID, name);
    result = MemCardCreateFile(chan, (char *)fn, CARD_FILE_BLOCKS);
    if ((result == CARD_RESULT_SUCCESS || result == CARD_RESULT_FILE_EXISTS) &&
        write_data != 0)
    {
        memcpy(data, mem, size);
        result = MemCardWriteFile(chan, (char *)fn, &block, 0,
                                  sizeof(block));
        MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    }
    return result;
}
