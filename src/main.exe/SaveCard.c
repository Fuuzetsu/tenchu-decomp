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
 * Original parameters and locals (the demo COUNT and TYPES are high-value
 * codegen evidence, not a retail spec: an earlier-build helper/API change
 * can replace either). Retail access widths and callee ABI win. A repeated
 * name is a nested-block scope, not a duplicate.
 * A ZERO-locals record is unverified, not a claim that the function has none:
 * vfree lists zero locals yet its byte-matched source needs seven.
 * The frame size and saved-reg mask above are the DEMO's: retail often needs
 * FEWER callee-saved registers (measured: Think1random exact; Think1chase's
 * 0x800f0000 = s0-s3+ra vs retail's s0,s1,ra). Treat them as an upper bound
 * and a hint at how many values stay live, never as a spec. The asm wins.
 * Locals:
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
extern s32 MemCardCreateFile(s32 chan, char *name, s32 blocks);
extern s32 MemCardWriteFile(s32 chan, char *name, void *data, s32 offset,
                            s32 size);
extern s32 MemCardSync(s32 mode, s32 *cmd, s32 *result);

/*
 * The 0x200-byte card header and payload are two views into a single 8 KiB
 * block. Fixed-size built-in copies use TCardHeader's recovered Clut/Icon
 * bounds while preserving the original compiler's aligned/unaligned loops.
 */
s16 SaveCard(s32 target, u8 *name, void *mem, s32 size, s16 write_data)
{
    u8 fn[200];
    u8 block[BLOCKSIZE];
    s32 cmd;
    s32 result;
    s32 chan;
    TCardHeader *hd;
    void *data;
    u8 *icon1;
    u8 *icon2;
    u8 *icon3;

    hd = (TCardHeader *)block;

    hd->Magic[0] = 0x53;
    hd->Magic[1] = 0x43;
    hd->Type = 0x13;
    hd->BlockEntry = 1;
    memset(hd->Title, 0, sizeof(hd->Title));
    sprintf(hd->Title, str_sjis_title);
    memset(hd->reserve, 0, sizeof(hd->reserve));

    icon1 = (u8 *)GetArcData(ICON_CARD1);
    icon2 = (u8 *)GetArcData(ICON_CARD2);
    chan = 0;
    icon3 = (u8 *)GetArcData(ICON_CARD3);
    data = block + sizeof(TCardHeader);
    __builtin_memcpy(hd->Clut, icon1 + 0x14, sizeof(hd->Clut));
    __builtin_memcpy(hd->Icon[0], icon1 + 0x40, sizeof(hd->Icon[0]));
    __builtin_memcpy(hd->Icon[1], icon2 + 0x40, sizeof(hd->Icon[1]));
    __builtin_memcpy(hd->Icon[2], icon3 + 0x40, sizeof(hd->Icon[2]));

    sprintf(fn, CardPathFormat, TENCHU_ID, name);
    result = MemCardCreateFile(chan, fn, 1);
    if ((result == 0 || result == 6) && write_data != 0)
    {
        memcpy(data, mem, size);
        result = MemCardWriteFile(chan, fn, block, 0, BLOCKSIZE);
        MemCardSync(0, &cmd, &result);
    }
    return result;
}
