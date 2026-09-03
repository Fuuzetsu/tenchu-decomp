#include "common.h"
#include "main.exe.h"
#include "memcard.h"
#include "vmemory.h"

/*
 * Retail MEMCARD.C adds check_card_file_ and emits SaveCard before the other
 * routines. Both retail emission order and demo source order are recorded in
 * the translation-unit manifest.
 */

extern char CardPathFormat[];
extern char str_sjis_title[];

extern void *memset(void *dst, s32 value, u32 size);
extern void *memcpy(void *dst, const void *src, u32 size);
extern int sprintf(char *buf, char *fmt, ...);

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

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short ChkCard(void);
 *     MEMCARD.C:84, 8 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     long cmd
 *     stack sp+20     long result
 * END PSX.SYM */

card_result ChkCard(void)
{
    s32 cmd;
    enum card_result result;

    result = MemCardAccept(MEMCARD_CHANNEL_0);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    return result;
}

card_result check_card_file_(char *name)
{
    char path[200];
    s32 cmd;
    enum card_result result;
    s32 acceptCmd;
    enum card_result acceptResult;

    result = MemCardAccept(MEMCARD_CHANNEL_0);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    sprintf(path, CardPathFormat, TENCHU_ID, name);
    acceptResult = MemCardOpen(MEMCARD_CHANNEL_0, path,
                               MEMCARD_OPEN_READ_ONLY);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &acceptCmd, &acceptResult);
    if (acceptResult == CARD_RESULT_SUCCESS)
    {
        MemCardClose();
    }
    return acceptResult;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short DeleteCard(unsigned char *name);
 *     MEMCARD.C:95, 10 src lines, frame 232 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       unsigned char * name
 *     stack sp+216    long cmd
 *     stack sp+220    long result
 *     stack sp+16     unsigned char [200] fn
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char *TENCHU_ID;
 * END PSX.SYM */

card_result DeleteCard(u8 *name)
{
    u8 fn[200];
    s32 cmd;
    enum card_result result;

    sprintf((char *)fn, CardPathFormat, TENCHU_ID, name);
    result = MemCardDeleteFile(MEMCARD_CHANNEL_0, (char *)fn);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    return result;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short FormatCard(void);
 *     MEMCARD.C:108, 8 src lines, frame 32 bytes, saved-reg mask 0x80000000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     stack sp+16     long cmd
 *     stack sp+20     long result
 * END PSX.SYM */

card_result FormatCard(void)
{
    s32 cmd;
    enum card_result result;

    result = MemCardFormat(MEMCARD_CHANNEL_0);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    return result;
}

/* BEGIN PSX.SYM — the original source's own facts, from the demo disc's
 * debug symbols. Regenerate with `tools/symnote.py --write`; see
 * docs/psx-sym.md. Do not hand-edit.
 *
 * short LoadCard(int target, unsigned char *name);
 *     MEMCARD.C:119, 35 src lines, frame 8448 bytes, saved-reg mask 0x800f0000 (DEMO build -- see below)
 *
 * Demo-build parameters and locals (evidence, not a retail spec —
 * see docs/psx-sym.md):
 *     param $a0       int target
 *     param $a1       unsigned char * name
 *     stack sp+24     unsigned char [200] fn
 *     stack sp+8416   long cmd
 *     stack sp+8420   long result
 *     stack sp+224    unsigned char [8192] block
 *
 * Globals it touches, as the original declared them:
 *     extern unsigned char *TENCHU_ID;
 * END PSX.SYM */

card_result LoadCard(s32 target, u8 *name)
{
    void *allocation;
    u8 fn[200];
    MemoryCardFileBlock block;
    s32 cmd;
    enum card_result result;

    allocation = valloc(BLOCKSIZE);
    result = MemCardAccept(MEMCARD_CHANNEL_0);
    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    sprintf(fn, CardPathFormat, TENCHU_ID, name);
    result = MemCardReadFile(MEMCARD_CHANNEL_0, (char *)fn, &block, 0,
                             sizeof(block));
    MemCardSync(MEMCARD_SYNC_BLOCKING, &cmd, &result);
    if (result != CARD_RESULT_SUCCESS)
    {
        vfree(allocation);
        allocation = 0;
    }
    else
    {
        __builtin_memcpy((void *)TENCHU_PERSISTENT_STATE_ADDRESS,
                         block.payload,
                         TENCHU_PERSISTENT_STATE_SIZE);
    }
    vfree(allocation);
    return result;
}
